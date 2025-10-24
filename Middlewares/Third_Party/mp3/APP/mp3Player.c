/*
******************************************************************************
* @file    recorder.c
* @author  fire
* @version V1.0
* @date    2015-xx-xx
* @brief   WM8978�������ܲ���+mp3����
******************************************************************************
* @attention
*
* ʵ��ƽ̨:Ұ��  STM32 F407 ������  
* ��̳    :http://www.chuxue123.com
* �Ա�    :http://firestm32.taobao.com
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

/* �Ƽ�ʹ�����¸�ʽmp3�ļ���
 * �����ʣ�44100Hz
 * ��  ����2
 * �����ʣ�320kbps
 */

/* ===== Audio playback synchronization ===== */
// ÿ֡���������������int16 ����������˫��������MP3 MPEG1 Layer III �̶�Ϊ 1152/ch -> 2304 ����
#define MP3_SAMPS_PER_FRAME  2304

// ��ѡ��ÿ��"��д����"�ü�֡��������������1 �㹻��2 ���Ƚ���ռ RAM��
#define FRAMES_PER_BUF  1
#define BUF_SAMPS       (MP3_SAMPS_PER_FRAME * FRAMES_PER_BUF)

/* ������������Ƶ����ʱ�������������Ҫ������СΪ2304*16/8�ֽ�(16ΪPCM����Ϊ16λ)��
 * �������Ƕ���MP3BUFFER_SIZEΪ2304��ʵ�����������ΪMP3BUFFER_SIZE*2���ֽ�
 */
#define MP3BUFFER_SIZE  BUF_SAMPS		
#define INPUTBUF_SIZE   3000

// ����̨��־���أ������ж���Ĵ�ӡ��
#ifndef AUDIO_DEBUG
#define AUDIO_DEBUG 0
#endif
#if AUDIO_DEBUG
#define ALOG(...)    printf(__VA_ARGS__)
#else
#define ALOG(...)    do{}while(0)
#endif	

static HMP3Decoder		Mp3Decoder;			/* mp3������ָ��	*/
static MP3FrameInfo		Mp3FrameInfo;		/* mP3֡��Ϣ  */
static MP3_TYPE mp3player;         /* mp3�����豸 */

/* ��Щ������ԭ�湤���в���Ҫ�����Ƴ� */

/* �ɵı�־�����������Լ������д��룩 */
volatile uint8_t Isread=0;           /* DMA������ɱ�־(volatile��ֹ�������Ż�) */
static volatile uint8_t bufflag=0;   /* ���ݻ�����ѡ���־(volatile��ֹ�������Ż�) */

uint32_t led_delay=0;

uint8_t inputbuf[INPUTBUF_SIZE]={0};        /* �������뻺������1940�ֽ�Ϊ���MP3֡��С  */
static short outbuffer[2][MP3BUFFER_SIZE];  /* ���������������Ҳ��I2S�������ݣ�ʵ��ռ���ֽ�����RECBUFFER_SIZE*2 */

FIL file;											/* file objects */
FRESULT result; 
UINT bw;            					/* File R/W count */

/* �������ļ��ڵ��õĺ������� */
void MP3Player_I2S_DMA_TX_Callback(void);

/**
  * @brief   MP3��ʽ��Ƶ����������(�ο�ԭ�湤�̼�ʵ��)
  * @param  mp3file: MP3�ļ�·��
  * @retval ��
  */
