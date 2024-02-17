/*===============================================================================
 * Name        : pwm.c
 * Author      : $(author)
 * Version     :
 * CPU type    : ARM Cortex-M3 LPC1343
 * Copyright   : $(copyright)
 * Description : Pulse Width Demodulator functions using CT16B0, CT16B1
 *===============================================================================*/
// Cortex Microcontroller Software Interface Standard
#ifdef __USE_CMSIS
#include "LPC13xx.h"
#endif

#include "type.h"
#include "pwm.h"
#include "ioport.h"

/*----------------------------------------------------------------------
 * 左右駆動系のバラツキを補正する係数[%]
 *----------------------------------------------------------------------*/
#define	PWM_GAIN_RATIO	100		// 百分率
#define	PWM_GAIN_L		100		// 左駆動系の補正係数
#define	PWM_GAIN_R		96		// 右駆動系の補正係数

/*----------------------------------------------------------------------
 * 停止時の出力 （FALSE:フリー、TRUE：回生ブレーキ）
 *
 * 回転方向の出力ポート（PIO0_8, PIO0_9）を両方とも 'H' とすれば、
 * 原理上は回生ブレーキとなるが、効果は観測されず、また停止し続けた
 * 場合は僅かながら電力を消費することになるため、FALSE とする。
 * ただし TRUE とすることで実験可能なコードは残しておくこととする。
 *----------------------------------------------------------------------*/
#define	STOP_BRAKE	FALSE

/*----------------------------------------------------------------------
 * PWM出力の初期化 - I/Oピン、タイマの設定
 *----------------------------------------------------------------------*/
void PWM_INIT(void) {
	/*-----------------------------------------
	 * I/O Configuration
	 *-----------------------------------------*/
	// 7.4.23 IOCON_PIO0_8 (MTR1)
	// Bit 2:0 (FUNC): Selects pin function, 0x2=Selects function CT16B0_MAT0
	// Bit 4:3 (MODE): On-chip pull-up/pull-down resistor control
	LPC_IOCON->PIO0_8 &= 0xE7; // 1110-0111, Pull-up disable
	LPC_IOCON->PIO0_8 |= 0x02; // 0000-0010, Selects function CT16B0_MAT0

	// 7.4.13 IOCON_PIO1_9 (MTR2)
	// Bit 2:0 (FUNC): Selects pin function, 0x2=Selects function CT16B1_MAT0
	// Bit 4:3 (MODE): On-chip pull-up/pull-down resistor control
	LPC_IOCON->PIO1_9 &= 0xE7; // 1110-0111, Pull-up disable
	LPC_IOCON->PIO1_9 |= 0x01; // 0000-0010, Selects function CT16B1_MAT0

	// 9.4.2 GPIO data direction register (MTR1)
	// Bit 11:0 (IO): Selects pin x as input or output (x = 0 to 11)
	SetGpioDir(LPC_GPIO2, 0, 1);	// Set PIO2_0 as output port pin
	SetGpioDir(LPC_GPIO2, 1, 1);	// Set PIO2_1 as output port pin

	// 9.4.2 GPIO data direction register (MTR2)
	// Bit 11:0 (IO): Selects pin x as input or output (x = 0 to 11)
	SetGpioDir(LPC_GPIO2, 2, 1);	// Set PIO2_2 as output port pin
	SetGpioDir(LPC_GPIO2, 3, 1);	// Set PIO2_3 as output port pin

	// 9.4.1 GPIO data register
	// Bit 11:0 (DATA): Logic levels for pins PIOn_0 to PIOn_11. HIGH=1, LOW=0
	LPC_GPIO2->DATA &= ~0x0FFF; // Clear motor bit

	/*-----------------------------------------
	 * TMR16B0, TMR16B1 Configuration
	 *-----------------------------------------*/
	// 3.5.18 System AHB clock control register
	// Bit 7 (CT16B0): Enables clock for 16-bit counter/timer 0
	// Bit 8 (CT16B1): Enables clock for 16-bit counter/timer 1
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<8|1<<7); // Enable AHB clock to the CT16B0, CT16B1

	// 15.8.12 PWM Control register (TMR16B0PWMC, TMR16B1PWMC)
	// Bit 3:0 (PWMENn): PWM channel n enable
	LPC_TMR16B0->PWMC = (0<<3|0<<2|0<<1|1<<0); // MAT0 enable, MAT1, MAT2, MAT3 disable
	LPC_TMR16B1->PWMC = (0<<3|0<<2|0<<1|1<<0); // MAT0 enable, MAT1, MAT2, MAT3 disable

	// 15.8.1 Interrupt Register (TMR16B0IR, TMR16B1IR)
	// Bit 3:0 (MRnINT): Reset the interrupt flag for MR0INT～MR3INT
	// Note: Writing '1' to the corresponding IR bit will reset the interrupt
	//       Writing '0' has no effect
	LPC_TMR16B0->IR = 0x0f; // Bit 3:0
	LPC_TMR16B1->IR = 0x0f; // Bit 3:0

	// 15.8.2 Timer Control Register (TMR16B0TCR, TMR16B1TCR)
	// Bit 0 (CEN): 1=Enable TC and PC for counting, 0=Disable
	LPC_TMR16B0->TCR &= ~0x01; // Disable TC and PC (Timer stop)
	LPC_TMR16B1->TCR &= ~0x01; // Disable TC and PC (Timer stop)

	// 15.8.4 PreScale Register (TMR16B0PR, TMR16B1PR)
	// Bit 15:0 (PR): PreScale max value
	// Note: TC is incremented on every PCLK when PR = 0
	LPC_TMR16B0->PR = 0; // PCLK = 72MHz を前提とする
	LPC_TMR16B1->PR = 0; // PCLK = 72MHz を前提とする

	// 15.8.6 Match Control Register (TMR16B0MCR, TMR16B1MCR)
	// Bit 11:0 (MRn[IRS]): Interrupt(I), Reset(R), Stop(S) when MRn matches TC
	LPC_TMR16B0->MCR = 0; // Disable operations when MR0 matches TC
	LPC_TMR16B1->MCR = 0; // Disable operations when MR0 matches TC

	// 15.8.7 Match Registers (TMR16B0MR0, TMR16B1MR0)
	// Actions are controlled by the settings in the MCR register
	// Bit 15:0 (MATCH): Timer counter match value
	LPC_TMR16B0->MR0 = ~0x0000; // Set duty 0% for MTR1
	LPC_TMR16B1->MR0 = ~0x0000; // Set duty 0% for MTR2

	// 15.8.2 Timer Control Register (TMR16B0TCR, TMR16B1TCR)
	// Bit 0 (CEN): 1=Enable TC and PC for counting, 0=Disable
	LPC_TMR16B0->TCR |= 0x01; // Enable counter (Timer start)
	LPC_TMR16B1->TCR |= 0x01; // Enable counter (Timer start)
}

