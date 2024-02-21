/*===============================================================================
 * Name        : pmu.c
 * Author      : $(author)
 * Version     :
 * CPU type    : ARM Cortex-M3 LPC1343
 * Copyright   : $(copyright)
 * Description : Power Management Unit functions
 *===============================================================================*/
// Cortex Microcontroller Software Interface Standard
#ifdef __USE_CMSIS
#include "LPC13xx.h"
#endif

/*----------------------------------------------------------------------
 * Power Management Unit debug configuration
 *----------------------------------------------------------------------*/
#define	PMU_DEBUG		0
#if		PMU_DEBUG
#include <stdio.h>
#include "sci.h"
#else
#define	SCI_PRINTF(...)
#endif

#include "type.h"
#include "clk.h"
#include "wdt.h"
#include "pmu.h"
#include "timer.h"

/*----------------------------------------------------------------------
 * Deep-sleep mode からの復帰方法
 *----------------------------------------------------------------------*/
#define	INTERNAL_TIMER		0	// 内部割込み：タイマー（CT32B1）
#define	EXTERNAL_SWITCH		1	// 外部割込み：スイッチ（SW1）
#define	INTERRUPT_FROM		0	// 0 = タイマー（CT32B1）, 1 = スイッチ（SW1）

/*----------------------------------------------------------------------
 *
 *----------------------------------------------------------------------*/
static unsigned long pwmInterval  = 0;
static unsigned char pwmWakeupPin = 0;

/*----------------------------------------------------------------------
 * スリープから復帰したときの割込みハンドラ
 *----------------------------------------------------------------------*/
void WAKEUP_IRQHandler(void) {
	// The start-up logic states must be cleared before waking up from Deep-sleep mode
	// 3.5.39 Start logic reset register 0 (STARTRSRP0CLR)
	// 3.5.38 Start logic signal enable register 0 (STARTERP0)
	if (pwmWakeupPin) {
		LPC_SYSCON->STARTRSRP0CLR |=  (1 << pwmWakeupPin); // Clear pending bit   of pwmWakeupPin
		LPC_SYSCON->STARTERP0     &= ~(1 << pwmWakeupPin); // Disable Start logic of pwmWakeupPin
	}

	// メインクロックの復帰
	if (LPC_SYSCON->MAINCLKSEL != CLK_SRC_PLLOUT) {
		SwitchMainClockSrc(CLK_SRC_PLLOUT);
	}

	// 次のタイマーの設定
	if (pwmInterval) {
		TIMER_WAKEUP(pwmInterval, WAKEUP_IRQHandler);
	}

	// https://github.com/microbuilder/LPC1343CodeBase/blob/master/core/pmu/pmu.c#L101C2-L101C26
	__asm volatile ("NOP"); // 効果なし
}

/*----------------------------------------------------------------------
 * スリープを復帰させるタイマーの周期を設定する（INTERRUPT_FROM = 0 の場合）
 *----------------------------------------------------------------------*/
void PMU_WAKEUP(unsigned long msec) {
	// 初期化
	pwmInterval = msec;
}

/*----------------------------------------------------------------------
 * 3.9.2.2 Programming Sleep mode
 * The following codes are from application note AN10973
 *----------------------------------------------------------------------*/
static void SleepMode() {
	// 1. The DPDEN bit in the PCON register must be set to zero
	//    and clear the Deep Power down flag from the PMU
	// 4.2.1 Power control register (PCON)
	// Bit  1 (DPDEN)  : 0 = ARM WFI will enter Sleep or Deep-sleep mode
	// Bit 11 (DPFLAG) : 1 = Clear the Deep power-down flag
	LPC_PMU->PCON = ((0<<1) | (1<<11));

	// 2. Specify Sleep mode before entering mode
	// Write one to the SLEEPDEEP bit in the ARM Cortex-M3 SCR register
	// https://developer.arm.com/documentation/dui0552/a/cortex-m3-peripherals/system-control-block/system-control-register
	SCB->SCR &= ~(1<<2); // Clear SLEEPDEEP bit

#if	0
	SwitchMainClockSrc(CLK_SRC_IRCOSC); // IRC oscillator (12[HMz])
#else
	SwitchMainClockSrc(CLK_SRC_WDTOSC); // WDT oscillator (9375[Hz])
#endif
}

/*----------------------------------------------------------------------
 * 3.9.3.2 Programming Deep-sleep mode
 * The following codes are from application note AN10973
 *----------------------------------------------------------------------*/
