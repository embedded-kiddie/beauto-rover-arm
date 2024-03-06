/*===============================================================================
 * Name        : main.c
 * Author      : $(author)
 * Version     :
 * Model type  : VS-WRC103LV
 * Copyright   : $(copyright)
 * Description : main function
 *===============================================================================*/

/*----------------------------------------------------------------------
 * メイン関数
 *----------------------------------------------------------------------*/
int main(void) {
#if		0
	/*-----------------------------------------
	 * 汎用I/Oポートの動作確認
	 * - スイッチ監視とLED点滅
	 *-----------------------------------------*/
	extern void gpioExample(void);
	gpioExample();

#elif	0
	/*-----------------------------------------
	 * A/D変換の動作確認
	 *　- バーストモードをサポート
	 *-----------------------------------------*/
	extern void adcExample(void);
	adcExample();

#elif	0
	/*-----------------------------------------
	 * モーターPWM制御の動作確認
	 * - exampleType
	 *	1: 直進性を確認し、駆動系のゲインを調整する
	 *	2: 前進 --> 右旋回 --> 左旋回 --> 後退
	 *-----------------------------------------*/
	extern void pwmExample(int exampleType);
	pwmExample(1);

#elif	0
	/*-----------------------------------------
	 * USBシリアル通信の動作確認
	 * - exampleType
	 *	1: エコーバック
	 *	2: printf(), scanf()
	 *-----------------------------------------*/
	extern void sciExample(int exampleType);
	sciExample(2);

#elif	0
	/*-----------------------------------------
	 * ウォッチドッグタイマーの動作確認
	 * - exampleType
	 *	1: メイン処理中でウォッチドッグを叩く
	 *	2: SysTick割り込み中でウォッチドッグを叩く
	 *-----------------------------------------*/
	extern void wdtExample(int exampleType);
	wdtExample(1);

#elif	0
	/*-----------------------------------------
	 * パワーマネジメントユニットの動作確認
	 * - exampleType
	 *	1: Active mode　でのスリープ
	 *	2: Sleep/Deep-sleep/Deep Power-down
	 *-----------------------------------------*/
	extern void pmuExample(int exampleType);
	pmuExample(2);

#elif	0
	/*-----------------------------------------
	 * バックグランド演奏の動作確認
	 * - フォアグラウンドでLチカ
	 *-----------------------------------------*/
	extern void playExample(void);
	playExample();

#else
	/*-----------------------------------------
	 * ライントレース
	 * - runMode
	 *	0: 赤外線センサの特性計測
	 *	1: ON-OFF制御によるライントレース
	 *	2: ON-OFF制御＋P制御によるライントレース
	 *	3: PD制御によるライントレース
	 *-----------------------------------------*/
	extern void traceRun(int runMode);
	traceRun(4);

#endif

	return 0;
}
