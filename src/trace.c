/*===============================================================================
 * Name        : trace.c
 * Author      : $(author)
 * Version     :
 * Model type  : VS-WRC103LV
 * Copyright   : $(copyright)
 * Description : Line Trace functions
 *===============================================================================*/
#include "type.h"
#include "timer.h"
#include "gpio.h"
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
 * 赤外線センサ値の正規化
 *----------------------------------------------------------------------*/
#define	NormalizeL(L, c)	((L - c.offsetL) * c.gainL / 100)
#define	NormalizeR(R, c)	((R - c.offsetR) * c.gainR / 100)

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

	if (cal == NULL) {
		return offset;
	}

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
static void TraceRun0(void) {
	CalibrateIR_t cal;
	CalibrateIR(&cal);
}

/*----------------------------------------------------------------------
 * ライントレース - ON-OFF制御
 *----------------------------------------------------------------------*/
static const PID_t G1 = {
	0,				// Kp (Don't care)
	0,				// Kd (Don't care)
	14000,			// 前進成分の制御量
	14000			// 旋回成分の制御量
};

static void TraceRun1(void) {
	int L, R, E;	// 左右センサ値、偏差

	// 赤外線センサのキャリブレーション
#if	(CALIBRATION_METHOD == 1)
	CalibrateIR_t cal;
	(void)CalibrateIR(&cal);
#else
	int offset = CalibrateIR(NULL);
#endif

	while (1) {
		L = ADC_READ(ADC_LEFT );	// 左赤外線センサ値を読み込む
		R = ADC_READ(ADC_RIGHT);	// 右赤外線センサ値を読み込む

#if	(CALIBRATION_METHOD == 1)
		L = NormalizeL(L, cal);		// 左赤外線センサ値を正規化する
		R = NormalizeR(R, cal);		// 右赤外線センサ値を正規化する
#else
		L -= offset;				// 左赤外線センサの原点を補正する
		R += offset;				// 右赤外線センサの原点を補正する
#endif

		// 中心からのずれを算出する
		E = L - R;

		// 右寄りのズレが大きければ左に曲げる
		if (E > 0 && L > IR_CENTER) {
			PWM_OUT(-G1.TURNING/2, G1.TURNING);
			LED(LED1);
		}

		// 左寄りのズレが大きければ右に曲げる
		else if (E < 0 && R > IR_CENTER) {
			PWM_OUT(G1.TURNING, -G1.TURNING/2);
			LED(LED1);
		}

		// 中央付近なら直進する
		else {
			PWM_OUT(G1.FORWARD, G1.FORWARD);
			LED(LED2);
		}
	}
}

/*----------------------------------------------------------------------
 * ライントレース - ON-OFF制御 + P制御
 *----------------------------------------------------------------------*/
static const PID_t G2 = {
	20,				// Kp
	0,				// Kd (Don't care)
	13000,			// FORWARD 前進成分の制御量
	12000			// TURNING 旋回成分の制御量
};

static void TraceRun2(void) {
	int L, R, E;	// 左右センサ値、偏差
	int P;			// 旋回の比例成分

	// 赤外線センサのキャリブレーション
	CalibrateIR_t cal;
	(void)CalibrateIR(&cal);

	while (1) {
		L = ADC_READ(ADC_LEFT );	// 左赤外線センサ値を読み込む
		R = ADC_READ(ADC_RIGHT);	// 右赤外線センサ値を読み込む

		L = NormalizeL(L, cal);		// 左赤外線センサ値を正規化する
		R = NormalizeR(R, cal);		// 右赤外線センサ値を正規化する

		// 中心からのずれを算出する
		E = L - R;

		// 旋回の比例成分を算出する
		P = G2.KP * ABS(E);
		P = MIN(MAX(P, -PWM_MAX), PWM_MAX);

		// 右寄りのズレが大きければ左に曲げる
		if (E > 0 && L > IR_CENTER) {
			PWM_OUT(-P, G2.TURNING + P);
			LED(LED1);
		}

		// 左寄りのズレが大きければ右に曲げる
		else if (E < 0 && R > IR_CENTER) {
			PWM_OUT(G2.TURNING + P, -P);
			LED(LED1);
		}

		// 中央付近なら直進する
		else {
			PWM_OUT(G2.FORWARD, G2.FORWARD);
			LED(LED2);
		}
	}
}

