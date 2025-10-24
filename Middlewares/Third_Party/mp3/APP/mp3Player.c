/*
******************************************************************************
* @file    recorder.c
* @author  fire
* @version V1.0
* @date    2015-xx-xx
* @brief   WM8978放音功能测试+mp3解码
******************************************************************************
* @attention
*
* 实验平台:野火  STM32 F407 开发板  
* 论坛    :http://www.chuxue123.com
* 淘宝    :http://firestm32.taobao.com
*
******************************************************************************
*/
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "./usart/bsp_debug_usart.h"
#include "delay.h"
#include "./wm8978/bsp_wm8978.h"
#include "ff.h" 
#include "mp3Player.h"
#include "mp3dec.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

/* 推荐使用以下格式mp3文件：
 * 采样率：44100Hz
 * 声  道：2
 * 比特率：320kbps
 */

/* ===== Audio playback synchronization ===== */
// 每帧解码输出样本数（int16 计数，包含双声道），MP3 MPEG1 Layer III 固定为 1152/ch -> 2304 样本
#define MP3_SAMPS_PER_FRAME  2304

// 可选：每个"可写缓冲"用几帧数据增加余量（1 足够；2 更稳健但占 RAM）
#define FRAMES_PER_BUF  1
#define BUF_SAMPS       (MP3_SAMPS_PER_FRAME * FRAMES_PER_BUF)

/* 处理立体声音频数据时，输出缓冲区需要的最大大小为2304*16/8字节(16为PCM数据为16位)，
 * 这里我们定义MP3BUFFER_SIZE为2304，实际输出缓冲区为MP3BUFFER_SIZE*2个字节
 */
#define MP3BUFFER_SIZE  BUF_SAMPS		
#define INPUTBUF_SIZE   3000

// 控制台日志开关（减少中断里的打印）
#ifndef AUDIO_DEBUG
#define AUDIO_DEBUG 0
#endif
#if AUDIO_DEBUG
#define ALOG(...)    printf(__VA_ARGS__)
#else
#define ALOG(...)    do{}while(0)
#endif	

static HMP3Decoder		Mp3Decoder;			/* mp3解码器指针	*/
static MP3FrameInfo		Mp3FrameInfo;		/* mP3帧信息  */
static MP3_TYPE mp3player;         /* mp3播放设备 */

/* 这些变量在原版工程中不需要，已移除 */

/* 旧的标志变量（保留以兼容现有代码） */
volatile uint8_t Isread=0;           /* DMA传输完成标志(volatile防止编译器优化) */
static volatile uint8_t bufflag=0;   /* 数据缓存区选择标志(volatile防止编译器优化) */

uint32_t led_delay=0;

uint8_t inputbuf[INPUTBUF_SIZE]={0};        /* 解码输入缓冲区，1940字节为最大MP3帧大小  */
static short outbuffer[2][MP3BUFFER_SIZE];  /* 解码输出缓冲区，也是I2S输入数据，实际占用字节数：RECBUFFER_SIZE*2 */

FIL file;											/* file objects */
FRESULT result; 
UINT bw;            					/* File R/W count */

/* 仅允许本文件内调用的函数声明 */
void MP3Player_I2S_DMA_TX_Callback(void);

/**
  * @brief   MP3格式音频播放主程序(参考原版工程简化实现)
  * @param  mp3file: MP3文件路径
  * @retval 无
  */