void mp3PlayerDemo(const char *mp3file)
{
	uint8_t *read_ptr = inputbuf;  /* ʹ�þֲ�����,����static */
	uint32_t frames = 0;
	int err = 0, i = 0, outputSamps = 0;	
	int	read_offset = 0;		/* ��ƫ��ָ�� */
	int	bytes_left = 0;			/* ʣ���ֽ��� */
	
	mp3player.ucFreq = I2S_AUDIOFREQ_DEFAULT;
	mp3player.ucStatus = STA_IDLE;
	mp3player.ucVolume = 25;
	
	result = f_open(&file, mp3file, FA_READ);
	if (result != FR_OK)
	{
		printf("[MP3] Open failed: %s (err=%d)\r\n", mp3file, result);
		result = f_close(&file);
		return;	/* ֹͣ���� */
	}
	printf("[MP3] Playing: %s (%.2f KB)\r\n", mp3file, f_size(&file) / 1024.0f);
	
	//��ʼ��MP3������
	Mp3Decoder = MP3InitDecoder();	
	if (Mp3Decoder == 0)
	{
		printf("[MP3] Decoder init failed\r\n");
		return;	/* ֹͣ���� */
	}
	
	
	delay_ms(10);	/* �ӳ�һ��ʱ�䣬�ȴ�I2S�жϽ��� */
	wm8978_Reset();		/* ��λWM8978����λ״̬ */
	/* ����WM8978оƬ������ΪDAC�����Ϊ���� */
	wm8978_CfgAudioPath(DAC_ON, EAR_LEFT_ON | EAR_RIGHT_ON);

	/* ����������������ͬ���� */
	wm8978_SetOUT1Volume(mp3player.ucVolume);

	/* ����WM8978��Ƶ�ӿ�Ϊ�����ֱ�׼I2S�ӿڣ�16bit */
	wm8978_CfgAudioIF(I2S_STANDARD_PHILIPS, 16);
	
	/*  ��ʼ��������I2S���ο�ԭ�湤�̣�  */
	I2S_Stop();
	I2S_GPIO_Config();
	I2Sx_Mode_Config(I2S_STANDARD_PHILIPS, I2S_DATAFORMAT_16B, mp3player.ucFreq);	
	I2S_DMA_TX_Callback = MP3Player_I2S_DMA_TX_Callback;
	I2Sx_TX_DMA_Init((uint32_t)&outbuffer[0], (uint32_t)&outbuffer[1], MP3BUFFER_SIZE);	
	
	bufflag = 0;
	Isread = 0;
	
	mp3player.ucStatus = STA_PLAYING;		/* ����״̬ */
	result = f_read(&file, inputbuf, INPUTBUF_SIZE, &bw);
	if (result != FR_OK)
	{
		printf("[MP3] Read failed (err=%d)\r\n", result);
		MP3FreeDecoder(Mp3Decoder);
		return;
	}
	read_ptr = inputbuf;
	bytes_left = bw;
	
	/* ���û����־��������������Ĳ���״̬�� */
	bufflag = 0;
	
	/* ����������ѭ���壨ԭ�湤�̵ļ򵥷�ʽ�� */
	while (mp3player.ucStatus == STA_PLAYING)
	{
		read_offset = MP3FindSyncWord(read_ptr, bytes_left);	//Ѱ��֡ͬ�������ص�һ��ͬ���ֵ�λ��
		if (read_offset < 0)	//û���ҵ�ͬ����
		{
			result = f_read(&file, inputbuf, INPUTBUF_SIZE, &bw);
			if (result != FR_OK)
			{
				printf("[MP3] Read error: %d\r\n", result);
				break;
			}
			read_ptr = inputbuf;
			bytes_left = bw;
			continue;	//����ѭ��2���ص�ѭ��1	
		}
		read_ptr += read_offset;	//ƫ����ͬ���ֵ�λ��
		bytes_left -= read_offset;	//ͬ����֮������ݴ�С	
		if (bytes_left < 1024)		//��������
		{
			/* ע������ط���Ϊ���õ���DMA��ȡ������һ��Ҫ4�ֽڶ���  */
			i = (uint32_t)(bytes_left) & 3;	//�ж϶�����ֽ�
			if (i) i = 4 - i;	//��Ҫ������ֽ�
			memcpy(inputbuf + i, read_ptr, bytes_left);	//�Ӷ���λ�ÿ�ʼ����
			read_ptr = inputbuf + i;	//ָ�����ݶ���λ��
			result = f_read(&file, inputbuf + bytes_left + i, INPUTBUF_SIZE - bytes_left - i, &bw);//��������
			if (result != FR_OK)
			{
				printf("[MP3] Read error: %d\r\n", result);
				break;
			}
			bytes_left += bw;	//��Ч��������С
		}
		err = MP3Decode(Mp3Decoder, &read_ptr, &bytes_left, outbuffer[bufflag], 0);	//bufflag��ʼ����
		frames++;
		if (err != ERR_MP3_NONE)	//������
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
					// ������֡
					if (bytes_left > 0)
					{
						bytes_left --;
						read_ptr ++;
					}	
					break;
			}
			Isread = 1;
		}
		else	//�����޴���׼�������������PCM
		{
			MP3GetLastFrameInfo(Mp3Decoder, &Mp3FrameInfo);	//��ȡ������Ϣ				
			/* �����DAC */
			outputSamps = Mp3FrameInfo.outputSamps;	//PCM���ݸ���
			if (outputSamps > 0)
			{
				if (Mp3FrameInfo.nChans == 1)	//������
				{
					//������������Ҫ����һ�ݵ���һ������
					for (i = outputSamps - 1; i >= 0; i--)
					{
						outbuffer[bufflag][i * 2] = outbuffer[bufflag][i];
						outbuffer[bufflag][i * 2 + 1] = outbuffer[bufflag][i];
					}
					outputSamps *= 2;
				}
			}
			
			/* ���ݽ�����Ϣ���ò����� */
			if (Mp3FrameInfo.samprate != mp3player.ucFreq)	//������ 
			{
				mp3player.ucFreq = Mp3FrameInfo.samprate;
				printf("[MP3] %dHz %dkbps %dch\r\n", 
				       Mp3FrameInfo.samprate, Mp3FrameInfo.bitrate/1000, Mp3FrameInfo.nChans);
				
				if (mp3player.ucFreq >= I2S_AUDIOFREQ_DEFAULT)	//������֡,ÿ�ζ�Ҫ������
				{
					Isread = 0;  /* ���ñ�־,�ȴ�DMA��ɵ�1֡ */
					bufflag = 0;  /* ����bufflag,��1֡��buffer[0] */
					
					I2Sx_Mode_Config(I2S_STANDARD_PHILIPS, I2S_DATAFORMAT_16B, mp3player.ucFreq);
					I2Sx_TX_DMA_Init((uint32_t)&outbuffer[0], (uint32_t)&outbuffer[1], outputSamps);
					I2S_Play_Start();
				}
			}
		}//else ��������
		
		if (f_eof(&file)) 	//mp3�ļ���ȡ��ɣ��˳�
		{
			printf("[MP3] EOF reached (fptr=%lu, fsize=%lu)\r\n", f_tell(&file), f_size(&file));
			break;
		}	
		
		/* �ȴ�DMA������� */
		if (frames <= 5) {
			printf("[MP3] Waiting for DMA (Isread=%d, bufflag=%d)...\r\n", Isread, bufflag);
		}
		/* �ȴ�DMA������� */
		uint32_t timeout = 0;
		while (Isread == 0)
		{
			/* ��FreeRTOS�������ó�CPU������æ�ȴ� */
			vTaskDelay(1);  /* �ӳ�1��tick (~1ms) */
			timeout++;
			if (timeout > 5000) {  /* 5000 ticks = ~5�볬ʱ */
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
  * @brief  DMA ������ɻص����ο�ԭ�湤�̣�
  * @param  ��
  * @retval ��
  */
void MP3Player_I2S_DMA_TX_Callback(void)
{
	uint8_t ct;
	
	/* ��ȡDMA��ǰĿ��(CTλ): 0=Memory0, 1=Memory1 */
	ct = I2S_DMA_Get_CurrentTarget();
	
	if (ct == 1)  /* DMA����ʹ��Memory1 */
	{
		bufflag = 0;  /* CPUд��Memory0 */
	}
	else  /* DMA����ʹ��Memory0 */
	{
		bufflag = 1;  /* CPUд��Memory1 */
	}
	
	Isread = 1;
}

/**
  * @brief   �������Ҳ���������
  * @param   freq_hz: ����Ƶ��(Hz)���Ƽ�440Hz(A4��׼��)
  * @param   duration_sec: ����ʱ��(��)
  * @retval  ��
  * @note    ����Ӳ�����ԣ���֤��Ƶ���ͨ·�Ƿ�����
  *
  * ��������:
  * 1. ��ʼ��WM8978��Ƶ�����оƬ
  * 2. ����I2S�ӿ�Ϊ16bit��44.1kHz��������
  * 3. �������Ҳ�������䵽˫������
  * 4. ͨ��DMA��I2S���������/����
  */
void mp3TestTone(uint32_t freq_hz, uint8_t duration_sec)
{
	/* �����ʹ̶�44.1kHz */
	const uint32_t SAMPLE_RATE = 44100;
	/* ÿ���������������� */
	const uint32_t SAMPLES_PER_BUFFER = MP3BUFFER_SIZE;
	/* �������� = ������ * ʱ�� */
	uint32_t total_samples = SAMPLE_RATE * duration_sec;
	uint32_t sample_index = 0;
	
	printf("[TONE] ========================================\r\n");
	printf("[TONE] MP3 Test Tone Generator\r\n");
	printf("[TONE] Frequency: %lu Hz\r\n", freq_hz);
	printf("[TONE] Duration: %u seconds\r\n", duration_sec);
	printf("[TONE] Sample Rate: %lu Hz\r\n", SAMPLE_RATE);
	printf("[TONE] Total Samples: %lu\r\n", total_samples);
	printf("[TONE] ========================================\r\n");
	
	/* ��ʼ�����Ų��� */
	mp3player.ucFreq = SAMPLE_RATE;
	mp3player.ucStatus = STA_IDLE;
	mp3player.ucVolume = 40;
	
	/* ��ʼ��WM8978��Ƶ�����оƬ */
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
	
	/* ��ʼ��I2S��DMA */
	printf("[TONE] Initializing I2S and DMA...\r\n");
	I2S_Stop();
	I2S_GPIO_Config();
	I2Sx_Mode_Config(I2S_STANDARD_PHILIPS, I2S_DATAFORMAT_16B, SAMPLE_RATE);
	I2S_DMA_TX_Callback = MP3Player_I2S_DMA_TX_Callback;
	I2Sx_TX_DMA_Init((uint32_t)&outbuffer[0], (uint32_t)&outbuffer[1], SAMPLES_PER_BUFFER);
	
	printf("[TONE] Starting I2S playback...\r\n");
	I2S_Play_Start();
	
	/* ��ʼ�������� */
	bufflag = 0;
	Isread = 0;
	mp3player.ucStatus = STA_PLAYING;
	
	printf("[TONE] Generating and playing sine wave...\r\n");
	
	/* ���ɲ�������Ҳ� */
	while (sample_index < total_samples && mp3player.ucStatus == STA_PLAYING)
	{
		/* ���㱾��Ҫ���ɵ������� */
		uint32_t samples_to_generate = (total_samples - sample_index) > SAMPLES_PER_BUFFER
		                                ? SAMPLES_PER_BUFFER
		                                : (total_samples - sample_index);
		
		/* �������Ҳ�����ǰ������ */
		for (uint32_t i = 0; i < samples_to_generate; i++)
		{
			/* ������λ (0 ~ 2��) */
			/* phase = 2�� * freq_hz * sample_index / SAMPLE_RATE */
			double phase = 2.0 * 3.14159265359 * freq_hz * (sample_index + i) / SAMPLE_RATE;
			
			/* �������Ҳ�ֵ (-1.0 ~ 1.0) */
			/* 16bit PCM: -32768 ~ 32767 */
			double sin_value = sin(phase);
			short pcm_sample = (short)(sin_value * 32767 * 0.8); /* ����0.8���ⱥ�� */
			
			/* ����������������ʹ����ͬ���� */
			outbuffer[bufflag][i * 2]     = pcm_sample;  /* ������ */
			outbuffer[bufflag][i * 2 + 1] = pcm_sample;  /* ������ */
		}
		
		/* ʣ��������0��� */
		for (uint32_t i = samples_to_generate; i < SAMPLES_PER_BUFFER; i++)
		{
			outbuffer[bufflag][i * 2]     = 0;
			outbuffer[bufflag][i * 2 + 1] = 0;
		}
		
		sample_index += samples_to_generate;
		
		/* �ȴ�DMA��ɵ�ǰ�������Ĵ��䣬Ȼ���л������� */
		while (Isread == 0)
		{
			/* �����ʱ���˳� */
			delay_ms(1);
		}
		Isread = 0;
		
		/* ��ӡ���� */
		uint32_t percent = (sample_index * 100) / total_samples;
		if (percent % 25 == 0)
		{
			printf("[TONE] Progress: %lu%% (%lu/%lu samples)\r\n", 
			       percent, sample_index, total_samples);
		}
	}
	
	printf("[TONE] Tone generation complete!\r\n");
	printf("[TONE] Stopping I2S playback...\r\n");
	
	/* ֹͣ���� */
	I2S_Stop();
	mp3player.ucStatus = STA_IDLE;
	
	printf("[TONE] ========================================\r\n");
	printf("[TONE] Test Tone Finished\r\n");
	printf("[TONE] ========================================\r\n");
}


/***************************** (END OF FILE) *********************************/
