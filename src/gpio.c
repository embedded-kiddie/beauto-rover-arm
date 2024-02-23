/*===============================================================================
 * Name        : gpio.c
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
#ifndef	gpioSetDir
void gpioSetDir(__IO LPC_GPIO_TypeDef* port, uint32_t bit, uint32_t dir)
{
	if (dir) {
		port->DIR |= (1 << bit);	// 出力ポートに設定
	}
	else {
		port->DIR &= ~(1 << bit);	// 入力ポートに設定
	}
}
#endif // gpioSetDir

/*----------------------------------------------------------------------
 * 9.5.1　Write/read data operations (Masked write/read operation)
 *----------------------------------------------------------------------*/
#ifndef	gpioSetBit
void gpioSetBit(__IO LPC_GPIO_TypeDef* port, uint32_t bit, uint32_t val)
{
	port->MASKED_ACCESS[(1 << bit)] = (val << bit);
}
#endif // gpioSetBit

#ifndef	gpioGetBit
unsigned char gpioGetBit(__IO LPC_GPIO_TypeDef* port, uint32_t bit)
{
	return port->MASKED_ACCESS[(1 << bit)];
}
#endif // gpioGetBit

/*----------------------------------------------------------------------
 * 割込み処理用テーブル
 *----------------------------------------------------------------------*/
static GPIO_IRQ_t irqTable[4] = {
	{LPC_GPIO0, EINT0_IRQn, 0, 0},
	{LPC_GPIO1, EINT1_IRQn, 0, 0},
	{LPC_GPIO2, EINT2_IRQn, 0, 0},
	{LPC_GPIO3, EINT3_IRQn, 0, 0},
};

/*----------------------------------------------------------------------
 * 割り込みによるポートの監視設定
 * - Note: 呼び出し前に指定ポートのピンを「入力」に設定すること
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
void gpioSetInterrupt(uint32_t portNo, uint32_t pin, uint8_t sense, uint8_t event, void (*f)(void)) {
	__IO LPC_GPIO_TypeDef* port = irqTable[portNo].port;

	// 監視するビットと実行する関数を登録
	irqTable[portNo].pin = pin;
	irqTable[portNo].function = f;

	// 9.4.3 GPIO interrupt sense register (GPIO0IS)
	// 0 = Interrupt on pin PIOn_x is configured as edge sensitive.
	port->IS &= ~(1 << pin);

	// 9.4.4 GPIO interrupt both edges sense register (GPIO0IBE)
	// 0 = Interrupt on pin PIOn_x is controlled through register GPIOIEV.
	port->IBE &= ~(1 << pin);

	// 9.4.5 GPIO interrupt event register (GPIO0IEV)
	// 1 = Depending on setting in register GPIOIS, rising edges
	// or HIGH level on pin PIOn_x trigger an interrupt.
	port->IEV |= (1 << pin);

	// 9.4.6 GPIO interrupt mask register (GPIO0IE)
	// 1 = Interrupt on pin PIOn_x is not masked.
	port->IE |= (1 << pin);

	// 9.4.9 GPIO interrupt clear register (GPIO0IC)
	// 1 = Clears edge detection logic for pin PIOn_x.
	port->IC |= (1 << pin);

	// IRQn_Type is defined in LPC13xx.h
	NVIC_EnableIRQ(irqTable[portNo].irqNo);
}

/*----------------------------------------------------------------------
 * 外部割込みの処理
 *----------------------------------------------------------------------*/
static void irqHandler(int portNo) {
	__IO LPC_GPIO_TypeDef *port = irqTable[portNo].port;

	// 9.4.7 GPIO raw interrupt status register (GPIO0RIS)
	// 1 = Interrupt requirements met on PIOn_x.
	if (port->RIS & (1 << irqTable[portNo].pin)) {
		// チャタリングによる多重割り込みを防止する
		NVIC_DisableIRQ(irqTable[portNo].irqNo);

		// 登録された関数を呼び出す
		if (irqTable[portNo].function) {
			irqTable[portNo].function();
		}

		// 9.4.9 GPIO interrupt clear register (GPIO0IC)
		// 1 = Clears edge detection logic for pin PIOn_x.
		port->IC |= (1 << irqTable[portNo].pin);

		// Remark: The synchronizer between the GPIO and the NVIC blocks
		// causes a delay of 2 clocks. It is recommended to add two NOPs
		// after the clear of the interrupt edge detection logic before
		// the exit of the interrupt service routine.
		__NOP();
		__NOP();
	}
}

