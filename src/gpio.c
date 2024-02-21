/*===============================================================================
 * Name        : ioport.c
 * Author      : $(author)
 * Version     :
 * CPU type    : ARM Cortex-M3 LPC1343
 * Copyright   : $(copyright)
 * Description : General Purpose I/O functions
 *===============================================================================*/
// Cortex Microcontroller Software Interface Standard
#ifdef __USE_CMSIS
#include "LPC13xx.h"
#endif

#include "type.h"
#include "gpio.h"
#include "timer.h"
#include "play.h"

/*----------------------------------------------------------------------
 * 9.4.2　GPIO data direction register (GPIO0DIR)
 *----------------------------------------------------------------------*/
#ifndef	SetGpioDir
void SetGpioDir(__IO LPC_GPIO_TypeDef* port, uint32_t bit, uint32_t dir)
{
	if (dir) {
		port->DIR |= (1 << bit);	// 出力ポートに設定
	}
	else {
		port->DIR &= ~(1 << bit);	// 入力ポートに設定
	}
}
#endif // SetGpioDir

/*----------------------------------------------------------------------
 * 9.5.1　Write/read data operations (Masked write/read operation)
 *----------------------------------------------------------------------*/
#ifndef	SetGpioBit
void SetGpioBit(__IO LPC_GPIO_TypeDef* port, uint32_t bit, uint32_t val)
{
	port->MASKED_ACCESS[(1 << bit)] = (val << bit);
}
#endif // SetGpioBit

#ifndef	GetGpioBit
unsigned char GetGpioBit(__IO LPC_GPIO_TypeDef* port, uint32_t bit)
{
	return port->MASKED_ACCESS[(1 << bit)];
}
#endif // GetGpioBit

/*----------------------------------------------------------------------
 * GPIOの初期化
 *----------------------------------------------------------------------*/
void GPIO_INIT(void) {
	SetGpioDir(LPC_GPIO0, GPIO_BIT_LED1, 1);	// PIO0 ビット3（LED1：橙）を出力に設定
	SetGpioDir(LPC_GPIO0, GPIO_BIT_LED2, 1);	// PIO0 ビット7（LED2：緑）を出力に設定
	SetGpioDir(LPC_GPIO0, GPIO_BIT_SW1,  0);	// PIO0 ビット1（SW1）を入力に設定
//	LPC_IOCON->PIO0_1 = ((0<<0) | (1<<3));		// PIO0 ビット1（SW1）をプルアップに設定 --> ハードウェアで対応済み
}

/*----------------------------------------------------------------------
 * LEDの点灯／消灯
 *----------------------------------------------------------------------*/
void LED(unsigned char led) {
	SetGpioBit(LPC_GPIO0, GPIO_BIT_LED1, !(led & LED1));	// Low active
	SetGpioBit(LPC_GPIO0, GPIO_BIT_LED2, !(led & LED2));	// Low active
}

/*----------------------------------------------------------------------
 * LEDの点灯を反転する
 *----------------------------------------------------------------------*/
void LED_TOGGLE(unsigned char led) {
	if (led & LED1) {
		SetGpioBit(LPC_GPIO0, GPIO_BIT_LED1, !GetGpioBit(LPC_GPIO0, GPIO_BIT_LED1));
	}
	if (led & LED2) {
		SetGpioBit(LPC_GPIO0, GPIO_BIT_LED2, !GetGpioBit(LPC_GPIO0, GPIO_BIT_LED2));
	}
}

/*----------------------------------------------------------------------
 * 指定時間間隔でLEDをフラッシュさせる
 *----------------------------------------------------------------------*/
void LED_FLUSH(unsigned long msec) {
	LED(LED1); WAIT(msec);
	LED(LED2); WAIT(msec);
}

/*----------------------------------------------------------------------
 * 指定時間間隔でLEDを点滅させる
 *----------------------------------------------------------------------*/
void LED_BLINK(unsigned long msec) {
	LED(LED_ON ); WAIT(msec);
	LED(LED_OFF); WAIT(msec);
}

/*----------------------------------------------------------------------
 * スイッチ状態（low active）をスキャンする
 *----------------------------------------------------------------------*/
#define WAIT_CHATTERING	50 // チャタリング除去の待機時間[msec]

