/*===============================================================================
 * Name        : pwm.h
 * Author      : $(author)
 * Version     :
 * CPU type    : ARM Cortex-M3 LPC1343
 * Copyright   : $(copyright)
 * Description : Pulse Width Demodulator definitions
 *===============================================================================*/
#ifndef _PWM_H_
#define _PWM_H_

/*----------------------------------------------------------------------
 * 関数のプロトタイプ宣言
 *----------------------------------------------------------------------*/
extern void pwmInit(void);
extern void pwmOut(short L, short R);

/*----------------------------------------------------------------------
 * PWM出力の最大値
 *----------------------------------------------------------------------*/
#define	PWM_MAX		(32767)

#ifdef	EXAMPLE
/*===============================================================================
 * モーターPWM制御の動作確認
 * - 電池ボックスをオンにして確認すること
 * - Vstone社製 Mtr_Run_lv() とはPWM指示値の左右、正負が異なることに注意
 * - exampleType
 *	1: 直進性を確認し、駆動系のゲインを調整する
 *	2: 前進 --> 右旋回 --> 左旋回 --> 後退
 *===============================================================================*/
extern void pwmExample(int exampleType);
#endif // EXAMPLE

#endif // _PWM_H_