/*----------------------------------------------------------------------
 * 外部割込みハンドラ
 *----------------------------------------------------------------------*/
void PIOINT0_IRQHandler(void) { irqHandler(0); }
void PIOINT1_IRQHandler(void) { irqHandler(1); }
void PIOINT2_IRQHandler(void) { irqHandler(2); }
void PIOINT3_IRQHandler(void) { irqHandler(3); }

/*----------------------------------------------------------------------
 * GPIOの初期化
 *----------------------------------------------------------------------*/
void gpioInit(void) {
	gpioSetDir(LPC_GPIO0, GPIO_BIT_LED1, 1);	// PIO0 ビット3（LED1：橙）を出力に設定
	gpioSetDir(LPC_GPIO0, GPIO_BIT_LED2, 1);	// PIO0 ビット7（LED2：緑）を出力に設定
	gpioSetDir(LPC_GPIO0, GPIO_BIT_SW1,  0);	// PIO0 ビット1（SW1）を入力に設定
//	LPC_IOCON->PIO0_1 = ((0<<0) | (1<<3));		// PIO0 ビット1（SW1）をプルアップに設定 --> ハードウェアで対応済み
}

/*----------------------------------------------------------------------
 * LEDの点灯／消灯
 *----------------------------------------------------------------------*/
void ledOn(unsigned char led) {
	gpioSetBit(LPC_GPIO0, GPIO_BIT_LED1, !(led & LED1));	// Low active
	gpioSetBit(LPC_GPIO0, GPIO_BIT_LED2, !(led & LED2));	// Low active
}

void ledOff(unsigned char led) {
	gpioSetBit(LPC_GPIO0, GPIO_BIT_LED1, (led & LED1));	// Low active
	gpioSetBit(LPC_GPIO0, GPIO_BIT_LED2, (led & LED2));	// Low active
}

/*----------------------------------------------------------------------
 * LEDの点灯を反転する
 *----------------------------------------------------------------------*/
void ledToggle(unsigned char led) {
	if (led & LED1) {
		gpioSetBit(LPC_GPIO0, GPIO_BIT_LED1, !gpioGetBit(LPC_GPIO0, GPIO_BIT_LED1));
	}
	if (led & LED2) {
		gpioSetBit(LPC_GPIO0, GPIO_BIT_LED2, !gpioGetBit(LPC_GPIO0, GPIO_BIT_LED2));
	}
}

/*----------------------------------------------------------------------
 * 指定時間間隔でLEDをフラッシュさせる
 *----------------------------------------------------------------------*/
void ledFlush(unsigned long msec) {
	ledOn(LED1); timerWait(msec);
	ledOn(LED2); timerWait(msec);
}

/*----------------------------------------------------------------------
 * 指定時間間隔でLEDを点滅させる
 *----------------------------------------------------------------------*/
void ledBlink(unsigned long msec) {
	ledOn (LED1_LED2); timerWait(msec);
	ledOff(LED1_LED2); timerWait(msec);
}

/*----------------------------------------------------------------------
 * スイッチ状態（low active）をスキャンする
 *----------------------------------------------------------------------*/
#define WAIT_CHATTERING	50 // チャタリング除去の待機時間[msec]

int swScan(void) {
	while (1) {
		// PIO0_1： SW1
		unsigned char c = gpioGetBit(LPC_GPIO0, GPIO_BIT_SW1);

		// チャタリング除去
		timerWait(WAIT_CHATTERING);

		if (c == gpioGetBit(LPC_GPIO0, GPIO_BIT_SW1)) {
			// 'L'：押されている
			// 'H'：離されている
			return c ? SW_OFF : SW_ON;
		}
	}
}

/*----------------------------------------------------------------------
 * スイッチ状態（low active）をスキャンしON→OFFの立下りを検知する
 *----------------------------------------------------------------------*/
