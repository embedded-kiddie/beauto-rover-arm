/*===============================================================================
 * Name        : trace.c
 * Author      : $(author)
 * Version     :
 * Model type  : VS-WRC103LV
 * Copyright   : $(copyright)
 * Description : Pulse Width Demodulator functions using CT16B0, CT16B1
 *===============================================================================*/
#include "type.h"
#include "timer.h"
#include "ioport.h"
#include "adc.h"
#include "pwm.h"
#include "trace.h"

#define	TRACE_DEBUG		0
#if		TRACE_DEBUG
#include <stdio.h>
#include "sci.h"
#else
#define	SCI_PRINTF(...)
#endif

/*----------------------------------------------------------------------
 * 車体をライン中心に設置した状態で左右センサ値の差を複数回観測し、
 * 平均化（ノイズ除去）することで、原点までのオフセットを算出する
 *
 * - 座標系の考え方（白地に黒ラインの場合）
 *  ・進行方向をy軸の正、右方向をx軸の正、原点を黒ラインにとる
 *  ・車が原点より左側（右センサが黒線に近い） --> 左センサ出力 - 右センサ出力 < 0
 *  ・車が原点より右側（左センサが黒線に近い） --> 左センサ出力 - 右センサ出力 > 0
 *
 *     Y
 *     ^
 *     |
 * ----+--->X
 *----------------------------------------------------------------------*/
static short AdjustCenter(void)
{
    int i, offset = 0;
    unsigned short L, R;

    // 中央における左右間の偏差を観測する
    for (i = 0; i < N_SAMPLES; i++) {
        ADC_READ2(&L, &R);
        offset += (int)(L - R);
    }

    // 偏差の中点の平均値を算出し、原点までのオフセットとする
    return (short)(offset / (i * 2));
}

/*----------------------------------------------------------------------
 * 赤外線センサの特性計測
 * IR(Infrared)センサは、環境光中の近赤外光（800nm～）の影響を受け、
 * 制御ゲインの安定的な調整が困難になる懸念がある。そこで走行前に簡単な
 * 出力特性の正規化を行い、走行性能を安定化させることを目指す。
 *
 * 1. 車体をトレースライン中央に設置し、原点までのオフセットを観測する
 * 2. ラインを跨ぐように車体を回転させ、左右差の最大、最小を観測する
 * 3. 左右それぞれのセンサ出力を正規化するための補正係数を算出する
 *----------------------------------------------------------------------*/
static short CalibrateIR(CalibrateIR_t *cal) {
	int i, j;
	short offset, pwm;				// 原点オフセット、モーター出力値
	unsigned short L, minL, maxL;	// 左のセンサ値、最小値、最大値
	unsigned short R, minR, maxR;	// 右のセンサ値、最小値、最大値

#if TRACE_DEBUG
	int n = 0;
	IRData_t IR[N_SAMPLES * 2];
#endif

	// 原点オフセットを観測する
	cal->offset = offset = AdjustCenter();

	minL = minR = 0xFFF;
	maxL = maxR = 0;

	// i = 0: 中央 → 左回転 → 右回転 → 中央
	// i = 1; 中央 → 右回転 → 左回転 → 中央
	for (pwm = MTR_POWER, i = 0; i < 2; i++, pwm = -pwm) {
		for (j = 0; j < N_SAMPLES; j++) {
			WAIT(10);

			if (j < N_SAMPLES / 2) {
				PWM_OUT( pwm,  pwm);	// p > 0: 左回転, p < 0: 右回転
			} else {
				PWM_OUT(-pwm, -pwm);	// p > 0: 右回転, p < 0: 左回転
			}

			ADC_READ2(&L, &R);
			L -= offset;
			R -= offset;
			minL = MIN(minL, L);
			maxL = MAX(maxL, L);
			minR = MIN(minR, R);
			maxR = MAX(maxR, R);

#if TRACE_DEBUG
			IR[n  ].L = (short)(L -= offset);
			IR[n  ].R = (short)(R += offset);
#endif
		}

		// 慣性モーメントがゼロになる様、完全に停止させる
		PWM_OUT(0, 0);
		WAIT(100);
	}

#if TRACE_DEBUG
	// USBケーブルの接続し、通信の成立を確認する
	while (!SW_CLICK()) { LED_FLUSH(100); }
	SCI_INIT();
	SW_STANDBY();

	for (i = 0; i < n; i++) {
		SCI_PRINTF("%d,%d\r\n", IR[i].L, IR[i].R);
	}

	SCI_PRINTF("offset:%d, minL:%d, maxL:%d, minR:%d, maxR:%d\r\n", offset, minL, maxL, minR, maxR);
#endif

	return offset;
}

/*----------------------------------------------------------------------
 * ライントレース - 赤外線センサの特性計測
 *----------------------------------------------------------------------*/
static void TRACE_RUN0(void) {
	CalibrateIR_t cal;
	CalibrateIR(&cal);
}

/*----------------------------------------------------------------------
 * ライントレース - ON-OFF制御
 *----------------------------------------------------------------------*/
#define	CENTER	100	// 中央とみなせる赤外線センサの閾値

static const PID_t G1 = {
	0,				// Kp (Don't care)
	0,				// Kd (Don't care)
	(PWM_MAX / 2),	// 前進成分の制御量
	(PWM_MAX / 2)	// 旋回成分の制御量
};

static void TRACE_RUN1(void) {
	CalibrateIR_t cal;
	unsigned short L, R;
	short E, offset;

    // 赤外線センサの校正
	offset = CalibrateIR(&cal);

    LED(LED1);

    while (1) {
    	ADC_READ2(&L, &R);	// 左右赤外線センサ値を読み込む
    	L -= offset;		// 左赤外線センサ値の原点を校正する
    	R += offset;		// 右赤外線センサ値の原点を校正する
    	E = L - R;			// 中心からのずれを算出する

    	if (E > 0 && L > CENTER) {				// 右よりなら
    		PWM_OUT(G1.TURNING, 0);				// 左に曲げる
    	}
    	else if (E < 0 && R > CENTER) {			// 左よりなら
    		PWM_OUT(0, G1.TURNING);				// 右に曲げる
    	}
    	else {									// 中央付近なら
    		PWM_OUT(G1.FORWARD, G1.FORWARD);	// 直進する
    	}
	}
}

/*----------------------------------------------------------------------
 * ライントレース
 *  0. 赤外線センサの特性計測
 *  1. ON-OFF制御によるライントレース
 *  2. ON-OFF制御＋P制御によるライントレース
 *  3. PD制御によるライントレース
 *----------------------------------------------------------------------*/
void TRACE_RUN(int method) {
	switch (method) {
	  case 0:
		TRACE_RUN0();
		break;

	  case 1:
		TRACE_RUN1();
		break;
	}
}
