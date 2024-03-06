/*===============================================================================
 Name        : trace.h
 Author      : $(author)
 Version     :
 Model type  : VS-WRC103LV
 Copyright   : $(copyright)
 Description : Line Trace definitions
===============================================================================*/
#ifndef _TRACE_H_
#define _TRACE_H_

/*----------------------------------------------------------------------
 * 関数のプロトタイプ宣言
 * - runMode
 *	0: 赤外線センサの特性計測
 *	1: ON-OFF制御によるライントレース
 *	2: ON-OFF制御＋P制御によるライントレース
 *	3: PD制御によるライントレース
 *	4: PD制御によるライントレース＋バックグラウンド演奏
 *----------------------------------------------------------------------*/
extern void traceRun(int runMode);

/*----------------------------------------------------------------------
 * 赤外線(Infrared)センサの出力補正用パラメータ
 *----------------------------------------------------------------------*/
#define N_SAMPLES	64		// 観測データのサンプル数
#define MTR_POWER	0x4000	// モーター制御量（トレースラインを十分に跨ぐ程度の出力）
#define	IR_RANGE	750		// センサ出力の範囲（0～IR_RANGE）
#define	IR_CENTER	100		// 中央とみなせる赤外線センサの閾値
#define	IR_STOP		825		// 停止を判定するセンサレベル
#define	IR_ERROR	50		// 左右のバラつきから異常を判定する閾値

/*----------------------------------------------------------------------
 * 赤外線(Infrared)センサのキャリブレーションパラメータ
 * 　- センサ出力の正規化
 *	L = (L - offsetL) * gainL / 100
 *	R = (R - offsetR) * gainR / 100
 *----------------------------------------------------------------------*/
typedef struct {
	short center;	// 中央とみなせる左右間偏差の閾値
	short offset;	// 原点オフセットの平均値
	short offsetL;	// 左センサの原点位置調整用オフセット
	short offsetR;	// 右センサの原点位置調整用オフセット
	short gainL;	// 左センサ出力を正規化する補正係数[%]
	short gainR;	// 右センサ出力を正規化する補正係数[%]
} CalibrateIR_t;

/*----------------------------------------------------------------------
 * 赤外線センサ(Infrared)の出力値（デバッグ用）
 *----------------------------------------------------------------------*/
typedef struct {
	short L;
	short R;
} IRData_t;

/*----------------------------------------------------------------------
 * PID制御ゲインの定義
 *----------------------------------------------------------------------*/
typedef struct {
	int KP;			// Kp 比例項係数
	int KI;			// Ki 積分項係数
	int KD;			// Kd 微分項係数
	int FORWARD;	// 前進成分の制御量 （0～MAX_PWM）
	int TURNING;	// 旋回成分の制御量 （0～MAX_PWM）
} PID_t;

/*----------------------------------------------------------------------
 * コースマップの定義
 *----------------------------------------------------------------------*/
#define	MAP_EOD		0xffffffff	// End of Data for "count"
typedef struct {
	unsigned long	count;
	PID_t			pid;
} CourseMap_t;

#endif // _TRACE_H_