int SW_SCAN(void) {
	while (1) {
		// PIO0_1： SW1
		unsigned char c = GetGpioBit(LPC_GPIO0, GPIO_BIT_SW1);

		// チャタリング除去
		WAIT(WAIT_CHATTERING);

		if (c == GetGpioBit(LPC_GPIO0, GPIO_BIT_SW1)) {
			// 'L'：押されている
			// 'H'：離されている
			return c ? SW_OFF : SW_ON;
		}
	}
}

/*----------------------------------------------------------------------
 * スイッチ状態（low active）をスキャンしON→OFFの立下りを検知する
 *----------------------------------------------------------------------*/
int SW_CLICK(void) {
	// スイッチが押されているかスキャンする
	if (SW_SCAN() == SW_ON) {
		// 押されている間はループ
		while (SW_SCAN() == SW_ON);

		return SW_ON;	// ONからOFFに変化した
	}
	else {
		return SW_OFF;	// OFFのままだった
	}
}

/*----------------------------------------------------------------------
 * スイッチが押されてから離されるまで待機する
 *----------------------------------------------------------------------*/
int SW_STANDBY(void) {
	// OFFの間は待機
	while (SW_CLICK() == SW_OFF) {
		__NOP();
	}

	return SW_ON;
}

/*----------------------------------------------------------------------
 * スイッチが短くクリックされたらONを、長押しされた場合はHOLDを返す
 *----------------------------------------------------------------------*/
int SW_CLICK_HOLD(unsigned long msec) {
	unsigned long t;

	// 離されていればすぐにOFFを返す
	if (SW_SCAN() == SW_OFF) {
		return SW_OFF;
	}

	t = TIMER_READ();

	// 押されている間は待機
	while (SW_SCAN() == SW_ON) {
		if (TIMER_READ() > t + msec) {
			LED(LED_OFF);
		}
	}

	// 指定時間以上押されていたらHOLDを返す
	return TIMER_READ() > t + msec ? SW_HOLD : SW_ON;
}

/*----------------------------------------------------------------------
 * 指定された整数値をLEDに出力する
 *----------------------------------------------------------------------*/
#define	CLICK_HOLD_TIME	1000	// 長押し判定時間[sec]

void SHOW_VALUE(long val) {
	int i, loop;
	const MusicScore_t m[] = {{Do6, N16}, {Do5, N16}};

	while (SW_CLICK() == SW_OFF) {
		LED_FLUSH(100);
	}
	LED(LED_OFF);
	WAIT(500);

	while (1) {
		// スイッチを押すごとにLSBから2ビットずつLED1（上位）とLED2（下位）で値を表示する
		for (i = 0; i < sizeof(val) * 8; i += 2) {
			int v = (val >> i);
			// LED1: 上位ビット、LED2: 下位ビット
			LED((v & 0x02 ? LED1 : LED_OFF) | (v & 0x01 ? LED2: LED_OFF));
			PLAY(m, 1, 155, 0);
			SW_STANDBY();
			WAIT(100);
		}

		// SW_OFF : スイッチが押されていなければLEDの点滅を繰り返す
		// SW_ON  : 短くクリックされたらループを抜け値の表示を繰り返す
		// SW_HOLD: 指定秒の長押し後、離されたら呼び出し元に戻る
		for (loop = 1; loop == 1;) {
			switch (SW_CLICK_HOLD(CLICK_HOLD_TIME)) {
			  case SW_OFF:
				LED_FLUSH(100);
				break;
			  case SW_ON:
				LED(LED_OFF);
				WAIT(500);
				loop = 0;
				break;
			  case SW_HOLD:
			  default:
				LED(LED_OFF);
				PLAY(m, 2, 155, 0);
				WAIT(1000);
				return;
			}
		}
	}
}

/*----------------------------------------------------------------------
 * 割り込みによるスイッチの監視
 *
 * Definition General Purpose Input/Output (GPIO) in LPC13xx.h
 * typedef struct {
 *   ...
 *   __IO uint32_t DIR;  9.4.2 GPIO data direction register
 *   __IO uint32_t IS;   9.4.3 GPIO interrupt sense register
 *   __IO uint32_t IBE;  9.4.4 GPIO interrupt both edges sense register
 *   __IO uint32_t IEV;  9.4.5 GPIO interrupt event register
 *   __IO uint32_t IE;   9.4.6 GPIO interrupt mask register
 *   __IO uint32_t RIS;  9.4.7 GPIO raw interrupt status register
 *   __IO uint32_t MIS;  9.4.8 GPIO masked interrupt status register
 *   __IO uint32_t IC;   9.4.9 GPIO interrupt clear register
 * } LPC_GPIO_TypeDef;
 *----------------------------------------------------------------------*/