void mp3PlayerDemo(const char *mp3file)
{
	uint8_t *read_ptr = inputbuf;  /* 使用局部变量,不是static */
	uint32_t frames = 0;
	int err = 0, i = 0, outputSamps = 0;	
	int	read_offset = 0;		/* 读偏移指针 */
	int	bytes_left = 0;			/* 剩余字节数 */
	
	mp3player.ucFreq = I2S_AUDIOFREQ_DEFAULT;
	mp3player.ucStatus = STA_IDLE;
	mp3player.ucVolume = 25;
	
	result = f_open(&file, mp3file, FA_READ);
	if (result != FR_OK)
	{
		printf("[MP3] Open failed: %s (err=%d)\r\n", mp3file, result);
		result = f_close(&file);
		return;	/* 停止播放 */
	}
	printf("[MP3] Playing: %s (%.2f KB)\r\n", mp3file, f_size(&file) / 1024.0f);
	
	//初始化MP3解码器
	Mp3Decoder = MP3InitDecoder();	
	if (Mp3Decoder == 0)
	{
		printf("[MP3] Decoder init failed\r\n");
		return;	/* 停止播放 */
	}
	
	
	delay_ms(10);	/* 延迟一段时间，等待I2S中断结束 */
	wm8978_Reset();		/* 复位WM8978到复位状态 */
	/* 配置WM8978芯片，输入为DAC，输出为耳机 */
	wm8978_CfgAudioPath(DAC_ON, EAR_LEFT_ON | EAR_RIGHT_ON);

	/* 调节音量，左右相同音量 */
	wm8978_SetOUT1Volume(mp3player.ucVolume);

	/* 配置WM8978音频接口为飞利浦标准I2S接口，16bit */
	wm8978_CfgAudioIF(I2S_STANDARD_PHILIPS, 16);
	
	/*  初始化并配置I2S（参考原版工程）  */
	I2S_Stop();
	I2S_GPIO_Config();
	I2Sx_Mode_Config(I2S_STANDARD_PHILIPS, I2S_DATAFORMAT_16B, mp3player.ucFreq);	
	I2S_DMA_TX_Callback = MP3Player_I2S_DMA_TX_Callback;
	I2Sx_TX_DMA_Init((uint32_t)&outbuffer[0], (uint32_t)&outbuffer[1], MP3BUFFER_SIZE);	
	
	bufflag = 0;
	Isread = 0;
	
	mp3player.ucStatus = STA_PLAYING;		/* 放音状态 */
	result = f_read(&file, inputbuf, INPUTBUF_SIZE, &bw);
	if (result != FR_OK)
	{
		printf("[MP3] Read failed (err=%d)\r\n", result);
		MP3FreeDecoder(Mp3Decoder);
		return;
	}
	read_ptr = inputbuf;
	bytes_left = bw;
	
	/* 重置缓冲标志（清除测试音调的残留状态） */
	bufflag = 0;
	
	/* 进入主程序循环体（原版工程的简单方式） */
	while (mp3player.ucStatus == STA_PLAYING)
	{
		read_offset = MP3FindSyncWord(read_ptr, bytes_left);	//寻找帧同步，返回第一个同步字的位置
		if (read_offset < 0)	//没有找到同步字
		{
			result = f_read(&file, inputbuf, INPUTBUF_SIZE, &bw);
			if (result != FR_OK)
			{
				printf("[MP3] Read error: %d\r\n", result);
				break;
			}
			read_ptr = inputbuf;
			bytes_left = bw;
			continue;	//跳出循环2，回到循环1	
		}
		read_ptr += read_offset;	//偏移至同步字的位置
		bytes_left -= read_offset;	//同步字之后的数据大小	
		if (bytes_left < 1024)		//补充数据
		{
			/* 注意这个地方因为采用的是DMA读取，所以一定要4字节对齐  */
			i = (uint32_t)(bytes_left) & 3;	//判断多余的字节
			if (i) i = 4 - i;	//需要补充的字节
			memcpy(inputbuf + i, read_ptr, bytes_left);	//从对齐位置开始复制
			read_ptr = inputbuf + i;	//指向数据对齐位置
			result = f_read(&file, inputbuf + bytes_left + i, INPUTBUF_SIZE - bytes_left - i, &bw);//补充数据
			if (result != FR_OK)
			{
				printf("[MP3] Read error: %d\r\n", result);
				break;
			}
			bytes_left += bw;	//有效数据流大小
		}
		err = MP3Decode(Mp3Decoder, &read_ptr, &bytes_left, outbuffer[bufflag], 0);	//bufflag开始解码
		frames++;
		if (err != ERR_MP3_NONE)	//错误处理
		{
			switch (err)
			{
				case ERR_MP3_INDATA_UNDERFLOW:
					result = f_read(&file, inputbuf, INPUTBUF_SIZE, &bw);
					read_ptr = inputbuf;
					bytes_left = bw;
					break;		
				case ERR_MP3_MAINDATA_UNDERFLOW:
					/* do nothing - next call to decode will provide more mainData */
					break;		
				default:
					// 跳过此帧
					if (bytes_left > 0)
					{
						bytes_left --;
						read_ptr ++;
					}	
					break;
			}
			Isread = 1;
		}
		else	//解码无错误，准备把数据输出到PCM
		{
			MP3GetLastFrameInfo(Mp3Decoder, &Mp3FrameInfo);	//获取解码信息				
			/* 输出到DAC */
			outputSamps = Mp3FrameInfo.outputSamps;	//PCM数据个数
			if (outputSamps > 0)
			{
				if (Mp3FrameInfo.nChans == 1)	//单声道
				{
					//单声道数据需要复制一份到另一个声道
					for (i = outputSamps - 1; i >= 0; i--)
					{
						outbuffer[bufflag][i * 2] = outbuffer[bufflag][i];
						outbuffer[bufflag][i * 2 + 1] = outbuffer[bufflag][i];
					}
					outputSamps *= 2;
				}
			}
			
			/* 根据解码信息设置采样率 */
			if (Mp3FrameInfo.samprate != mp3player.ucFreq)	//采样率 
			{
				mp3player.ucFreq = Mp3FrameInfo.samprate;
				printf("[MP3] %dHz %dkbps %dch\r\n", 
				       Mp3FrameInfo.samprate, Mp3FrameInfo.bitrate/1000, Mp3FrameInfo.nChans);
				
				if (mp3player.ucFreq >= I2S_AUDIOFREQ_DEFAULT)	//正常的帧,每次都要改速率
				{
					Isread = 0;  /* 重置标志,等待DMA完成第1帧 */
					bufflag = 0;  /* 重置bufflag,第1帧在buffer[0] */
					
					I2Sx_Mode_Config(I2S_STANDARD_PHILIPS, I2S_DATAFORMAT_16B, mp3player.ucFreq);
					I2Sx_TX_DMA_Init((uint32_t)&outbuffer[0], (uint32_t)&outbuffer[1], outputSamps);
					I2S_Play_Start();
				}
			}
		}//else 解码正常
		
		if (f_eof(&file)) 	//mp3文件读取完成，退出
		{
			printf("[MP3] EOF reached (fptr=%lu, fsize=%lu)\r\n", f_tell(&file), f_size(&file));
			break;
		}	
		
		/* 等待DMA传输完成 */
		if (frames <= 5) {
			printf("[MP3] Waiting for DMA (Isread=%d, bufflag=%d)...\r\n", Isread, bufflag);
		}
		/* 等待DMA传输完成 */
		uint32_t timeout = 0;
		while (Isread == 0)
		{
			/* 在FreeRTOS环境下让出CPU，避免忙等待 */
			vTaskDelay(1);  /* 延迟1个tick (~1ms) */
			timeout++;
			if (timeout > 5000) {  /* 5000 ticks = ~5秒超时 */
				printf("[MP3] DMA timeout!\r\n");
				mp3player.ucStatus = STA_IDLE;
				break;
			}
		}
		Isread = 0;
	}
	I2S_Stop();
	mp3player.ucStatus = STA_IDLE;
	MP3FreeDecoder(Mp3Decoder);
	f_close(&file);	
	printf("[MP3] Playback finished\r\n");
}

