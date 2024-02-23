/*===============================================================================
 * Name        : play.h
 * Author      : $(author)
 * Version     :
 * CPU type    : ARM Cortex-M3 LPC1343
 * Copyright   : $(copyright)
 * Description : Music player definitions
 *===============================================================================*/
#ifndef _PLAY_H_
#define	_PLAY_H_

/*----------------------------------------------------------------------
 * 音階再生アルゴリズムの選択
 *----------------------------------------------------------------------*/
#define	PLAY_TIMER_MODE	(0)	// マッチレジスタMR3のみを使用するタイマーモード
#define	PLAY_PWM_MODE	(1)	// マッチレジスタMR3とMR2を使用するPWMモード
#define	PLAY_PWM_WRC103	(2)	// ビュートローバーARMオリジナルのモード（バックグラウンド再生は不可）

#if		1
#define	PLAY_MODE		PLAY_TIMER_MODE
#elif	1
#define	PLAY_MODE		PLAY_PWM_MODE
#else
#define	PLAY_MODE		PLAY_PWM_WRC103
#endif

/*----------------------------------------------------------------------
 * Peripheral Clockの設定
 * LPC_SYSCON->SYSAHBCLKDIV = 1 --> PCLK = 72[MHz]
 *----------------------------------------------------------------------*/
#define	PLAY_PCLK		72000000

/*----------------------------------------------------------------------
 * ピッチとPWMサイクル長の設定
 *
 * 【音階の計算方法】
 * count = (PLAY_PCLK ÷ (2 × PLAY_PRE_SCALE)) ÷ 音程周波数
 * pitch = count ÷ PLAY_PIT_SCALE
 *
 * 注1：プリスケールレジスタは0からカウントされるので、PRには-1して設定する
 * 注2：PLAY_PRE_SCALEは、ON-OFFの2回で1周期となるため、2倍を設定する
 *
 * PLAY_PRE_SCALE: プリスケールカウンタ値 ... PCLKを分周するカウンタ値+1
 * PLAY_PIT_SCALE: ピッチ伸長係数         ... 8ビットの音階を伸長する係数
 * PLAY_PWM_CYCLE: PWMサイクル長          ... countより少し長く設定する
 *----------------------------------------------------------------------*/
#if	PLAY_MODE != PLAY_PWM_WRC103
#if		0
// 周波数の再現性：あまり良くない
#define	PLAY_PRE_SCALE	720
#define	PLAY_PIT_SCALE	1
#define	PLAY_PWM_CYCLE	0x0100
#elif	1
// 周波数の再現性：まぁまぁ
#define	PLAY_PRE_SCALE	180
#define	PLAY_PIT_SCALE	1
#define	PLAY_PWM_CYCLE	0x0400
#else
// 周波数の再現性：これ以上はあまり変化なし
#define	PLAY_PRE_SCALE	72
#define	PLAY_PIT_SCALE	1
#define	PLAY_PWM_CYCLE	0x1000
#endif
#else // PLAY_MODE == PLAY_PWM_WRC103
// 周波数の再現性：最高
#define	PLAY_PRE_SCALE	1
#define	PLAY_PIT_SCALE	384
#define	PLAY_PWM_CYCLE	0x10000
#endif // PLAY_MODE

/*----------------------------------------------------------------------
 * 演奏する速さ（1分間に入る四分音符の数、メトロノーム記号）
 * truth.dat = 155
 * lupin.dat = 132
 * idle.dat  = 128
 * eva.dat   = 126
 *----------------------------------------------------------------------*/
#define	PLAY_BASE_TEMPO	132		// Allegro（アレグロ） 快速に

/*----------------------------------------------------------------------
 * 楽譜データ
 *----------------------------------------------------------------------*/
typedef struct {
	unsigned short pitch;		// 音階
	unsigned short duration;	// 音符の長さ
} MusicScore_t;