static void DeepSleepMode() {

#if	(INTERRUPT_FROM == INTERNAL_TIMER)

	// 0. Selects function CT32B1_MAT0
	// 7.4.30 IOCON_R_PIO1_1 (I/O configuration for pin R/PIO1_1/AD2/CT32B1_MAT0)
	// Bit 2:0 (FUNC): 0x3 = Selects function CT32B1_MAT0
	// Bit 7 (ADMODE): 1 = Digital functional mode
	LPC_IOCON->R_PIO1_1 = ((3<<0) | (1<<7));

	// 1. Turn on the IRC, FLASH and WDTOSC
	// Note: IRCOUT, IRC and FLASH are set by reset, WDTOSC is set in WDT_INIT()
//	AddPowerDownRun(PDRUNCFG_IRCOUT | PDRUNCFG_IRC | PDRUNCFG_FLASH | PDRUNCFG_WDTOSC);

	// 2. メインクロックを WDT oscillator (9375[Hz]) に変更する
	SwitchMainClockSrc(CLK_SRC_WDTOSC);

	// 3. Ensure DPDEN is disabled in the power control register
	// 4.2.1 Power control register (PCON)
	// Bit  1 (DPDEN): 0 = WFI will enter Sleep or Deep-sleep mode
	// Bit 11 (DPDFLAG): Clear the Deep power-down flag
	LPC_PMU->PCON = (1<<11); // Clear DPDFLAG if it was set

	// 4. Select the power configuration in Deep-sleep mode in the PDSLEEPCFG register
	SetPowerDownSleep(PDSLEEPCFG_WDTOSC); // BOD off, Watchdog oscillator on

	// 5. Specify peripherals to be powered up again when returning from deep sleep mode
	// 3.5.46 Wake-up configuration register (PDAWAKECFG)
	LPC_SYSCON->PDAWAKECFG = LPC_SYSCON->PDRUNCFG;

	// 6. Specify the start logic to allow the chip to be waken up
	// 3.5.37 Start logic edge control register 0 (STARTAPRP0)
	// 3.5.39 Start logic reset register 0 (STARTRSRP0CLR)
	// 3.5.38 Start logic signal enable register 0 (STARTERP0)
	// 7.4.30 IOCON_R_PIO1_1 (R/PIO1_1/AD2/CT32B1_MAT0)
	pwmWakeupPin = 13;
	LPC_SYSCON->STARTAPRP0    &= ~(1 << pwmWakeupPin);	// PIO1_1 Falling edge
	LPC_SYSCON->STARTRSRP0CLR |=  (1 << pwmWakeupPin);	// PIO1_1 Clear pending bit
	LPC_SYSCON->STARTERP0     |=  (1 << pwmWakeupPin);	// PIO1_1 Enable Start Logic

	NVIC_ClearPendingIRQ(WAKEUP13_IRQn);
	NVIC_EnableIRQ(WAKEUP13_IRQn);

	// 7. Specify Deep-sleep mode before entering mode
	// Write one to the SLEEPDEEP bit in the ARM Cortex-M3 SCR register
	// https://developer.arm.com/documentation/dui0552/a/cortex-m3-peripherals/system-control-block/system-control-register
	SCB->SCR |= (1<<2); // Bit 2: Set SLEEPDEEP bit

#else // INTERRUPT_FROM == EXTERNAL_SWITCH

	// 1. Turn on the IRC, FLASH
	// Note: IRC and FLASH are set by reset
//	AddPowerDownRun(PDRUNCFG_IRCOUT | PDRUNCFG_IRC | PDRUNCFG_FLASH);

	// 2. Switch MAINCLKSEL to IRC
	SwitchMainClockSrc(CLK_SRC_IRCOSC);

	// 3. Ensure DPDEN is disabled in the power control register
	// 4.2.1 Power control register (PCON)
	// Bit  1 (DPDEN): 0 = WFI will enter Sleep or Deep-sleep mode
	// Bit 11 (DPDFLAG): Clear the Deep power-down flag
	LPC_PMU->PCON = (1<<11); // Clear DPDFLAG if it was set

	// 4. Select the power configuration in Deep-sleep mode in the PDSLEEPCFG register
	SetPowerDownSleep(0); // BOD off, Watchdog oscillator off

	// 5. Specify peripherals to be powered up again when returning from deep sleep mode
	// 3.5.46 Wake-up configuration register (PDAWAKECFG)
	LPC_SYSCON->PDAWAKECFG = LPC_SYSCON->PDRUNCFG;

	// 6. Specify the start logic to allow the chip to be waken up
	// 3.5.37 Start logic edge control register 0 (STARTAPRP0)
	// 3.5.39 Start logic reset register 0 (STARTRSRP0CLR)
	// 3.5.38 Start logic signal enable register 0 (STARTERP0)
	// 7.4.4 IOCON_PIO0_1 (PIO0_1/CLKOUT/ 0xD0 CT32B0_MAT2/USB_FTOGGLE)
	pwmWakeupPin = 1;
	LPC_SYSCON->STARTAPRP0    &= ~(1 << pwmWakeupPin);	// PIO0_1 Falling edge
	LPC_SYSCON->STARTRSRP0CLR |=  (1 << pwmWakeupPin);	// PIO0_1 Clear pending bit
	LPC_SYSCON->STARTERP0     |=  (1 << pwmWakeupPin);	// PIO0_1 Enable Start Logic

	NVIC_ClearPendingIRQ(WAKEUP1_IRQn);
	NVIC_EnableIRQ(WAKEUP1_IRQn);

	// 7. Specify Deep-sleep mode before entering mode
	// Write one to the SLEEPDEEP bit in the ARM Cortex-M3 SCR register
	// https://developer.arm.com/documentation/dui0552/a/cortex-m3-peripherals/system-control-block/system-control-register
	SCB->SCR |= (1<<2); // Bit 2: Set SLEEPDEEP bit

#endif // INTERRUPT_FROM
}