/**
  * @brief  DMA 传输完成回调（参考原版工程）
  * @param  无
  * @retval 无
  */
void MP3Player_I2S_DMA_TX_Callback(void)
{
	uint8_t ct;
	
	/* 获取DMA当前目标(CT位): 0=Memory0, 1=Memory1 */
	ct = I2S_DMA_Get_CurrentTarget();
	
	if (ct == 1)  /* DMA正在使用Memory1 */
	{
		bufflag = 0;  /* CPU写入Memory0 */
	}
	else  /* DMA正在使用Memory0 */
	{
		bufflag = 1;  /* CPU写入Memory1 */
	}
	
	Isread = 1;
}

/**
  * @brief   生成正弦波测试音调
  * @param   freq_hz: 音调频率(Hz)，推荐440Hz(A4标准音)
  * @param   duration_sec: 播放时长(秒)
  * @retval  无
  * @note    用于硬件测试，验证音频输出通路是否正常
  *
  * 测试流程:
  * 1. 初始化WM8978音频编解码芯片
  * 2. 配置I2S接口为16bit、44.1kHz、立体声
  * 3. 生成正弦波样本填充到双缓冲区
  * 4. 通过DMA和I2S输出到耳机/音箱
  */
void mp3TestTone(uint32_t freq_hz, uint8_t duration_sec)
{
	/* 采样率固定44.1kHz */
	const uint32_t SAMPLE_RATE = 44100;
	/* 每个缓冲区的样本数 */
	const uint32_t SAMPLES_PER_BUFFER = MP3BUFFER_SIZE;
	/* 总样本数 = 采样率 * 时长 */
	uint32_t total_samples = SAMPLE_RATE * duration_sec;
	uint32_t sample_index = 0;
	
	printf("[TONE] ========================================\r\n");
	printf("[TONE] MP3 Test Tone Generator\r\n");
	printf("[TONE] Frequency: %lu Hz\r\n", freq_hz);
	printf("[TONE] Duration: %u seconds\r\n", duration_sec);
	printf("[TONE] Sample Rate: %lu Hz\r\n", SAMPLE_RATE);
	printf("[TONE] Total Samples: %lu\r\n", total_samples);
	printf("[TONE] ========================================\r\n");
	
	/* 初始化播放参数 */
	mp3player.ucFreq = SAMPLE_RATE;
	mp3player.ucStatus = STA_IDLE;
	mp3player.ucVolume = 40;
	
	/* 初始化WM8978音频编解码芯片 */
	printf("[TONE] Resetting WM8978...\r\n");
	delay_ms(10);
	wm8978_Reset();
	
	printf("[TONE] Configuring WM8978 audio path (DAC -> Headphone)...\r\n");
	wm8978_CfgAudioPath(DAC_ON, EAR_LEFT_ON | EAR_RIGHT_ON);
	
	printf("[TONE] Setting volume to %u...\r\n", mp3player.ucVolume);
	wm8978_SetOUT1Volume(mp3player.ucVolume);
	
	printf("[TONE] Unmuting output...\r\n");
	wm8978_OutMute(0);
	
	printf("[TONE] Configuring I2S interface (Philips, 16bit, 44.1kHz)...\r\n");
	wm8978_CfgAudioIF(I2S_STANDARD_PHILIPS, 16);
	
	/* 初始化I2S和DMA */
	printf("[TONE] Initializing I2S and DMA...\r\n");
	I2S_Stop();
	I2S_GPIO_Config();
	I2Sx_Mode_Config(I2S_STANDARD_PHILIPS, I2S_DATAFORMAT_16B, SAMPLE_RATE);
	I2S_DMA_TX_Callback = MP3Player_I2S_DMA_TX_Callback;
	I2Sx_TX_DMA_Init((uint32_t)&outbuffer[0], (uint32_t)&outbuffer[1], SAMPLES_PER_BUFFER);
	
	printf("[TONE] Starting I2S playback...\r\n");
	I2S_Play_Start();
	
	/* 初始化缓冲标记 */
	bufflag = 0;
	Isread = 0;
	mp3player.ucStatus = STA_PLAYING;
	
	printf("[TONE] Generating and playing sine wave...\r\n");
	
	/* 生成并输出正弦波 */
	while (sample_index < total_samples && mp3player.ucStatus == STA_PLAYING)
	{
		/* 计算本次要生成的样本数 */
		uint32_t samples_to_generate = (total_samples - sample_index) > SAMPLES_PER_BUFFER
		                                ? SAMPLES_PER_BUFFER
		                                : (total_samples - sample_index);
		
		/* 生成正弦波到当前缓冲区 */
		for (uint32_t i = 0; i < samples_to_generate; i++)
		{
			/* 计算相位 (0 ~ 2π) */
			/* phase = 2π * freq_hz * sample_index / SAMPLE_RATE */
			double phase = 2.0 * 3.14159265359 * freq_hz * (sample_index + i) / SAMPLE_RATE;
			
			/* 生成正弦波值 (-1.0 ~ 1.0) */
			/* 16bit PCM: -32768 ~ 32767 */
			double sin_value = sin(phase);
			short pcm_sample = (short)(sin_value * 32767 * 0.8); /* 乘以0.8避免饱和 */
			
			/* 立体声：左右声道使用相同数据 */
			outbuffer[bufflag][i * 2]     = pcm_sample;  /* 左声道 */
			outbuffer[bufflag][i * 2 + 1] = pcm_sample;  /* 右声道 */
		}
		
		/* 剩余样本用0填充 */
		for (uint32_t i = samples_to_generate; i < SAMPLES_PER_BUFFER; i++)
		{
			outbuffer[bufflag][i * 2]     = 0;
			outbuffer[bufflag][i * 2 + 1] = 0;
		}
		
		sample_index += samples_to_generate;
		
		/* 等待DMA完成当前缓冲区的传输，然后切换缓冲区 */
		while (Isread == 0)
		{
			/* 如果超时则退出 */
			delay_ms(1);
		}
		Isread = 0;
		
		/* 打印进度 */
		uint32_t percent = (sample_index * 100) / total_samples;
		if (percent % 25 == 0)
		{
			printf("[TONE] Progress: %lu%% (%lu/%lu samples)\r\n", 
			       percent, sample_index, total_samples);
		}
	}
	
	printf("[TONE] Tone generation complete!\r\n");
	printf("[TONE] Stopping I2S playback...\r\n");
	
	/* 停止播放 */
	I2S_Stop();
	mp3player.ucStatus = STA_IDLE;
	
	printf("[TONE] ========================================\r\n");
	printf("[TONE] Test Tone Finished\r\n");
	printf("[TONE] ========================================\r\n");
}


/***************************** (END OF FILE) *********************************/
