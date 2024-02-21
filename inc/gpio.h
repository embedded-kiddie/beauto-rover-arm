/*===============================================================================
 Name        : ioport.h
 Author      : $(author)
 Version     :
 CPU type    : ARM Cortex-M3 LPC1343
 Copyright   : $(copyright)
 Description : General Purpose I/O definitions
===============================================================================*/
#ifndef	_IOPORT_H_
#define	_IOPORT_H_

// Cortex Microcontroller Software Interface Standard
#ifdef __USE_CMSIS
#include "LPC13xx.h"
#endif

/*----------------------------------------------------------------------
 * GPIO functions
 *----------------------------------------------------------------------*/
#if		1
#define	SetGpioDir(p, b, d)	{if((d)){(p)->DIR |=(1<<(b));}else{(p)->DIR &= ~(1<<(b));}}
#define	SetGpioBit(p, b, v)	{(p)->MASKED_ACCESS[(1<<(b))] = ((v)<<(b));}
#define	GetGpioBit(p, b)		((p)->MASKED_ACCESS[(1<<(b))])
#else
extern void SetGpioDir(__IO LPC_GPIO_TypeDef* port, uint32_t bit, uint32_t dir);
extern void SetGpioBit(__IO LPC_GPIO_TypeDef* port, uint32_t bit, uint32_t val);
extern unsigned char GetGpioBit(__IO LPC_GPIO_TypeDef* port, uint32_t bit);
#endif

/*----------------------------------------------------------------------
 * GPIOの初期化
 *----------------------------------------------------------------------*/
extern void GPIO_INIT(void);

/*----------------------------------------------------------------------
 * GPIO0のビット割り付け
 * PIO0_1:SW1
 * PIO0_3：LED1（橙）
 * PIO0_7：LED2（緑）
 *----------------------------------------------------------------------*/
#define	GPIO_BIT_SW1	(1)
#define	GPIO_BIT_LED1	(3)
#define	GPIO_BIT_LED2	(7)

/*----------------------------------------------------------------------
 * LEDの点灯／消灯
 *----------------------------------------------------------------------*/
#define LED1		(1)		// LED1（橙）の点灯データ
#define LED2		(2)		// LED2（緑）の点灯データ
#define	LED1_LED2	(3)		// LED1、2の点灯データ
#define	LED_ON		(3)		// LED1、2の点灯データ
#define LED_OFF		(0)		// LED1、2の消灯データ

extern void LED(unsigned char led);
extern void LED_TOGGLE(unsigned char led);
extern void LED_FLUSH(unsigned long msec);
extern void LED_BLINK(unsigned long msec);

/*----------------------------------------------------------------------
 * プッシュスイッチの状態
 *----------------------------------------------------------------------*/
// PIO0_1：SW1
#define SW_OFF	(0)		// SWはOFF状態のまま（'H'）
#define SW_ON	(1)		// SWはON→OFFに変化（'L'）
#define SW_HOLD	(2)		// SWは指定時間以上ON→OFFに変化

/*----------------------------------------------------------------------
 * スイッチ状態（low active）の監視
 *----------------------------------------------------------------------*/
extern int SW_SCAN(void);		// SWの状態をスキャンする
extern int SW_CLICK(void);		// SWをスキャンしON→OFFの立上がりを検知する
extern int SW_STANDBY(void);	// SWが押されてから離されるまで待機する
extern void SW_WATCH(void (*f)(void)); // 割込みの監視と処理関数の登録

/*----------------------------------------------------------------------
 * SWが押されるまで待機し、指定時間長押しされた場合にONを返す
 *----------------------------------------------------------------------*/
extern int SW_CLICK_HOLD(unsigned long msec);

/*----------------------------------------------------------------------
 * 指定の整数値をLEDに出力する
 *----------------------------------------------------------------------*/
extern void SHOW_VALUE(long val);

#ifdef	EXAMPLE
/*===============================================================================
 * 汎用I/Oポートの動作確認
 * - スイッチ監視とLED点滅
 *===============================================================================*/
extern void GPIO_EXAMPLE(void);
#endif // EXAMPLE

#endif // _IOPORT_H_
