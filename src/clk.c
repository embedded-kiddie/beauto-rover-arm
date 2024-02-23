/*===============================================================================
 * Name        : clk.c
 * Author      : $(author)
 * Version     :
 * CPU type    : ARM Cortex-M3 LPC1343
 * Copyright   : $(copyright)
 * Description : Microcontroller internal clock functions
 *===============================================================================*/
// Cortex Microcontroller Software Interface Standard
#ifdef __USE_CMSIS
#include "LPC13xx.h"
#endif

/*----------------------------------------------------------------------
 * Power Management Unit debug configuration
 *----------------------------------------------------------------------*/
#define	CLK_DEBUG		0
#if		CLK_DEBUG
#include <stdio.h>
#include "sci.h"
#else
#define	sciPrintf(...)
#endif

#include "type.h"
#include "clk.h"

/*----------------------------------------------------------------------
 * メインクロックの発信源、即ちクロック周波数を変更した場合、タイマーのプリスケール値も
 * 変更する必要があるため、必要な関数などを以下に定義をする
 *----------------------------------------------------------------------*/
// 3.5.8 Watchdog oscillator control register (WDTOSCCTRL)
// Bit 8:5 (FREQSEL) Select watchdog oscillator analog output frequency (Fclkana)
static const unsigned long wdtOSCFreq[] = {
	0UL,		// WDT_FREQSEL_ILLEGAL
	600000UL,	// WDT_FREQSEL_0_60
	1050000UL,	// WDT_FREQSEL_1_05
	1400000UL,	// WDT_FREQSEL_1_40
	1750000UL,	// WDT_FREQSEL_1_75
	2100000UL,	// WDT_FREQSEL_2_10
	2400000UL,	// WDT_FREQSEL_2_40
	2700000UL,	// WDT_FREQSEL_2_70
	3000000UL,	// WDT_FREQSEL_3_00
	3250000UL,	// WDT_FREQSEL_3_25
	3500000UL,	// WDT_FREQSEL_3_50
	3750000UL,	// WDT_FREQSEL_3_75
	4000000UL,	// WDT_FREQSEL_4_00
	4200000UL,	// WDT_FREQSEL_4_20
	4400000UL,	// WDT_FREQSEL_4_40
	4600000UL,	// WDT_FREQSEL_4_60
};

/*----------------------------------------------------------------------
 * メインクロックの発信源を変更する
 * - src
 * 	CLK_SRC_IRCOSC	IRC oscillator (12[HMz])
 * 	CLK_SRC_PLLIN	Input clock to system PLL (12[HMz])
 * 	CLK_SRC_WDTOSC	WDT oscillator (9375[Hz])
 * 	CLK_SRC_PLLOUT	System PLL clock out (72[HMz])
 *----------------------------------------------------------------------*/
void clkSwitchMainClockSrc(int src) {
	extern void timerInit(void);

	switch (src) {
	  case CLK_SRC_IRCOSC:
	  case CLK_SRC_PLLIN:
	  case CLK_SRC_WDTOSC:
	  case CLK_SRC_PLLOUT:
		break;
	  default:
		return;
	}

	// 3.5.15 Main clock source select register (MAINCLKSEL)
	// 3.5.16 Main clock source update enable register (MAINCLKUEN)
	// 3.5.17 System AHB clock divider register (SYSAHBCLKDIV)
	LPC_SYSCON->MAINCLKSEL = src;
	LPC_SYSCON->MAINCLKUEN = 0; // first write a zero to the MAINCLKUEN
	LPC_SYSCON->MAINCLKUEN = 1;	// then  write a one  to the MAINCLKUEN
	while (!(LPC_SYSCON->MAINCLKUEN & 0x01));
	LPC_SYSCON->SYSAHBCLKDIV = 1; // Divide by 1 and enable WDCLK

	// タイマープリスケールを更新する
	timerInit();

#if	CLK_DEBUG
	// Watchdog oscillator の場合、周波数が低過ぎてシリアル通信ができない
	if ((LPC_SYSCON->MAINCLKSEL & 0x3) != CLK_SRC_WDTOSC) {
		sciPrintf("Main clock source = %d\r\n", LPC_SYSCON->MAINCLKSEL);
		sciPrintf("Main clock frequency = %d\r\n", clkGetMainClock());
	}
#endif
}

/*----------------------------------------------------------------------
 * ウォッチドッグタイマーのクロック発信源を変更する
 * - src
 * 	CLK_SRC_IRCOSC	IRC oscillator (12[HMz])
 * 	CLK_SRC_PLLIN	Input clock to system PLL (12[HMz])
 * 	CLK_SRC_WDTOSC	WDT oscillator (9375[Hz])
 * 	CLK_SRC_PLLOUT	System PLL clock out (72[HMz])
 *----------------------------------------------------------------------*/
