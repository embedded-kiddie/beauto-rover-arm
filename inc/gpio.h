/*===============================================================================
 Name        : gpio.h
 Author      : $(author)
 Version     :
 CPU type    : ARM Cortex-M3 LPC1343
 Copyright   : $(copyright)
 Description : General Purpose I/O definitions
===============================================================================*/
#ifndef	_GPIO_H_
#define	_GPIO_H_

// Cortex Microcontroller Software Interface Standard
#ifdef __USE_CMSIS
#include "LPC13xx.h"
#endif

/*----------------------------------------------------------------------
 * GPIO functions
 *----------------------------------------------------------------------*/
#if		1
#define	gpioSetDir(p, b, d)	{if((d)){(p)->DIR |=(1<<(b));}else{(p)->DIR &= ~(1<<(b));}}
#define	gpioSetBit(p, b, v)	{(p)->MASKED_ACCESS[(1<<(b))] = ((v)<<(b));}
#define	gpioGetBit(p, b)	((p)->MASKED_ACCESS[(1<<(b))])
#else
extern void gpioSetDir(__IO LPC_GPIO_TypeDef* port, uint32_t bit, uint32_t dir);
extern void gpioSetBit(__IO LPC_GPIO_TypeDef* port, uint32_t bit, uint32_t val);
extern unsigned char gpioGetBit(__IO LPC_GPIO_TypeDef* port, uint32_t bit);
#endif

/*----------------------------------------------------------------------
 * GPIO Interrupt Handler
 * 9.4.3 GPIO interrupt sense register
 * 9.4.4 GPIO interrupt both edges sense register
 * 9.4.5 GPIO interrupt event register
 *----------------------------------------------------------------------*/
#define	SENSITIVE_EDGE	0	// エッジで割り込みを検出
#define	SENSITIVE_LEVEL	1	// レベルで割り込みを検出
#define	EVENT_FALLING	0	// エッジの立下がりで割り込みを検出
#define	EVENT_RISING	1	// エッジの立上がりで割り込みを検出

extern void gpioSetInterrupt(uint32_t portNo, uint32_t pin, uint8_t sense, uint8_t event, void (*f)(void));

/*----------------------------------------------------------------------
 * 割込み処理用テーブル
 *----------------------------------------------------------------------*/
typedef struct {
	__IO LPC_GPIO_TypeDef* port;	// GPIOポートアドレス
	IRQn_Type irqNo;				// 割込みNo
	uint32_t pin;					// 割込みを監視するピン
	void (*function)(void);			// 割込み発生時に実行する関数
} GPIO_IRQ_t;

/*----------------------------------------------------------------------
 * GPIOの初期化
 *----------------------------------------------------------------------*/
extern void gpioInit(void);

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

extern void ledOn(unsigned char led);
extern void ledOff(unsigned char led);
extern void ledToggle(unsigned char led);
extern void ledFlush(unsigned long msec);
extern void ledBlink(unsigned long msec);

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
extern int swScan(void);		// SWの状態をスキャンする
extern int swClick(void);		// SWをスキャンしON→OFFの立上がりを検知する
extern int swStandby(void);	// SWが押されてから離されるまで待機する
extern void swWatch(void (*f)(void)); // 割込みの監視と処理関数の登録

/*----------------------------------------------------------------------
 * SWが押されるまで待機し、指定時間長押しされた場合にONを返す
 *----------------------------------------------------------------------*/
extern int swClickHold(unsigned long msec);

/*----------------------------------------------------------------------
 * 指定の整数値をLEDに出力する
 *----------------------------------------------------------------------*/
extern void showValue(long val);

#ifdef	EXAMPLE
/*===============================================================================
 * 汎用I/Oポートの動作確認
 * - スイッチ監視とLED点滅
 *===============================================================================*/
extern void gpioExample(void);
#endif // EXAMPLE

#endif // _GPIO_H_
