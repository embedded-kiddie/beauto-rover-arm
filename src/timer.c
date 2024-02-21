/*===============================================================================
 Name        : timer.c
 Author      : $(author)
 Version     :
 CPU type    : ARM Cortex-M3 LPC1343
 Copyright   : $(copyright)
 Description : Hardware / Software timer functions using CT32B1
===============================================================================*/
// Cortex Microcontroller Software Interface Standard
#ifdef __USE_CMSIS
#include "LPC13xx.h"
#endif

#include "type.h"
#include "clk.h"
#include "timer.h"

/*----------------------------------------------------------------------
 * 1msecごとのカウンタ
 *----------------------------------------------------------------------*/
volatile static unsigned char cancelTimer = 0;

/*----------------------------------------------------------------------
 * 指定時間後に起動する関数
 *----------------------------------------------------------------------*/
static void (*TIMER_IRQHandler)(void) = 0;

/*----------------------------------------------------------------------
 * タイマCT32B1の初期化
 *----------------------------------------------------------------------*/
void TIMER_INIT(void) {
	/*-----------------------------------------
	 * 16.2 Basic configuration
	 *-----------------------------------------*/
	// 16.8.2 Timer Control Register (TMR32B1TCR)
	LPC_TMR32B1->TCR = 0; // Bit0(CEN) : When zero, the counters are disabled

	// 3.5.18 System AHB clock control register (SYSAHBCLKCTRL)
	// Bit10: Enables clock for 32-bit counter/timer 1
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<10);

	// 16.8.4 PreScale Register (TMR32B1PR)
	// Bit31:0(PR): Maximum value for the PC
	// Note: LPC_SYSCON->SYSAHBCLKDIV = 1 --> PCLK = 72MHz
	LPC_TMR32B1->PR = GetMainClock() / 1000 - 1; //TIMER_PRE_SCALE - 1;

	// 16.8.2 Timer Control Register (TMR32B1TCR)
	LPC_TMR32B1->TCR = 2; // Bit1(CRES): TC and PC are synchronously reset on next positive edge of PCLK
	LPC_TMR32B1->TCR = 1; // Bit0(CEN) : TC and PC are enabled for counting
}

/*----------------------------------------------------------------------
 * タイマーカウンタの読み出し
 *----------------------------------------------------------------------*/
inline unsigned long TIMER_READ(void) {
	// 16.8.3 Timer Counter (TMR32B0TC)
	return LPC_TMR32B1->TC;
}

/*----------------------------------------------------------------------
 * 指定時間後に割り込みを起動する
 *----------------------------------------------------------------------*/
void TIMER_WAKEUP(unsigned long msec, void (*f)(void)) {
	// 実行する関数を登録
	TIMER_IRQHandler = f;

	// 16.8.6 Match Control Register (TMR32B1MCR)
	// Bit 0(MR0I): Enable interrupt when MR0 matches TC
	LPC_TMR32B1->MCR = (1<<0);

	// Reset the interrupt flag for MR0INT～MR3INT
	// 16.8.1 Interrupt Register (TMR32B1IR)
	// Note: Writing a zero has no effect.
	LPC_TMR32B1->IR = (1<<0); // Reset interrupt flag for match channel 0

	// 16.8.7 Match Registers (TMR32B1MRn)
	// Bit 31:0 (MATCH): Timer counter match value
	// Note: Initial value is required before timer start
	LPC_TMR32B1->MR0 = TIMER_READ() + MAX(1, msec);

	// Configure CT32B1_MAT0 to go from High to Low for Deep Sleep mode
	// 16.8.10 External Match Register (TMR32B1EMR)
	// Bit 0 (EM0): External Match 0 reflects the state of output CT32Bn_MAT0 --> High
	// Bit 5:4 (EMC0): Determines the functionality of External Match 0 --> Low
	// Note: This is tightly coupled with the start logic edge control to enter Deep Sleep mode
	LPC_TMR32B1->EMR = ((1<<0) | (1<<4));

	// 6.6.2 Interrupt Set-Enable Register 1 (ISER1)
	// Bit 12 (ISE_CT32B1): Enable timer CT32B1 interrupt
	NVIC_ClearPendingIRQ(TIMER_32_1_IRQn);
	NVIC_EnableIRQ(TIMER_32_1_IRQn); // Nested Vectored Interrupt Controller (NVIC)
}

/*----------------------------------------------------------------------
 * タイマCT32B1 割り込みハンドラ
 *----------------------------------------------------------------------*/
void TIMER32_1_IRQHandler(void) {
	uint32_t IR = LPC_TMR32B1->IR;

	// 6.6.4 Interrupt Clear-Enable Register 1 register (ICER1)
	// Bit12(ICE_CT32B1): Timer CT32B1 interrupt disable
	NVIC_DisableIRQ(TIMER_32_1_IRQn); // Nested Vectored Interrupt Controller (NVIC)

	// Reset the interrupt flag for MR0INT～MR3INT
	// 16.8.1 Interrupt Register (TMR32B1IR)
	// Note: Writing a zero has no effect.
	LPC_TMR32B1->IR = IR; // Required

	// 登録された関数を呼び出す
	if (TIMER_IRQHandler) {
		TIMER_IRQHandler();
	}
}

/*----------------------------------------------------------------------
 * 指定時間[msec]の待機
 *----------------------------------------------------------------------*/
#define	SOFTWARE_WAIT	2000UL // ソフトウェアタイマーの1msec当たりの空ループ回数

void __attribute__((optimize("O0"))) WAIT(unsigned long msec) {
	// キャンセルフラグを初期化する
	cancelTimer = 0;

	// 16.8.2 Timer Control Register (TMR32B1TCR)
	// Bit0(CEN) : TC and PC are enabled for counting
	if (LPC_TMR32B1->TCR & 1) {
		unsigned long count = TIMER_READ() + msec;
		while (!cancelTimer && count > TIMER_READ()) {
			__NOP();
		}
	}

	// Software timer
	// TIMER_INIT()が実行されなかった場合は空ループで代替する
	else {
		// Disable optimization and force the counter to be placed into memory
		volatile static unsigned long i;
		for (i = msec * SOFTWARE_WAIT; !cancelTimer && i > 0; i--) {
			__NOP();
		}
	}
}

/*----------------------------------------------------------------------
 * 待機のキャンセル
 *----------------------------------------------------------------------*/
void WAIT_CANCEL(void) {
	cancelTimer = 1;
}