/*----------------------------------------------------------------------
 * PWM出力 - I/Oポートへの出力
 * ・R： PIO2_0、PIO2_1 (方向出力)、PIO0_8 (PWM、CT16B0_MAT0)
 * ・L： PIO2_2、PIO2_3 (方向出力)、PIO1_9 (PWM、CT16B1_MAT0)
 *
 * 【方向出力】
 *   PIO2_1/3 PIO2_0/2 	PIO0_8/PIO1_9
 *     'L'      'L'   	停止（フリー）
 *     'L'      'H'   	正転（時計回り）
 *     'H'      'L'   	逆転（反時計回り）
 *     'H'      'H'   	停止（ブレーキ）
 * 【PWM出力指示】
 *	前進		0 ～ +32767
 *	後退		0 ～ -32767
 *	※ 両輪とも、前進が正（+）、後退が負（-）をPWM指示値とする
 *----------------------------------------------------------------------*/
void PWM_OUT(short L, short R) {
	// モーター回転方向の指示値
	// PIO2_0 ～PIO2_3 を 'L' に初期化
	int dir = LPC_GPIO2->DATA & 0x0ff0;

	if (L > 0) {
		dir |= (0<<3)|(1<<2);	// 正転：PIO2_3=L, PIO2_2=H
	}
	else if (L < 0) {
		dir |= (1<<3)|(0<<2);	// 逆転：PIO2_3=H, PIO2_2=L
	}

#if	STOP_BRAKE
	else {
		dir |= (1<<3)|(1<<2);	// 停止：PIO2_3=H, PIO2_2=H
	}
#endif

	if (R > 0) {
		dir |= (0<<1)|(1<<0);	// 正転：PIO2_1=L, PIO2_0=H
	}
	else if (R < 0) {
		dir |= (1<<1)|(0<<0);	// 逆転：PIO2_1=H, PIO2_0=L
	}

#if	STOP_BRAKE
	else {
		dir |= (1<<1)|(1<<0);	// 停止：PIO2_1=H, PIO2_0=H
	}
#endif

	// 9.4.1 GPIO data register
	// Bit 11:0 DATA
	// モーター回転方向を指示
	LPC_GPIO2->DATA = dir;

	// 左右駆動系のバラツキを補正する
	L = (L * PWM_GAIN_L) / PWM_GAIN_RATIO;
	R = (R * PWM_GAIN_R) / PWM_GAIN_RATIO;

	// 最小値、最大値で制限
//	L = MIN(PWM_MAX, MAX(-PWM_MAX, L));
//	R = MIN(PWM_MAX, MAX(-PWM_MAX, R));

	// 15.8.7 Match Registers (TMR16B0MR0, TMR16B1MR0)
	// モーターに出力値を設定
	// MR0 = TC の場合： Duty 0%
	// MR0 = TC + 1 の場合: Duty 100%
	LPC_TMR16B0->MR0 = ~(ABS(R) * 2 + (R ? 1 : 0));
	LPC_TMR16B1->MR0 = ~(ABS(L) * 2 + (L ? 1 : 0));
}

