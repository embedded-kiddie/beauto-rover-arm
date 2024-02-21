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

/*----------------------------------------------------------------------
 *
 *----------------------------------------------------------------------*/
// 3.5.45 Deep-sleep mode configuration register (PDSLEEPCFG)
#define PDSLEEPCFG_BOD		(1<<3)	// BOD Powered down = ~0x0FF7
#define PDSLEEPCFG_WDTOSC	(1<<6)	// WDT oscillator Powered down = ~0x0FBF
#define PDSLEEPCFG_FIXEDVAL	((7<<0) | (3<<4) | (0x1F<<7)) // 0xFB7
#define	PDSLEEPCFG_MASK		((1<<3) | (1<<6)) // ~0x0FB7

// 3.5.46 Wake-up configuration register (PDAWAKECFG)
#define	PDAWAKECFG_IRCOUT	(1<<0)
#define	PDAWAKECFG_IRC		(1<<1)
#define	PDAWAKECFG_FLASH	(1<<2)
#define	PDAWAKECFG_BOD		(1<<3)
#define	PDAWAKECFG_ADC		(1<<4)
#define	PDAWAKECFG_SYSOSC	(1<<5)
#define	PDAWAKECFG_WDTOSC	(1<<6)
#define	PDAWAKECFG_SYSPLL	(1<<7)
#define	PDAWAKECFG_USBPLL	(1<<8)
#define	PDAWAKECFG_USBPAD	(1<<10)
#define	PDAWAKECFG_FIXEDVAL	((0<<9)|(1<<11)|(1<<12)|(7<<13)) // 0xE800
#define	PDAWAKECFG_MASK		((1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5)|(1<<6)|(1<<7)|(1<<8)|(1<<10)) // 0x05FF

// 3.5.47 Power-down configuration register (PDRUNCFG)
#define	PDRUNCFG_IRCOUT		(1<<0)
#define	PDRUNCFG_IRC		(1<<1)
#define	PDRUNCFG_FLASH		(1<<2)
#define	PDRUNCFG_BOD		(1<<3)
#define	PDRUNCFG_ADC		(1<<4)
#define	PDRUNCFG_SYSOSC		(1<<5)
#define	PDRUNCFG_WDTOSC		(1<<6)
#define	PDRUNCFG_SYSPLL		(1<<7)
#define	PDRUNCFG_USBPLL		(1<<8)
#define	PDRUNCFG_USBPAD		(1<<10)
#define	PDRUNCFG_FIXEDVAL	((0<<9)|(1<<11)|(1<<12)|(7<<13)) // 0xE800
#define	PDRUNCFG_MASK		((1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5)|(1<<6)|(1<<7)|(1<<8)|(1<<10)) // 0x05FF

/*----------------------------------------------------------------------
 *
 *----------------------------------------------------------------------*/
extern void SetPowerDownSleep(unsigned long mask);
extern void SetPowerDownAwake(unsigned long mask);
extern void DelPowerDownRun(unsigned long mask);
extern void AddPowerDownRun(unsigned long mask);

#endif // _CLK_H_