/*----------------------------------------------------------------------
 * 3.9.4.2 Programming Deep power-down mode
 * The following codes are from application note AN10973
 *----------------------------------------------------------------------*/
static void DeepPowerDown() {
	// この例題では、リセットがかかり続けてしまうので無効にする
#if	FALSE
	// 0. Configure PIO1_4 as WAKEUP pin
	// 7.4.36 IOCON_PIO1_4 I/O configuration for pin PIO1_4/AD5/CT32B1_MAT3/WAKEUP
	// Note: This pin is pulled-up and works as WAKEUP pin if the LPC13xx is in Deep power-down mode
	;

	// 1. Write one to the DPDEN bit in the PCON register
	// 4.2.1 Power control register
	// Bit  1 (DPDEN)  : ARM WFI will enter Deep-power down mode
	// Bit 11 (DPDFLAG): Clear the Deep power-down flag
	LPC_PMU->PCON = ((1<<1) | (1<<11));

	// 2. Store data to be retained in the general purpose registers
	// 4.2.2 General purpose registers 0 to 3
	;

	// 3. Specify Deep Power-down mode before entering mode
	// Write one to the SLEEPDEEP bit in the ARM Cortex-M3 SCR register
	// https://developer.arm.com/documentation/dui0552/a/cortex-m3-peripherals/system-control-block/system-control-register
	SCB->SCR |= (1<<2); // Bit 2: Set SLEEPDEEP bit

	// 4. Enable the IRC, FLASH and WDTOSC before entering power-down mode
	// Note: IRCOUT and IRC are set by reset
//	AddPowerDownRun(PDRUNCFG_IRCOUT | PDRUNCFG_IRC);
#endif
}

/*----------------------------------------------------------------------
 * 各機能へのクロック供給を停止し、消費電力を抑える
 *----------------------------------------------------------------------*/
unsigned int PMU_SLEEP(int mode) {
	switch (mode) {
	  case PMU_ACTIVE_MODE:
		if (pwmInterval) {
			TIMER_WAKEUP(pwmInterval, WAKEUP_IRQHandler);
		}
		break;

	  case PMU_SLEEP_MODE:
		if (pwmInterval) {
			TIMER_WAKEUP(pwmInterval, WAKEUP_IRQHandler);
		}
		SleepMode();
		break;

	  case PMU_DEEP_SLEEP:
#if	(INTERRUPT_FROM == INTERNAL_TIMER)
		if (pwmInterval) {
			TIMER_WAKEUP(pwmInterval, WAKEUP_IRQHandler);
		}
#endif
		DeepSleepMode();
		break;

	  case PMU_POWER_DOWN:
	  default:
		DeepPowerDown(); // なぜかリセットがかかってしまう
		break;
	}

	// Enter Sleep/Deep-sleep/Power Down mode by the ARM WFI (Wait For Interrupt)
	__WFI();

	return 1;
}

