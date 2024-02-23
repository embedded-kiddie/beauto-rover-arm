/*===============================================================================
 * Name        : wdt.c
 * Author      : $(author)
 * Version     :
 * CPU type    : ARM Cortex-M3 LPC1343
 * Copyright   : $(copyright)
 * Description : WatchDog Timer functions
 *===============================================================================*/
// Cortex Microcontroller Software Interface Standard
#ifdef __USE_CMSIS
#include "LPC13xx.h"
#endif

#include "type.h"
#include "clk.h"
#include "pmu.h"
#include "wdt.h"

/*----------------------------------------------------------------------
 * WatchDog Timer clock configuration
 * Note: 18.5 Description より
 *	WDT_OSC_FREQ = 0.6[MHz], WDT_OSC_DIV = 64 で設定可能な最小の
 *	ウォッチドッグ周期は 0xFF × 4 ÷ (0.6[MHz] ÷ 64) ≒ 110[msec] となる
 *----------------------------------------------------------------------*/
#define	WDT_OSC_FREQ	(WDT_FREQSEL_t)(1)	// WDT_FREQSEL_0_60 (0.6MHz)
#define	WDT_OSC_DIV		64					// 2 × (1 + DIVSEL) = 64
#define	WDT_CLKSRC		CLK_SRC_WDTOSC		// watchdog oscillator

/*----------------------------------------------------------------------
 * WatchDog Timer debug configuration
 *----------------------------------------------------------------------*/
#define	WDT_RESET_CPU	0	// 1 = a watchdog time-out will cause a chip reset
#define	WDT_FEED_ON		0	// 1 = WDFEED will be updated in wdtFeed()
#define	WDT_DEBUG		0
#if		WDT_DEBUG
#include <stdio.h>
#include "sci.h"
#else
#define	sciPrintf(...)
#endif

/*----------------------------------------------------------------------
 * タイムアウト時に WDT_IRQHandler() で実行される関数
 *----------------------------------------------------------------------*/
static void (*wdtTimeoutHandler)(void) = 0;

/*----------------------------------------------------------------------
 * WatchDog Timer - 初期化
 *----------------------------------------------------------------------*/
void wdtInit(void) {
	// Get Core Clock Frequency
	// SystemCoreClockUpdate() is defined in CMSIS_CORE_LPC13xx\src\system_LPC13xx.c
	SystemCoreClockUpdate();

	// 3.5.18 System AHB clock control register (SYSAHBCLKCTRL)
	// Bit 15 (WDT): 1 = enable
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<15);

	wdtStop();

	// Keep Watchdog oscillator powered in sleep mode and deep-sleep mode
	// 18.6 Clocking and power control
	pmuAddPowerDownRun(PDRUNCFG_WDTOSC); // defined in Power Management Unit

	// Setup Watchdog oscillator before attaching to the clock source
	// 3.5.8 Watchdog oscillator control register (WDTOSCCTRL)
	// ex) wdt_osc_clk = Fclkana 0.6[MHz] / 64 = 9.375[KHz]
	// --> Actual watchdog interval = wdt_osc_clk ÷ 4 (fixed pre-scaler)
	clkSetWDTClock(WDT_OSC_FREQ, WDT_OSC_DIV);

	// Select watchdog oscillator for WDT clock source
	clkSwitchWDTClockSrc(WDT_CLKSRC);
}

/*----------------------------------------------------------------------
 * WatchDog Timer - タイムアウト[msec]の設定
 *----------------------------------------------------------------------*/
void wdtTimeout(unsigned long msec, void (*f)(void)) {
	unsigned long wdtclock;

	// Actual watchdog interval = wdt_osc_clk ÷ 4 (fixed pre-scaler)
	wdtclock = clkGetWDTClock() / 4;
	wdtclock *= msec / 1000;

	// 18.7.2 Watchdog Timer Constant register (WDTC)
	LPC_WDT->TC = MAX(0xFF, MIN(wdtclock, 0x00FFFFFFUL));

	// タイムアウト時に実行する関数を登録
	wdtTimeoutHandler = f;
}