static void (*SW_IRQHandler)(void) = 0;

void PIOINT0_IRQHandler(void) {
	// 9.4.7 GPIO raw interrupt status register (GPIO0RIS)
	// 1 = Interrupt requirements met on PIOn_x.
	if (LPC_GPIO0->RIS & (1 << GPIO_BIT_SW1)) {
		NVIC_DisableIRQ(EINT0_IRQn); // チャタリングによる多重割り込みを防止する

		// 登録された関数を呼び出す
		if (SW_IRQHandler) {
			SW_IRQHandler();
		}

		// 9.4.9 GPIO interrupt clear register (GPIO0IC)
		// 1 = Clears edge detection logic for pin PIOn_x.
		LPC_GPIO0->IC |= (1 << GPIO_BIT_SW1);

		// Remark: The synchronizer between the GPIO and the NVIC blocks
		// causes a delay of 2 clocks. It is recommended to add two NOPs
		// after the clear of the interrupt edge detection logic before
		// the exit of the interrupt service routine.
		__NOP();
		__NOP();
	}
}

/*----------------------------------------------------------------------
 * スイッチの立ち上がりエッジ（押されてから離された時）で実行する関数を登録する
 *----------------------------------------------------------------------*/
void SW_WATCH(void (*f)(void)) {
	// 実行する関数を登録
	SW_IRQHandler = f;

	// 9.4.3 GPIO interrupt sense register (GPIO0IS)
	// 0 = Interrupt on pin PIOn_x is configured as edge sensitive.
	LPC_GPIO0->IS &= ~(1 << GPIO_BIT_SW1);

	// 9.4.4 GPIO interrupt both edges sense register (GPIO0IBE)
	// 0 = Interrupt on pin PIOn_x is controlled through register GPIOIEV.
	LPC_GPIO0->IBE &= ~(1 << GPIO_BIT_SW1);

	// 9.4.5 GPIO interrupt event register (GPIO0IEV)
	// 1 = Depending on setting in register GPIOIS, rising edges
	// or HIGH level on pin PIOn_x trigger an interrupt.
	LPC_GPIO0->IEV |= (1 << GPIO_BIT_SW1);

	// 9.4.6 GPIO interrupt mask register (GPIO0IE)
	// 1 = Interrupt on pin PIOn_x is not masked.
	LPC_GPIO0->IE |= (1 << GPIO_BIT_SW1);

	// 9.4.9 GPIO interrupt clear register (GPIO0IC)
	// 1 = Clears edge detection logic for pin PIOn_x.
	LPC_GPIO0->IC |= (1 << GPIO_BIT_SW1);

	// EINT0_IRQn: IRQn_Type is defined in LPC13xx.h
	NVIC_EnableIRQ(EINT0_IRQn);
}

#ifdef	EXAMPLE
/*===============================================================================
 * 汎用I/Oポートの動作確認
 * - スイッチの監視とLEDの点滅を確認する
 *===============================================================================*/
#include "type.h"
#include "timer.h"
#include "gpio.h"

/*----------------------------------------------------------------------
 * whileループを抜ける処理（割り込みハンドラから呼び出される想定）
 *----------------------------------------------------------------------*/
volatile static char loopFlag = 1;
void ExitLoop(void) {
	loopFlag = 0;
	WAIT_CANCEL();
}

/*----------------------------------------------------------------------
 * 汎用I/Oポートの動作例
 *----------------------------------------------------------------------*/
void GPIO_EXAMPLE(void) {
	TIMER_INIT();			// WAIT()
	GPIO_INIT();			// SW_STANDBY()
	PLAY_INIT();			// SHOW_VALUE() で短音を鳴らす

	LED(LED1|LED2);			// LED1とLED2を点灯させる
	SW_STANDBY();			// スイッチが押されるまで待機

	while (!SW_CLICK()) {	// クリックされたら（ON→OFF）ループを抜ける
		LED_BLINK(100);		// ON:100[msec] + OFF:100[msec]
	}

	SW_WATCH(ExitLoop);		// 割込み処理関数を登録し、スイッチ状態を監視する
	while (loopFlag) {		// スイッチの割り込みが検知されたらループを抜ける
		LED_FLUSH(500);		// LED1:500[msec] + LED2:500[msec]
	}

	SHOW_VALUE(0xffff);		// スイッチを押すごとにLSBから2ビットずつLED1（上位）とLED2（下位）で値を表示する
}
#endif // EXAMPLE
