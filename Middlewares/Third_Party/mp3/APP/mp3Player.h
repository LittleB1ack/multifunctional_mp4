#ifndef __MP3PLAYER_H__
#define __MP3PLAYER_H__

#include <inttypes.h>
#include "./led/led.h" 


/* ״̬ */
enum
{
	STA_IDLE = 0,	/* ����״̬ */
	STA_PLAYING,	/* ����״̬ */
	STA_ERR,			/*  error  */
};

typedef struct
{
	uint8_t ucVolume;			/* ��ǰ�������� */
	uint8_t ucStatus;			/* ״̬��0��ʾ������1��ʾ�����У�2 ���� */	
	uint32_t ucFreq;			/* ����Ƶ�� */
}MP3_TYPE;	

void mp3PlayerDemo(const char *mp3file);
void mp3TestTone(uint32_t freq_hz, uint8_t duration_sec);  /* �������� */

void MP3Player_I2S_DMA_TX_Callback(void);
#endif  /* __MP3PLAYER_H__   */