/*----------------------------------------------------------------------
 * WatchDog Timer - ウォッチドッグの更新
 *----------------------------------------------------------------------*/
void wdtFeed(void) {
#if WDT_FEED_ON
	// 18.7.3 Watchdog Feed register (WDFEED)
	// Bit 7:0 (FEED) Feed value should be 0xAA followed by 0x55
	LPC_WDT->FEED = 0xAA;
	LPC_WDT->FEED = 0x55;
#endif
}

/*----------------------------------------------------------------------
 * WatchDog Timer - ウォッチドッグの更新
 *----------------------------------------------------------------------*/
void wdtFeedSysTick(unsigned long msec) {
	unsigned long mainclock;

	// CPU main clock
	mainclock = clkGetMainClock();

	sciPrintf("Main clock frequency = %d\r\n", mainclock);

	// SysTick_Config() is defined in  CMSIS_CORE_LPC13xx/inc/core_cm3.h
	mainclock *= msec / 1000;
	SysTick_Config(MAX(2, MIN(mainclock, 0x00FFFFFFUL)));
}

/*----------------------------------------------------------------------
 * WatchDog Timer - ウォッチドッグの停止
 *----------------------------------------------------------------------*/
void wdtStop(void) {
	// Clear and enable watchdog interrupt
	NVIC_DisableIRQ(WDT_IRQn);

	// Clear Watchdog time-out flag
	wdtClearTimeoutFlag();

	// 18.7.1 Watchdog Mode register (WDMOD)
	// 18.7.2 Watchdog Timer Constant register (WDTC)
	LPC_WDT->MOD = 0;		// Bit 0 (WDEN) Watchdog enable bit = disable
	LPC_WDT->TC  = 0xFF;	// Bit 23:0 (COUNT) Watchdog time-out interval

	// Stop SysTick interrupt
	// 17.6.1 System Timer Control and status register (CTRL)
	// Bit 0 (ENABLE): 0 = the counter is disabled
	// Bit 1 (TICKINT): 0 = the System Tick interrupt is disabled
	// Bit 2 (CLKSOURCE): 1 = the system clock 0 (CPU) clock, 0 = system tick clock divider (SYSTICKDIV)
	if (SysTick->CTRL & (1<<0)) {
		SysTick->CTRL &= (1<<2);
	}
}

/*----------------------------------------------------------------------
 * WatchDog Timer - ウォッチドッグの開始
 *----------------------------------------------------------------------*/
void wdtStart(void) {
	// Clear Watchdog time-out flag
	wdtClearTimeoutFlag();

	// Clear and enable watchdog interrupt
	NVIC_ClearPendingIRQ(WDT_IRQn);
	NVIC_EnableIRQ(WDT_IRQn);

	// 18.7.1 Watchdog Mode register (WDMOD)
	// Bit 0 (WDEN) Watchdog enable bit = 1
	// Bit 1 (WDRESET) Watchdog reset enable bit = WDT_RESET_CPU
	LPC_WDT->MOD |= ((1<<0) | (WDT_RESET_CPU << 1));

	// Start Watchdog when WDEN is enabled in WDMOD register
	// 18.7.3 Watchdog Feed register (WDFEED)
	// Bit 7:0 (FEED) Feed value should be 0xAA followed by 0x55
	// Note: For test purpose, start directly instead of calling wdtFeed()
	LPC_WDT->FEED = 0xAA;
	LPC_WDT->FEED = 0x55;
}

/*----------------------------------------------------------------------
 * SysTickタイマーによる周期的なウォッチドッグの更新
 *----------------------------------------------------------------------*/
void SysTick_Handler(void)
{
	wdtFeed();

	sciPrintf("SysTick_Handler()\r\n");
}

/*----------------------------------------------------------------------
 * ウォッチドッグタイムアウト時の割込みハンドラ
 *----------------------------------------------------------------------*/