#ifdef	EXAMPLE
/*===============================================================================
 * パワーマネジメントユニットの動作確認
 * - Active/Sleep/Deep-sleep/Deep Power-down 各モードの動作例
 *
 * 【EXAMPLE1】
 *	TIMER_WAKEUP() によるタイマー割り込みにより一定周期でタスクを起動する
 *
 * 【EXAMPLE2】
 *	内部割込み（タイマー割込み）、または外部割込み（スイッチによる割込み）でタスクを起動する
 *	・INTERRUPT_FROM = INTERNAL_TIMER （0, 内部割込み）の場合
 *	  PMW_WAKEUP() によるタイマー割り込みでタスクを起動する
 *
 *	・INTERRUPT_FROM = EXTERNAL_SWITCH （1, 外部割込み）の場合
 *	  スイッチによる割り込みでタスクを起動する
 *	  ・Active/Sleep mode では、SW_WATCH() により1回だけ起動する
 *	  ・Deep-sleep mode では、直接 start logic で外部トリガ（スイッチ）を監視しているため、
 *	    スイッチを押すたびにタスクが起動される
 *===============================================================================*/
#include "type.h"
#include "timer.h"
#include "gpio.h"
#include "play.h"
#include "wdt.h"
#include "pmu.h"

/*----------------------------------------------------------------------
 * タイマーの割込みハンドラ  TIMER32_1_IRQHandler() から呼び出される関数
 *----------------------------------------------------------------------*/
static void Wakeup(void) {
	SCI_PRINTF("Wakeup\r\n");

	TIMER_WAKEUP(1000, Wakeup);		// 1秒周期でタイマー割り込み発生させる
}

/*----------------------------------------------------------------------
 * 動作例1: Active mode　でのスリープ
 *----------------------------------------------------------------------*/
static void PMU_EXAMPLE1() {
	TIMER_WAKEUP(1, Wakeup);		// 1[msec]後にタイマー割り込み発生させる

	while (1) {
		__WFI();					// Wait For Interrupt
		LED_BLINK(100);				// タスク（100[msec] on + 100[msec] off）
	}
}

/*----------------------------------------------------------------------
 * 動作例2: Active/Sleep/Deep-sleep/Deep Power-down の各モード動作例
 *----------------------------------------------------------------------*/
static void PMU_EXAMPLE2() {
#if	(INTERRUPT_FROM == INTERNAL_TIMER)
	PMU_WAKEUP(1000);				// 内部割込み：1秒周期のタイマー割り込みでSLEEPから復帰
#else
	SW_WATCH(NULL);					// 外部割込み：スイッチの割り込みでSLEEPから復帰
#endif

	while (1) {
		// 各モードでSLEEPする
#if		0
		PMU_SLEEP(PMU_ACTIVE_MODE);	// 割込み発生から500[nsec]でSLEEPから復帰
#elif	0
		PMU_SLEEP(PMU_SLEEP_MODE);	// 割込み発生から2.9[μsec]でSLEEPから復帰
#elif	1
		PMU_SLEEP(PMU_DEEP_SLEEP);	// 割込み発生から5[msec]でSLEEPから復帰
#elif	0
		PMU_SLEEP(PMU_POWER_DOWN);	// なぜかリセットがかかってしまう
#endif

		LED_BLINK(100);				// タスク（100[msec] on + 100[msec] off）
	}
}

/*----------------------------------------------------------------------
 * パワーマネジメントユニットの動作例
 * - exampleType
 *	1: Active mode　でのスリープ
 *	2: Sleep/Deep-sleep/Deep Power-down
 *----------------------------------------------------------------------*/
void PMU_EXAMPLE(int exampleType) {
	const MusicScore_t m[] = {{Fa6, N16}, {Fa5, N16}};

	TIMER_INIT();	// TIMER_WAKEUP()
	GPIO_INIT();	// LED_BLINK()
	WDT_INIT();		// WDT oscillator (Deep-sleep mode)

	// 再起動を知らせる短音
	PLAY_INIT();
	PLAY(m, 2, 155, 0);
	while (IS_PLAYING());

#if	PMU_DEBUG
	SCI_INIT();		// PIO0_3がUSB_VBUSと競合するため、LED1（橙）点灯せず
	SW_STANDBY();	// 通信の確立を確認し、SW1で動作を開始する
#endif

	switch (exampleType) {
	  case 1:
		PMU_EXAMPLE1();
		break;

	  case 2:
	  default:
		PMU_EXAMPLE2();
		break;
	}
}
#endif // EXAMPLE
