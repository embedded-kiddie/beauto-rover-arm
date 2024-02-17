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
 * 赤外線センサのキャリブレーション方法
 *	0: 左右センサの原点位置だけを補正する
 *	1: 左右センサの出力値がそれぞれ 0 ～ IR_RANGE となるよう正規化する
 *----------------------------------------------------------------------*/
#define	CALIBRATION_METHOD	1

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
	short offset, pwm;				// 原点オフセット平均値、モーター出力値
	unsigned short L, minL, maxL;	// 左のセンサ値、最小値、最大値
	unsigned short R, minR, maxR;	// 右のセンサ値、最小値、最大値

#if TRACE_DEBUG
	int n = 0;
	IRData_t IR[N_SAMPLES * 2];
#endif

	// 原点オフセット平均値を観測する
	offset = AdjustCenter();

	minL = minR = 0xFFF;
	maxL = maxR = 0;

	// i = 0: 中央 → 左回転 → 右回転 → 中央
	// i = 1; 中央 → 右回転 → 左回転 → 中央
	for (pwm = MTR_POWER, i = 0; i < 2; i++, pwm = -pwm) {
		for (j = 0; j < N_SAMPLES; j++) {
			WAIT(10);

			if (j < N_SAMPLES / 2) {
				PWM_OUT(+pwm, -pwm); // p > 0: 左回転, p < 0: 右回転
			} else {
				PWM_OUT(-pwm, +pwm); // p > 0: 右回転, p < 0: 左回転
			}

			// 左右センサの値を読み込む
			ADC_READ2(&L, &R);

			// センサ位置のオフセットを補正する
			//L -= offset;
			//R += offset;

			// それぞれの最小値、最大値を観測する
			minL = MIN(minL, L);
			maxL = MAX(maxL, L);
			minR = MIN(minR, R);
			maxR = MAX(maxR, R);

#if TRACE_DEBUG
			IR[n  ].L = L;
			IR[n++].R = R;
#endif

			if (j == N_SAMPLES / 2 - 1) {
				// 慣性モーメントがゼロになる様、完全に停止させる
				PWM_OUT(0, 0);
				WAIT(100);
			}
		}

		// 慣性モーメントがゼロになる様、完全に停止させる
		PWM_OUT(0, 0);
		WAIT(100);
	}

	// キャリブレーションパラメータを設定する
	cal->center  = IR_CENTER;
	cal->offset  = offset;
	cal->offsetL = minL;
	cal->offsetR = minR;
	cal->gainL   = IR_RANGE * 100 / (maxL - minL);
	cal->gainR   = IR_RANGE * 100 / (maxR - minR);

#if TRACE_DEBUG
	// USBケーブルの接続し、通信の成立を確認する
	while (!SW_CLICK()) { LED_FLUSH(100); }
	SCI_INIT();		// 通信確立を確認し、
	SW_STANDBY();	// スイッチを押したら出力開始

	// 補正係数用パラメータを出力する
	SCI_PRINTF("#cal,offset,minL,maxL,gainL,minR,maxR,gainR\r\n");
	SCI_PRINTF("0,%d,%d,%d,%d,%d,%d,%d\r\n", offset, minL, maxL, cal->gainL, minR, maxR, cal->gainR);
	SCI_PRINTF("#No,L,R,L - R,L',R',L' - R'\r\n");

	// キャリブレーション前後の値を出力する
	for (i = 0; i < n; i++) {
		// 原点オフセット平均値のみ、左右の正規化なし
		L = IR[i].L - offset;
		R = IR[i].R + offset;
		SCI_PRINTF("%d,%d,%d,%d,", i + 1, L, R, (short)(L - R));

		// 左右の正規化あり
		L = (IR[i].L - cal->offsetL) * cal->gainL / 100;
		R = (IR[i].R - cal->offsetR) * cal->gainR / 100;
		SCI_PRINTF("%d,%d,%d\r\n", L, R, (short)(L - R));
	}
#endif

	return offset;
}

/*----------------------------------------------------------------------
 * ライントレース - 赤外線センサの特性計測
 *	- TRACE_DEBUG を 1 に設定、計測結果をシリアル通信経由でホストPCに送信する
 *----------------------------------------------------------------------*/
static void TRACE_RUN0(void) {
	CalibrateIR_t cal;
	CalibrateIR(&cal);
}

/*----------------------------------------------------------------------
 * ライントレース - ON-OFF制御
 *----------------------------------------------------------------------*/
static const PID_t G1 = {
	0,		// Kp (Don't care)
	0,		// Kd (Don't care)
	15000,	// 前進成分の制御量
	15000	// 旋回成分の制御量
};

static void TRACE_RUN1(void) {
	CalibrateIR_t cal;
	int L, R, E;

	// 赤外線センサのキャリブレーション
#if	(CALIBRATION_METHOD == 1)
	int oL, oR, gL, gR;

	CalibrateIR(&cal);
	oL = cal.offsetL;
	oR = cal.offsetR;
	gL = cal.gainL;
	gR = cal.gainR;
#else
	int offset = CalibrateIR(&cal);
#endif

	while (1) {
		L = ADC_READ(ADC_LEFT );	// 左赤外線センサ値を読み込む
		R = ADC_READ(ADC_RIGHT);	// 右赤外線センサ値を読み込む

#if	(CALIBRATION_METHOD == 1)
		L = (L - oL) * gL / 100;	// 左赤外線センサ値を正規化する
		R = (R - oR) * gR / 100;	// 右赤外線センサ値を正規化する
#else
		L -= offset;				// 左赤外線センサの原点を補正する
		R += offset;				// 右赤外線センサの原点を補正する
#endif

		// 中心からのずれを算出する
		E = L - R;

		// 右寄りのズレが大きければ左に曲げる
		if (E > 0 && L > IR_CENTER) {
			PWM_OUT(-G1.TURNING/2, G1.TURNING);
			LED(LED2);
		}

		// 左寄りのズレが大きければ右に曲げる
		else if (E < 0 && R > IR_CENTER) {
			PWM_OUT(G1.TURNING, -G1.TURNING/2);
			LED(LED1);
		}

		// 中央付近なら直進する
		else {
			PWM_OUT(G1.FORWARD, G1.FORWARD);
			LED(LED_ON);
		}
	}
}

/*----------------------------------------------------------------------
 * ライントレース
 *  0: 赤外線センサの特性計測
 *  1: ON-OFF制御によるライントレース
 *  2: ON-OFF制御＋P制御によるライントレース
 *  3: PD制御によるライントレース
 *----------------------------------------------------------------------*/
void TRACE_RUN(int method) {
	TIMER_INIT();	// WAIT()
	PORT_INIT();	// SW_STANDBY()
	ADC_INIT();		// A/D変換の初期化
	PWM_INIT();		// PWM出力の初期化

	LED(LED_ON);	// LEDを点灯させて
	SW_STANDBY();	// スイッチが押されるまで待機
	WAIT(500);		// 少し待ってからスタート

	switch (method) {
	  case 0:
		TRACE_RUN0();
		break;

	  case 1:
		TRACE_RUN1();
		break;

	  case 2:
		break;

	  case 3:
	  default:
		break;
	}
}