void WDT_IRQHandler(void) {
	// 18.7.1 Watchdog Mode register (WDMOD)
	unsigned long mode = LPC_WDT->MOD;

	// 12 = Watchdog interrupt flag + Watchdog time-out flag
	//  9 = Watchdog interrupt flag + Watchdog enable bit
	sciPrintf("WDT_IRQHandler WDMOD = %d\r\n", mode);

	// Bit 2 (WDTOF) Watchdog time-out flag
	if (mode & (1<<2)) {
		// Clear Watchdog time-out flag
		wdtClearTimeoutFlag();

		// User handler or just restart WDT
		if (wdtTimeoutHandler) {
			wdtTimeoutHandler();
		} else {
			wdtStart(); // Needs restart
		}
	}
}

#ifdef	EXAMPLE
/*===============================================================================
 * ウォッチドッグタイマーの動作確認
 * - タイムアウト時の振る舞を確認する
 *
 *　【WDT_FEED_ON = 0, WDT_RESET_CPU = 0】
 *	・failHandler() で wdtStart() を実行した場合
 *	  2秒後に WDT_IRQn 割り込みが発生し続け、メイン処理が停止する
 *
 *	・failHandler() で wdtStop() を実行した場合
 *	  2秒後に WDT_IRQn 割り込みが停止し、メイン処理は継続される
 *
 *	・failHandler() で NVIC_SystemReset() を実行した場合
 *	  2秒後に failHandler() 内でリセットをかける
 *
 *　【WDT_FEED_ON = 0, WDT_RESET_CPU = 1】
 *	・failHandler() は実行されず即座にリセットがかかる
 *===============================================================================*/
#include "type.h"
#include "timer.h"
#include "gpio.h"
#include "play.h"
#include "wdt.h"

/*----------------------------------------------------------------------
 * タイムアウト時の割込みハンドラ WDT_IRQHandler() から呼び出される処理
 *----------------------------------------------------------------------*/
static void failHandler(void) {
	sciPrintf("failHandler\r\n");

	ledOn(LED2);
	timerWait(250);
	ledToggle(LED2);

#if		0
	wdtStart();			// Restart WatchDog timer
#elif	0
	wdtStop();			// Stop    WatchDog timer
#else
	NVIC_SystemReset();	// Request a system reset to the MCU
#endif
}

/*----------------------------------------------------------------------
 * 動作例1:　メイン処理の中でウォッチドッグを叩く
 *----------------------------------------------------------------------*/
static void wdtExample1(void) {
	wdtTimeout(2000, failHandler);	// タイムアウト時に実行する関数を登録する
	wdtStart();						// ウォッチドッグによる監視を開始する

	// メイン処理
	while (1) {
		ledToggle(LED1);
		timerWait(500);
		wdtFeed();					// 定期的にウォッチドッグを叩く
	}
}

/*----------------------------------------------------------------------
 * 動作例2: SysTick割り込みハンドラでウォッチドッグを叩く
 *----------------------------------------------------------------------*/
static void wdtExample2(void) {
	wdtTimeout(2000, failHandler);	// タイムアウト時に実行される関数を登録する
	wdtFeedSysTick(1000);			// SysTick割込みハンドラで定期的にウォッチドッグを叩く
	wdtStart();						// ウォッチドッグによる監視を開始する

	// メイン処理
	while (1) {
		ledToggle(LED1);
		timerWait(500);
	}
}

/*----------------------------------------------------------------------
 * ウォッチドッグタイマーの動作例
 * - exampleType
 *	1: メイン処理中でウォッチドッグを叩く
 *	2: SysTick割り込み中でウォッチドッグを叩く
 *----------------------------------------------------------------------*/
void wdtExample(int exampleType) {
	const MusicScore_t m[] = {{Fa6, N16}, {Fa5, N16}};

	timerInit();	// timerWait()
	gpioInit();		// ledToggle()
	wdtInit();

	// 再起動を知らせる短音
	playInit();
	playScore(m, 2, 180, 0);
	while (playIsPlaying());

#if	WDT_DEBUG
	sciInit();		// PIO0_3がUSB_VBUSと競合するため、LED1（橙）点灯せず
	swStandby();	// 通信の確立を確認し、SW1で動作を開始する
#endif

	switch (exampleType) {
	  case 1:
		wdtExample1();
		break;

	  case 2:
	  default:
		wdtExample2();
		break;
	}
}
#endif // EXAMPLE
