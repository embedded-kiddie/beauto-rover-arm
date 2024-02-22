/*===============================================================================
 * Name        : clk.h
 * Author      : $(author)
 * Version     :
 * CPU type    : ARM Cortex-M3 LPC1343
 * Copyright   : $(copyright)
 * Description : Microcontroller internal clock definitions
 *===============================================================================*/
#ifndef _CLK_H_
#define _CLK_H_

/*----------------------------------------------------------------------
 * メインクロックの発信源、即ちクロック周波数を変更した場合、タイマーのプリスケール値も
 * 変更する必要があるため、必要な関数などを以下に定義をする
 *----------------------------------------------------------------------*/
// 3.5.8 Watchdog oscillator control register (WDTOSCCTRL)
// Bit 8:5 (FREQSEL) Select watchdog oscillator analog output frequency (Fclkana)
typedef enum {
	WDT_FREQSEL_ILLEGAL,
	WDT_FREQSEL_0_60,	// 0.6  MHz
	WDT_FREQSEL_1_05,	// 1.05 MHz
	WDT_FREQSEL_1_40,	// 1.4  MHz
	WDT_FREQSEL_1_75,	// 1.75 MHz
	WDT_FREQSEL_2_10,	// 2.1  MHz
	WDT_FREQSEL_2_40,	// 2.4  MHz
	WDT_FREQSEL_2_70,	// 2.7  MHz
	WDT_FREQSEL_3_00,	// 3.0  MHz
	WDT_FREQSEL_3_25,	// 3.25 MHz
	WDT_FREQSEL_3_50,	// 3.5  MHz
	WDT_FREQSEL_3_75,	// 3.75 MHz
	WDT_FREQSEL_4_00,	// 4.0  MHz
	WDT_FREQSEL_4_20,	// 4.2  MHz
	WDT_FREQSEL_4_40,	// 4.4  MHz
	WDT_FREQSEL_4_60	// 4.6  MHz
} WDT_FREQSEL_t;

/*----------------------------------------------------------------------
 *
 *----------------------------------------------------------------------*/
// 3.5.11 System PLL clock source select register (SYSPLLCLKSEL)
// 3.5.15 Main clock source select register (MAINCLKSEL)
// 3.5.27 WDT clock source select register (WDTCLKSEL)
#define	CLK_SRC_IRCOSC	0	// IRC oscillator (12[HMz])
#define	CLK_SRC_PLLIN	1	// Input clock to system PLL (12[HMz])
#define	CLK_SRC_WDTOSC	2	// WDT oscillator (9375[Hz])
#define	CLK_SRC_PLLOUT	3	// System PLL clock out (72[HMz])

// Internal oscillator frequency
#define CLK_FREQ_IRCOSC	(12000000UL)
#define CLK_FREQ_PLLIN	(12000000UL)

/*----------------------------------------------------------------------
 *
 *----------------------------------------------------------------------*/
extern void SwitchMainClockSrc(int src);
extern void SwitchWDTClockSrc(int src);
extern void SetWDTClock(WDT_FREQSEL_t freqsel, int divsel);
extern unsigned long GetWDTClock(void);
extern unsigned long GetSysPLLClock(void);
extern unsigned long GetMainClock(void);

#endif // _CLK_H_