int swClick(void) {
	// スイッチが押されているかスキャンする
	if (swScan() == SW_ON) {
		// 押されている間はループ
		while (swScan() == SW_ON);

		return SW_ON;	// ONからOFFに変化した
	}
	else {
		return SW_OFF;	// OFFのままだった
	}
}

/*----------------------------------------------------------------------
 * スイッチが押されてから離されるまで待機する
 *----------------------------------------------------------------------*/
int swStandby(void) {
	// OFFの間は待機
	while (swClick() == SW_OFF) {
		__NOP();
	}

	return SW_ON;
}

/*----------------------------------------------------------------------
 * スイッチの立ち上がりエッジ（押されてから離された時）で実行する関数を登録する
 *----------------------------------------------------------------------*/
void swWatch(void (*f)(void)) {
	gpioSetInterrupt(0, GPIO_BIT_SW1, 0, 0, f);
}

/*----------------------------------------------------------------------
 * スイッチが短くクリックされたらONを、長押しされた場合はHOLDを返す
 *----------------------------------------------------------------------*/
int swClickHold(unsigned long msec) {
	unsigned long t;

	// 離されていればすぐにOFFを返す
	if (swScan() == SW_OFF) {
		return SW_OFF;
	}

	t = timerRead();

	// 押されている間は待機
	while (swScan() == SW_ON) {
		if (timerRead() > t + msec) {
			ledOff(LED1_LED2);
		}
	}

	// 指定時間以上押されていたらHOLDを返す
	return timerRead() > t + msec ? SW_HOLD : SW_ON;
}

/*----------------------------------------------------------------------
 * 指定された整数値をLEDに出力する
 *----------------------------------------------------------------------*/
#define	CLICK_HOLD_TIME	1000	// 長押し判定時間[sec]

void showValue(long val) {
	int i, loop;
	const MusicScore_t m[] = {{Do6, N16}, {Do5, N16}};

	while (swClick() == SW_OFF) {
		ledFlush(100);
	}
	ledOff(LED1_LED2);
	timerWait(500);

	while (1) {
		// スイッチを押すごとにLSBから2ビットずつLED1（上位）とLED2（下位）で値を表示する
		for (i = 0; i < sizeof(val) * 8; i += 2) {
			int v = (val >> i);
			// LED1: 上位ビット、LED2: 下位ビット
			ledOn((v & 0x02 ? LED1 : LED_OFF) | (v & 0x01 ? LED2: LED_OFF));
			playScore(m, 1, 180, 0);
			swStandby();
			timerWait(100);
		}

		// SW_OFF : スイッチが押されていなければLEDの点滅を繰り返す
		// SW_ON  : 短くクリックされたらループを抜け値の表示を繰り返す
		// SW_HOLD: 指定秒の長押し後、離されたら呼び出し元に戻る
		for (loop = 1; loop == 1;) {
			switch (swClickHold(CLICK_HOLD_TIME)) {
			  case SW_OFF:
				ledFlush(100);
				break;
			  case SW_ON:
				ledOff(LED1_LED2);
				timerWait(500);
				loop = 0;
				break;
			  case SW_HOLD:
			  default:
				ledOff(LED1_LED2);
				playScore(m, 2, 180, 0);
				timerWait(1000);
				return;
			}
		}
	}
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
void exitLoop(void) {
	loopFlag = 0;
	timerWaitCancel();
}

/*----------------------------------------------------------------------
 * 汎用I/Oポートの動作例
 *----------------------------------------------------------------------*/
void gpioExample(void) {
	timerInit();			// timerWait()
	gpioInit();				// swStandby()
	playInit();				// showValue() で短音を鳴らす

	ledOn(LED1|LED2);		// LED1とLED2を点灯させる
	swStandby();			// スイッチが押されるまで待機

	while (!swClick()) {	// クリックされたら（ON→OFF）ループを抜ける
		ledBlink(100);		// ON:100[msec] + OFF:100[msec]
	}

	swWatch(exitLoop);		// 割込み処理関数を登録し、スイッチ状態を監視する
	while (loopFlag) {		// スイッチの割り込みが検知されたらループを抜ける
		ledFlush(500);		// LED1:500[msec] + LED2:500[msec]
	}

	showValue(0xffff);		// スイッチを押すごとにLSBから2ビットずつLED1（上位）とLED2（下位）で値を表示する
}
#endif // EXAMPLE