void clkSwitchWDTClockSrc(int src) {
	switch (src) {
	  case CLK_SRC_IRCOSC:
	  case CLK_SRC_PLLIN:
	  case CLK_SRC_WDTOSC:
	  case CLK_SRC_PLLOUT:
		break;
	  default:
		return;
	}

	// Select watchdog oscillator for WDT clock source
	// 3.5.27 WDT clock source select register (WDTCLKSEL)
	// 3.5.28 WDT clock source update enable register (WDTCLKUEN)
	// 3.5.29 WDT clock divider register (WDTCLKDIV)
	LPC_SYSCON->WDTCLKSEL = src;
	LPC_SYSCON->WDTCLKUEN = 0; // first write a '0'
	LPC_SYSCON->WDTCLKUEN = 1; // then  write a '1'
	while (!(LPC_SYSCON->WDTCLKUEN & 0x01));
	LPC_SYSCON->WDTCLKDIV = 1; // Divide by 1 and enable WDCLK
}

/*----------------------------------------------------------------------
 * ウォッチドッグOSCのクロックを設定する
 *----------------------------------------------------------------------*/
void clkSetWDTClock(WDT_FREQSEL_t freqsel, int divsel) {
	// 3.5.8 Watchdog oscillator control register (WDTOSCCTRL)
	// Bit 4:0 (DIVSEL), Bit 8:5 (FREQSEL)
	// wdt_osc_clk = Fclkana/(2 × (1 + DIVSEL))
	// --> DIVSEL = (Fclkana/wdt_osc_clk) ÷ 2 - 1
	LPC_SYSCON->WDTOSCCTRL = ((freqsel << 5) | ((divsel >> 1) - 1));
}

/*----------------------------------------------------------------------
 * ウォッチドッグOSCの周波数を取得する
 *----------------------------------------------------------------------*/
unsigned long clkGetWDTClock(void) {
	unsigned long wdtfreq, freqsel, divsel;

	// Calculate Watchdog oscillator frequency divided by DIVSEL
	// 3.5.8 Watchdog oscillator control register
	// Bit 4:0 (DIVSEL) Select divider for Fclkana
	// Bit 8:5 (FREQSEL) Select watchdog oscillator analog output frequency (Fclkana)
	// wdt_osc_clk = Fclkana/(2 × (1 + DIVSEL))
	freqsel = ((LPC_SYSCON->WDTOSCCTRL >> 5) & 0xF);
	divsel  = (LPC_SYSCON->WDTOSCCTRL & 0x1F);
	wdtfreq = wdtOSCFreq[freqsel] / ((divsel + 1) << 1);

#if	CLK_DEBUG
	// Watchdog oscillator の場合、周波数が低過ぎてシリアル通信ができない
	if ((LPC_SYSCON->MAINCLKSEL & 0x3) != CLK_SRC_WDTOSC) {
		sciPrintf("freqsel = %d\r\n", freqsel);
		sciPrintf("divsel  = %d\r\n", divsel );
		sciPrintf("wdtfreq = %d\r\n", wdtfreq);
	}
#endif

	return wdtfreq;
}

/*----------------------------------------------------------------------
 * システムPLLクロックの周波数を取得する
 *----------------------------------------------------------------------*/
unsigned long clkGetSysPLLClock(void) {
	unsigned long clock = 0;

	// 3.5.11 System PLL clock source select register (SYSPLLCLKSEL)
	// Bit 1:0 (SEL) System PLL clock source (0 = IRC oscillator, 1 = System oscillator)
	switch (LPC_SYSCON->SYSPLLCLKSEL & 0x3) {
	  case CLK_SRC_IRCOSC:	// 0: 12[MHz]
		clock = CLK_FREQ_IRCOSC;
		break;

	  case CLK_SRC_PLLIN:	// 1: 12[MHz]
		clock = CLK_FREQ_PLLIN;
		break;
	}

	return clock;
}

/*----------------------------------------------------------------------
 * メインクロックの周波数を取得する
 *----------------------------------------------------------------------*/
unsigned long clkGetMainClock(void) {
	unsigned long clock = 0;

	// 3.5.15 Main clock source select register (MAINCLKSEL)
	switch (LPC_SYSCON->MAINCLKSEL & 0x3) {
	  case CLK_SRC_IRCOSC:	// 0: 12[MHz]
		clock = CLK_FREQ_IRCOSC;
		break;

	  case CLK_SRC_PLLIN:	// 1: 12[MHz]
		clock = clkGetSysPLLClock();
		break;

	  case CLK_SRC_WDTOSC:	// 2: 9375[Hz]
		clock = clkGetWDTClock();
		break;

	  case CLK_SRC_PLLOUT:	// 3: 72[MHz]
		// 3.5.3 System PLL control register (SYSPLLCTRL)
		// Bit 4:0 (MSEL) Feedback divider value (The division is MSEL + 1)
		clock = (LPC_SYSCON->SYSPLLCTRL & 0x1F) + 1;
		clock *= clkGetSysPLLClock();
		break;
	}

	return clock;
}