/*----------------------------------------------------------------------
 * 関数のプロトタイプ宣言
 *----------------------------------------------------------------------*/
extern void playInit(void);
extern void playScore(const MusicScore_t *score, int num, int tempo, int loop);
extern void playStop(void);
extern int playIsPlaying(void);

#if	PLAY_MODE == PLAY_PWM_WRC103
extern void BuzzerSet(unsigned char pitch, unsigned char vol);
extern void BuzzerStart(void);
extern void BuzzerStop(void);
extern unsigned char isBuzzer(void);
#endif // PLAY_MODE

/*----------------------------------------------------------------------
 * 音階（pitch）の定義
 * ※ マッチカウンタ設定時に PLAY_PIT_SCALE 倍して -1 すること
 *----------------------------------------------------------------------*/
#if	PLAY_MODE != PLAY_PWM_WRC103
#if		PLAY_PRE_SCALE == 720
#define Do2     764 //   65.406[Hz]
#define Do2s    722 //   69.296[Hz]
#define Re2f    722 //   69.296[Hz]
#define Re2     681 //   73.416[Hz]
#define Re2s    643 //   77.782[Hz]
#define Mi2f    643 //   77.782[Hz]
#define Mi2     607 //   82.407[Hz]
#define Fa2     573 //   87.307[Hz]
#define Fa2s    541 //   92.499[Hz]
#define So2f    541 //   92.499[Hz]
#define So2     510 //   97.999[Hz]
#define So2s    482 //  103.826[Hz]
#define Ra2f    482 //  103.826[Hz]
#define Ra2     455 //  110.000[Hz]
#define Ra2s    429 //  116.541[Hz]
#define Si2f    429 //  116.541[Hz]
#define Si2     405 //  123.471[Hz]
#define Do3     382 //  130.813[Hz]
#define Do3s    361 //  138.591[Hz]
#define Re3f    361 //  138.591[Hz]
#define Re3     341 //  146.832[Hz]
#define Re3s    321 //  155.563[Hz]
#define Mi3f    321 //  155.563[Hz]
#define Mi3     303 //  164.814[Hz]
#define Fa3     286 //  174.614[Hz]
#define Fa3s    270 //  184.997[Hz]
#define So3f    270 //  184.997[Hz]
#define So3     255 //  195.998[Hz]
#define So3s    241 //  207.652[Hz]
#define Ra3     227 //  220.000[Hz]
#define Ra3s    215 //  233.082[Hz]
#define Si3f    215 //  233.082[Hz]
#define Si3     202 //  246.942[Hz]
#define Do4     191 //  261.626[Hz]
#define Do4s    180 //  277.183[Hz]
#define Re4f    180 //  277.183[Hz]
#define Re4     170 //  293.665[Hz]
#define Re4s    161 //  311.127[Hz]
#define Mi4f    161 //  311.127[Hz]
#define Mi4     152 //  329.628[Hz]
#define Fa4     143 //  349.228[Hz]
#define Fa4s    135 //  369.994[Hz]
#define So4f    135 //  369.994[Hz]
#define So4     128 //  391.995[Hz]
#define So4s    120 //  415.305[Hz]
#define Ra4f    120 //  415.305[Hz]
#define Ra4     114 //  440.000[Hz]
#define Ra4s    107 //  466.164[Hz]
#define Si4f    107 //  466.164[Hz]
#define Si4     101 //  493.883[Hz]
#define Do5     96  //  523.251[Hz]
#define Do5s    90  //  554.365[Hz]
#define Re5f    90  //  554.365[Hz]
#define Re5     85  //  587.330[Hz]
#define Re5s    80  //  622.254[Hz]
#define Mi5f    80  //  622.254[Hz]
#define Mi5     76  //  659.255[Hz]
#define Fa5     72  //  698.456[Hz]
#define Fa5s    68  //  739.989[Hz]
#define So5f    68  //  739.989[Hz]
#define So5     64  //  783.991[Hz]
#define So5s    60  //  830.609[Hz]
#define Ra5f    60  //  830.609[Hz]
#define Ra5     57  //  880.000[Hz]
#define Ra5s    54  //  932.328[Hz]
#define Si5f    54  //  932.328[Hz]
#define Si5     51  //  987.767[Hz]
#define Do6     48  // 1046.502[Hz]
#define Do6s    45  // 1108.731[Hz]
#define Re6f    45  // 1108.731[Hz]
#define Re6     43  // 1174.659[Hz]
#define Re6s    40  // 1244.508[Hz]
#define Mi6f    40  // 1244.508[Hz]
#define Mi6     38  // 1318.510[Hz]
#define Fa6     36  // 1396.913[Hz]
#define Fa6s    34  // 1479.978[Hz]
#define So6f    34  // 1479.978[Hz]
#define So6     32  // 1567.982[Hz]
#define So6s    30  // 1661.219[Hz]
#define Ra6f    30  // 1661.219[Hz]
#define Ra6     28  // 1760.000[Hz]
#define Ra6s    27  // 1864.655[Hz]
#define Si6f    27  // 1864.655[Hz]
#define Si6     25  // 1975.533[Hz]
#define Do7     24  // 2093.005[Hz]
#elif	PLAY_PRE_SCALE == 180
#define Do2     3058 //   65.406[Hz]
#define Do2s    2886 //   69.296[Hz]
#define Re2f    2886 //   69.296[Hz]
#define Re2     2724 //   73.416[Hz]
#define Re2s    2571 //   77.782[Hz]
#define Mi2f    2571 //   77.782[Hz]
#define Mi2     2427 //   82.407[Hz]
#define Fa2     2291 //   87.307[Hz]
#define Fa2s    2162 //   92.499[Hz]
#define So2f    2162 //   92.499[Hz]
#define So2     2041 //   97.999[Hz]
#define So2s    1926 //  103.826[Hz]
#define Ra2f    1926 //  103.826[Hz]
#define Ra2     1818 //  110.000[Hz]
#define Ra2s    1716 //  116.541[Hz]
#define Si2f    1716 //  116.541[Hz]
#define Si2     1620 //  123.471[Hz]
#define Do3     1529 //  130.813[Hz]
#define Do3s    1443 //  138.591[Hz]
#define Re3f    1443 //  138.591[Hz]
#define Re3     1362 //  146.832[Hz]
#define Re3s    1286 //  155.563[Hz]
#define Mi3f    1286 //  155.563[Hz]
#define Mi3     1213 //  164.814[Hz]
#define Fa3     1145 //  174.614[Hz]
#define Fa3s    1081 //  184.997[Hz]
#define So3f    1081 //  184.997[Hz]
#define So3     1020 //  195.998[Hz]
#define So3s    963 //  207.652[Hz]
#define Ra3     909 //  220.000[Hz]
#define Ra3s    858 //  233.082[Hz]
#define Si3f    858 //  233.082[Hz]
#define Si3     810 //  246.942[Hz]
#define Do4     764 //  261.626[Hz]
#define Do4s    722 //  277.183[Hz]
#define Re4f    722 //  277.183[Hz]
#define Re4     681 //  293.665[Hz]
#define Re4s    643 //  311.127[Hz]
#define Mi4f    643 //  311.127[Hz]
#define Mi4     607 //  329.628[Hz]
#define Fa4     573 //  349.228[Hz]
#define Fa4s    541 //  369.994[Hz]
#define So4f    541 //  369.994[Hz]
#define So4     510 //  391.995[Hz]
#define So4s    482 //  415.305[Hz]
#define Ra4f    482 //  415.305[Hz]
#define Ra4     455 //  440.000[Hz]
#define Ra4s    429 //  466.164[Hz]
#define Si4f    429 //  466.164[Hz]
#define Si4     405 //  493.883[Hz]
#define Do5     382 //  523.251[Hz]
#define Do5s    361 //  554.365[Hz]
#define Re5f    361 //  554.365[Hz]
#define Re5     341 //  587.330[Hz]
#define Re5s    321 //  622.254[Hz]
#define Mi5f    321 //  622.254[Hz]
#define Mi5     303 //  659.255[Hz]
#define Fa5     286 //  698.456[Hz]
#define Fa5s    270 //  739.989[Hz]
#define So5f    270 //  739.989[Hz]
#define So5     255 //  783.991[Hz]
#define So5s    241 //  830.609[Hz]
#define Ra5f    241 //  830.609[Hz]
#define Ra5     227 //  880.000[Hz]
#define Ra5s    215 //  932.328[Hz]
#define Si5f    215 //  932.328[Hz]
#define Si5     202 //  987.767[Hz]
#define Do6     191 // 1046.502[Hz]
#define Do6s    180 // 1108.731[Hz]
#define Re6f    180 // 1108.731[Hz]
#define Re6     170 // 1174.659[Hz]
#define Re6s    161 // 1244.508[Hz]
#define Mi6f    161 // 1244.508[Hz]
#define Mi6     152 // 1318.510[Hz]
#define Fa6     143 // 1396.913[Hz]
#define Fa6s    135 // 1479.978[Hz]
#define So6f    135 // 1479.978[Hz]
#define So6     128 // 1567.982[Hz]
#define So6s    120 // 1661.219[Hz]
#define Ra6f    120 // 1661.219[Hz]
#define Ra6     114 // 1760.000[Hz]
#define Ra6s    107 // 1864.655[Hz]
#define Si6f    107 // 1864.655[Hz]
#define Si6     101 // 1975.533[Hz]
#define Do7     96  // 2093.005[Hz]
#else	// PLAY_PRE_SCALE == 72
#define Do2     7645 //   65.406[Hz]
#define Do2s    7215 //   69.296[Hz]
#define Re2f    7215 //   69.296[Hz]
#define Re2     6811 //   73.416[Hz]
#define Re2s    6428 //   77.782[Hz]
#define Mi2f    6428 //   77.782[Hz]
#define Mi2     6067 //   82.407[Hz]
#define Fa2     5727 //   87.307[Hz]
#define Fa2s    5405 //   92.499[Hz]
#define So2f    5405 //   92.499[Hz]
#define So2     5102 //   97.999[Hz]
#define So2s    4816 //  103.826[Hz]
#define Ra2f    4816 //  103.826[Hz]
#define Ra2     4545 //  110.000[Hz]
#define Ra2s    4290 //  116.541[Hz]
#define Si2f    4290 //  116.541[Hz]
#define Si2     4050 //  123.471[Hz]
#define Do3     3822 //  130.813[Hz]
#define Do3s    3608 //  138.591[Hz]
#define Re3f    3608 //  138.591[Hz]
#define Re3     3405 //  146.832[Hz]
#define Re3s    3214 //  155.563[Hz]
#define Mi3f    3214 //  155.563[Hz]
#define Mi3     3034 //  164.814[Hz]
#define Fa3     2863 //  174.614[Hz]
#define Fa3s    2703 //  184.997[Hz]
#define So3f    2703 //  184.997[Hz]
#define So3     2551 //  195.998[Hz]
#define So3s    2408 //  207.652[Hz]
#define Ra3     2273 //  220.000[Hz]
#define Ra3s    2145 //  233.082[Hz]
#define Si3f    2145 //  233.082[Hz]
#define Si3     2025 //  246.942[Hz]
#define Do4     1911 //  261.626[Hz]
#define Do4s    1804 //  277.183[Hz]
#define Re4f    1804 //  277.183[Hz]
#define Re4     1703 //  293.665[Hz]
#define Re4s    1607 //  311.127[Hz]
#define Mi4f    1607 //  311.127[Hz]
#define Mi4     1517 //  329.628[Hz]
#define Fa4     1432 //  349.228[Hz]
#define Fa4s    1351 //  369.994[Hz]
#define So4f    1351 //  369.994[Hz]
#define So4     1276 //  391.995[Hz]
#define So4s    1204 //  415.305[Hz]
#define Ra4f    1204 //  415.305[Hz]
#define Ra4     1136 //  440.000[Hz]
#define Ra4s    1073 //  466.164[Hz]
#define Si4f    1073 //  466.164[Hz]
#define Si4     1012 //  493.883[Hz]
#define Do5     956 //  523.251[Hz]
#define Do5s    902 //  554.365[Hz]
#define Re5f    902 //  554.365[Hz]
#define Re5     851 //  587.330[Hz]
#define Re5s    804 //  622.254[Hz]
#define Mi5f    804 //  622.254[Hz]
#define Mi5     758 //  659.255[Hz]
#define Fa5     716 //  698.456[Hz]
#define Fa5s    676 //  739.989[Hz]
#define So5f    676 //  739.989[Hz]
#define So5     638 //  783.991[Hz]
#define So5s    602 //  830.609[Hz]
#define Ra5f    602 //  830.609[Hz]
#define Ra5     568 //  880.000[Hz]
#define Ra5s    536 //  932.328[Hz]
#define Si5f    536 //  932.328[Hz]
#define Si5     506 //  987.767[Hz]
#define Do6     478 // 1046.502[Hz]
#define Do6s    451 // 1108.731[Hz]
#define Re6f    451 // 1108.731[Hz]
#define Re6     426 // 1174.659[Hz]
#define Re6s    402 // 1244.508[Hz]
#define Mi6f    402 // 1244.508[Hz]
#define Mi6     379 // 1318.510[Hz]
#define Fa6     358 // 1396.913[Hz]
#define Fa6s    338 // 1479.978[Hz]
#define So6f    338 // 1479.978[Hz]
#define So6     319 // 1567.982[Hz]
#define So6s    301 // 1661.219[Hz]
#define Ra6f    301 // 1661.219[Hz]
#define Ra6     284 // 1760.000[Hz]
#define Ra6s    268 // 1864.655[Hz]
#define Si6f    268 // 1864.655[Hz]
#define Si6     253 // 1975.533[Hz]
#define Do7     239 // 2093.005[Hz]
#endif	// PLAY_PRE_SCALE
#else // PLAY_MODE == PLAY_PWM_WRC103
#define Do2     1433 //   65.406[Hz]
#define Do2s    1353 //   69.296[Hz]
#define Re2f    1353 //   69.296[Hz]
#define Re2     1277 //   73.416[Hz]
#define Re2s    1205 //   77.782[Hz]
#define Mi2f    1205 //   77.782[Hz]
#define Mi2     1138 //   82.407[Hz]
#define Fa2     1074 //   87.307[Hz]
#define Fa2s    1014 //   92.499[Hz]
#define So2f    1014 //   92.499[Hz]
#define So2     957 //   97.999[Hz]
#define So2s    903 //  103.826[Hz]
#define Ra2f    903 //  103.826[Hz]
#define Ra2     852 //  110.000[Hz]
#define Ra2s    804 //  116.541[Hz]
#define Si2f    804 //  116.541[Hz]
#define Si2     759 //  123.471[Hz]
#define Do3     717 //  130.813[Hz]
#define Do3s    676 //  138.591[Hz]
#define Re3f    676 //  138.591[Hz]
#define Re3     638 //  146.832[Hz]
#define Re3s    603 //  155.563[Hz]
#define Mi3f    603 //  155.563[Hz]
#define Mi3     569 //  164.814[Hz]
#define Fa3     537 //  174.614[Hz]
#define Fa3s    507 //  184.997[Hz]
#define So3f    507 //  184.997[Hz]
#define So3     478 //  195.998[Hz]
#define So3s    451 //  207.652[Hz]
#define Ra3     426 //  220.000[Hz]
#define Ra3s    402 //  233.082[Hz]
#define Si3f    402 //  233.082[Hz]
#define Si3     380 //  246.942[Hz]
#define Do4     358 //  261.626[Hz]
#define Do4s    338 //  277.183[Hz]
#define Re4f    338 //  277.183[Hz]
#define Re4     319 //  293.665[Hz]
#define Re4s    301 //  311.127[Hz]
#define Mi4f    301 //  311.127[Hz]
#define Mi4     284 //  329.628[Hz]
#define Fa4     268 //  349.228[Hz]
#define Fa4s    253 //  369.994[Hz]
#define So4f    253 //  369.994[Hz]
#define So4     239 //  391.995[Hz]
#define So4s    226 //  415.305[Hz]
#define Ra4f    226 //  415.305[Hz]
#define Ra4     213 //  440.000[Hz]
#define Ra4s    201 //  466.164[Hz]
#define Si4f    201 //  466.164[Hz]
#define Si4     190 //  493.883[Hz]
#define Do5     179 //  523.251[Hz]
#define Do5s    169 //  554.365[Hz]
#define Re5f    169 //  554.365[Hz]
#define Re5     160 //  587.330[Hz]
#define Re5s    151 //  622.254[Hz]
#define Mi5f    151 //  622.254[Hz]
#define Mi5     142 //  659.255[Hz]
#define Fa5     134 //  698.456[Hz]
#define Fa5s    127 //  739.989[Hz]
#define So5f    127 //  739.989[Hz]
#define So5     120 //  783.991[Hz]
#define So5s    113 //  830.609[Hz]
#define Ra5f    113 //  830.609[Hz]
#define Ra5     107 //  880.000[Hz]
#define Ra5s    101 //  932.328[Hz]
#define Si5f    101 //  932.328[Hz]
#define Si5     95  //  987.767[Hz]
#define Do6     90  // 1046.502[Hz]
#define Do6s    85  // 1108.731[Hz]
#define Re6f    85  // 1108.731[Hz]
#define Re6     80  // 1174.659[Hz]
#define Re6s    75  // 1244.508[Hz]
#define Mi6f    75  // 1244.508[Hz]
#define Mi6     71  // 1318.510[Hz]
#define Fa6     67  // 1396.913[Hz]
#define Fa6s    63  // 1479.978[Hz]
#define So6f    63  // 1479.978[Hz]
#define So6     60  // 1567.982[Hz]
#define So6s    56  // 1661.219[Hz]
#define Ra6f    56  // 1661.219[Hz]
#define Ra6     53  // 1760.000[Hz]
#define Ra6s    50  // 1864.655[Hz]
#define Si6f    50  // 1864.655[Hz]
#define Si6     47  // 1975.533[Hz]
#define Do7     45  // 2093.005[Hz]
#endif // PLAY_MODE

/*----------------------------------------------------------------------
 * 音符長（note）の定義
 * 「16分音符」を 1 としたときの各音符の長さを定義する
 *----------------------------------------------------------------------*/
#define  N16     1   // 16分音符
#define  N8      2   //  8分音符
#define  N8x     3   //8.5分音符
#define  N4      4   //  4分音符
#define  N4x     6   //4.5分音符
#define  N2      8   //  2分音符
#define  N2x     12  //2.5分音符
#define  N0      16  //   全音符

#define  N84     6   // N8 + N4
#define  N83     8   // N8 + N3
#define  N82     10  // N8 + N2
#define  N81     14  // N8 + N1
#define  N80     18  // N8 + N0

#ifdef	EXAMPLE
/*===============================================================================
 * バックグラウンド演奏の動作確認
 * - フォアグラウンドでLチカ
 * - フォアグラウンド演奏としたい場合は、以下の様に playIsPlaying() でブロックする
 *	playScore(...);
 *	while (playIsPlaying());
 *===============================================================================*/
extern void playExample(void);
#endif // EXAMPLE

#endif // _PLAY_H_