/*----------------------------------------------------------------------
 * ライントレース - PD制御
 *----------------------------------------------------------------------*/
#define	DT	1000	// 微分用の制御周期[sec]の逆数[Hz]
static const PID_t G3 = {
	28,				// Kp 比例項係数
	3,				// Kd 比例項係数
	20000,			// 前進成分の制御量 （0～MAX_PWM）
	32767			// 旋回成分の制御量 （0～MAX_PWM）
};

static void TraceRun3(void) {
	int L, R;		// 左右の赤外線センサ値
	int F, T;		// 前進成分、旋回成分の制御量
	int P, Q = 0;	// P項の元となる偏差、前回偏差
	int D;			// D項の元となる偏差の微分値

	// 赤外線センサのキャリブレーション
	CalibrateIR_t cal;
	(void)CalibrateIR(&cal);

	while (1) {
		L = ADC_READ(ADC_LEFT );	// 左赤外線センサ値を読み込む
		R = ADC_READ(ADC_RIGHT);	// 右赤外線センサ値を読み込む

		L = NormalizeL(L, cal);		// 左赤外線センサ値を正規化する
		R = NormalizeR(R, cal);		// 右赤外線センサ値を正規化する

		/*-----------------------------------------
		 * 旋回成分の算出 - PD制御量
		 *-----------------------------------------*/
		P = L - R; 					// 中央からの偏差を算出する
		D = (P - Q) * DT;			// 偏差の微分値を算出する
		Q = P;						// 前回偏差を今回値で更新する
		T = G3.KP * P + G3.KD * D;	// PD制御量を算出する
		T = MIN(MAX(T, -G3.TURNING), G3.TURNING);

		/*-----------------------------------------
		 * 前進成分の算出 - Duty 100% - |旋回成分|
		 *-----------------------------------------*/
		F = G3.FORWARD - ABS(T);

		/*-----------------------------------------
		 * 前進成分と旋回成分を足し合わせて走行
		 *-----------------------------------------*/
		PWM_OUT(F - T, F + T);

		LED(P > 0 ? LED2 : LED1);
	}
}

/*----------------------------------------------------------------------
 * ライントレース - PD制御 + 楽譜再生
 *----------------------------------------------------------------------*/
static void TraceRun4(void) {
#include "play.h"
	const MusicScore_t ms[] = {
#include "truth.dat"	// tempo = 155
	};

	PLAY_INIT();

	// バックグラウンドで演奏
	PLAY(ms, sizeof(ms) / sizeof(MusicScore_t), 155, -1);

	// ライントレース - PD制御
	TraceRun3();
}

/*----------------------------------------------------------------------
 * ライントレース
 * - runType
 *	0: 赤外線センサの特性計測
 *	1: ON-OFF制御によるライントレース
 *	2: ON-OFF制御＋P制御によるライントレース
 *	3: PD制御によるライントレース
 *	4: PD制御によるライントレース＋バックグラウンド演奏
 *----------------------------------------------------------------------*/
void TRACE_RUN(int runType) {
	TIMER_INIT();	// WAIT()
	GPIO_INIT();	// SW_STANDBY()
	ADC_INIT();		// A/D変換の初期化
	PWM_INIT();		// PWM出力の初期化

	LED(LED_ON);	// LEDを点灯させて
	SW_STANDBY();	// スイッチが押されるまで待機
	WAIT(500);		// 少し待ってからスタート

	switch (runType) {
	  case 0:	TraceRun0(); break;
	  case 1:	TraceRun1(); break;
	  case 2:	TraceRun2(); break;
	  case 3:	TraceRun3(); break;
	  default:	TraceRun4(); break;
	}
}
