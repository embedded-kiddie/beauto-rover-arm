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
extern void PWM_INIT(void);
extern void PWM_OUT(short L, short R);

/*----------------------------------------------------------------------
 * PWM出力の最大値
 *----------------------------------------------------------------------*/
#define	PWM_MAX		(32767)

#ifdef	EXAMPLE
/*===============================================================================
 * モーターPWM制御の動作確認
 *	- 電池ボックスをオンにして確認すること
 *	- Vstone社製 Mtr_Run_lv() とはPWM指示値の正負が異なることに注意すること
 *===============================================================================*/
extern void PWM_EXAMPLE(int method);
#endif // EXAMPLE

#endif // _PWM_H_