#ifdef	EXAMPLE
/*===============================================================================
 * モーターPWM制御の動作確認
 * - 電池ボックスをオンにして確認すること
 * - Vstone社製 Mtr_Run_lv() とはPWM指示値の左右、正負が異なることに注意
 *===============================================================================*/
#include "type.h"
#include "timer.h"
#include "ioport.h"
#include "pwm.h"

/*----------------------------------------------------------------------
 * 動作例1: 直進性を確認し、駆動系のゲインを調整する
 *----------------------------------------------------------------------*/
void PWM_EXAMPLE1(void) {
	unsigned short pwm, delta = PWM_MAX / 200;

	while (1) {
		// 前進
		for (pwm = 0; pwm <= PWM_MAX; pwm += delta) {
			PWM_OUT(pwm, pwm);
			WAIT(10);
		}

		// 停止
		PWM_OUT(0, 0);
		WAIT(1000);

		// 後退
		for (pwm = 0; pwm <= PWM_MAX; pwm += delta) {
			PWM_OUT(-pwm, -pwm);
			WAIT(10);
		}

		// 停止
		PWM_OUT(0, 0);
		WAIT(1000);

		// スイッチが押されるまで待機
		while (!SW_CLICK()) { LED_FLUSH(100); }
		LED(LED_ON);
	}
}

/*----------------------------------------------------------------------
 * 動作例2: 前進 --> 右旋回 --> 左旋回 --> 後退
 *----------------------------------------------------------------------*/
void PWM_EXAMPLE2(void) {
	short pwm = PWM_MAX / 2; // デューティ50%で動作

	while (1) {
		PWM_OUT(+pwm, +pwm); WAIT(1000); // 前進
		PWM_OUT(   0,    0); WAIT(1000); // 停止

		PWM_OUT(+pwm, -pwm); WAIT(1000); // 右回転
		PWM_OUT(   0,    0); WAIT(1000); // 停止

		PWM_OUT(-pwm, +pwm); WAIT(1000); // 左回転
		PWM_OUT(   0,    0); WAIT(1000); // 停止

		PWM_OUT(-pwm, -pwm); WAIT(1000); // 後退
		PWM_OUT(   0,    0); WAIT(1000); // 停止
	}
}

/*----------------------------------------------------------------------
 * モーターPWM制御の動作確認
 *  1: 直進性を確認し、駆動系のゲインを調整する
 *  2: 前進 --> 右旋回 --> 左旋回 --> 後退
 *----------------------------------------------------------------------*/
void PWM_EXAMPLE(int method) {
	TIMER_INIT();	// WAIT()
	PORT_INIT();	// SW_CLICK(), LED()
	PWM_INIT();		// PWM出力の初期化

	// スイッチが押されるまで待機
	while (!SW_CLICK()) { LED_FLUSH(100); }
	WAIT(500);		// 少し待ってから
	LED(LED_ON);	// LEDを点灯させてスタート

	switch (method) {
	  case 1:
		  PWM_EXAMPLE1();
		break;

	  case 2:
	  default:
		  PWM_EXAMPLE2();
		break;
	}
}

#endif // EXAMPLE
