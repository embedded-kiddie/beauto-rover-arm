/*===============================================================================
 * Name        : play.c
 * Author      : $(author)
 * Version     :
 * CPU type    : ARM Cortex-M3 LPC1343
 * Copyright   : $(copyright)
 * Description : Music player functions using CT32B0
 *===============================================================================*/
// Cortex Microcontroller Software Interface Standard
#ifdef __USE_CMSIS
#include "LPC13xx.h"
#endif

#include "type.h"
#include "play.h"
#include "ioport.h"

#if	PLAY_MODE != PLAY_PWM_WRC103

/*----------------------------------------------------------------------
 * 割込みハンドラ用変数 - バックグラウンド再生用変数
 *----------------------------------------------------------------------*/
#if	PLAY_MODE == PLAY_PWM_MODE
static unsigned long playPitch = 0;
#endif
static int countIRQ = 0;
static int scoreNum = 0;
static int baseTempo = 0;
static const MusicScore_t* scorePtr = 0;

/*----------------------------------------------------------------------
 * 割込みハンドラ用変数 - 繰り返し再生用変数
 *----------------------------------------------------------------------*/
static int loopPlay = 0;
static int loopNum  = 0;
static const MusicScore_t* loopPtr = 0;

/*----------------------------------------------------------------------
 * 割込みハンドラ用変数 - 休符フラグ
 *----------------------------------------------------------------------*/
#if	PLAY_MODE == PLAY_TIMER_MODE
static int playRest = 0;
#endif

/*----------------------------------------------------------------------
 * I/Oポート、タイマの初期化
 *----------------------------------------------------------------------*/
void PLAY_INIT(void) {
	/*-----------------------------------------
	 * I/O Configuration
	 *-----------------------------------------*/
	SetGpioDir(LPC_GPIO1, 8, 1);	// PIO1 ビット8（ブザー）を出力に設定

	// Selects function PIO1_8, no pull-down/pull-up,
	// Hysteresis disable, Standard GPIO output
	// 7.4.5 IOCON_PIO1_8 (PIO1_8/CT16B1_CAP0)
	LPC_IOCON->PIO1_8 = 0;

	/*-----------------------------------------
	 * Timer Configuration
	 *-----------------------------------------*/
	// 3.5.18 System AHB clock control register (SYSAHBCLKCTRL)
	// Bit 9 (CT32B0): Enables clock for 32-bit counter/timer 0
	// Note: LPC_SYSCON->SYSAHBCLKDIV = 1 --> PCLK = 72MHz
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<9);

	// 16.8.1 Interrupt Register (TMR32B0IR)
	// Reset the interrupt flag for MR0INT～MR3INT
	// Note: Writing '1' to the corresponding IR bit will reset the interrupt
	//       Writing '0' has no effect
	LPC_TMR32B0->IR = 0x1f; // bit 4:0

	// 16.8.2 Timer Control Register (TMR32B0TCR)
	// Bit 0 (CEN) : Disable TC and PC (Timer Stop)
	// Bit 1 (CRES): TC and PC remain reset
	LPC_TMR32B0->TCR = 0;

	// 16.8.4 PreScale Register (TMR32B0PR)
	// Bit 31:0 (PR): Maximum value for the PC
	// Note: LPC_SYSCON->SYSAHBCLKDIV = 1
	LPC_TMR32B0->PR = PLAY_PRE_SCALE - 1; // PCLK = 72MHz を前提とする

	// 16.8.11 Count Control Register (TMR32B0CTCR)
	// Bit 1:0 (CTM): Counter/Timer Mode
	LPC_TMR32B0->CTCR = 0; // 00 = Timer Mode: every rising PCLK edge

#if	PLAY_MODE == PLAY_TIMER_MODE

	// 16.8.6 Match Control Register (TMR32B0MCR)
	// Bit 9 (MR3I): Enable interrupt when MR3 matches TC
	// Bit10 (MR3R): Reset TC when MR3 matches TC
	LPC_TMR32B0->MCR = (1<<10|1<<9);

#else // PLAY_MODE == PLAY_PWM_MODE

	// 16.8.12 PWM Control Register (TMR32B0PWMC)
	// Bit 2 (PWMEN2): PWM mode is enabled for CT32Bn_MAT2
	// Bit 3 (PWMEN3): CT32Bn_MAT3 is controlled by EM3
	// Note: EM3 will do nothing
	LPC_TMR32B0->PWMC = (0<<3|1<<2);

	// 16.8.6 Match Control Register (TMR32B0MCR)
	// Bit 6 (MR2I): Enable interrupt when MR2 matches TC
	// Bit 9 (MR3I): Enable interrupt when MR3 matches TC
	// Bit10 (MR3R): Reset TC when MR3 matches TC
	LPC_TMR32B0->MCR = (1<<10|1<<9|1<<6);

	// 16.8.7 Match register (TMR32B0MRn)
	// Actions are controlled by the settings in the MCR register
	LPC_TMR32B0->MR2 = PLAY_PWM_CYCLE - 1;// MR2 match counter initial value
	LPC_TMR32B0->MR3 = PLAY_PWM_CYCLE - 1;// MR3 set cycle

#endif // PLAY_MODE
}

/*----------------------------------------------------------------------
 * 音符長のための割込み回数を算出する
 *----------------------------------------------------------------------*/
static int CalcInterval(const MusicScore_t *score) {
#if	PLAY_MODE == PLAY_TIMER_MODE

	/*-------------------------------------------------------------------
	 * 音符長に応じたマッチレジスタ（MR3）による割り込み回数の設定方法
	 *
	 * 再生する音階の周波数により、マッチレジスタの値が異なるため、
	 * 音階ごとに1音符あたりの再生時間を調整する必要がある。そこで
	 * 各音符長に応じた割り込みの発生回数（countIRQ）を算出する。
	 *
	 * 【1. 各音階の再生に必要なマッチレジスタの値を算出する】
	 *    PCLK（デフォルト72MHz）をPLAY_PRE_SCALEで分周すると
	 *    1カウントあたりの周波数f[Hz]と周期t[sec]は以下となる。
	 *
	 *      f = PCLK ÷ PRE_SCALE
	 *      t = 1/f = PRE_SCALE ÷ PCLK
	 *
	 *    ∴再生する音階の周波数F[Hz]に必要なマッチレジスタ（MR3）
	 *    の値pは以下となる。
	 *
	 *      p = 1/F ÷ t = PCLK ÷ (PRE_SCALE × F)
	 *
	 * 【2. 音符長から再生時間を算出する】
	 *    4分音符の1分間の拍数B(BASE_DURATION)を基準に、16分音符の
	 *    再生時間をT[sec]としたときの、N分音符と再生時間の関係は以下の通り。
	 *
	 *    N 分音符  	16分音符	8分音符	 4分音符	2分音符	 全音符
	 *    拍数／分  	B×4    	B×2   	 B×1   	B÷2   	 B÷4
	 *    再生時間  	T×1    	T×2   	 T×4   	T×8   	 T×16
	 *    倍数 n    	1       	2      	 4      	8      	 16
	 *
	 *    即ち、各音符長の再生時間Lは以下となる。
	 *
	 *      L = 60 ÷ (4 × BASE_DURATION) × N
	 *
	 *    また、1回のマッチに要する時間Sは
	 *
	 *      S = PRE_SCALE × p ÷ PCLK
	 *
	 *    となるため、MR3による割り込み発生回数Iは以下で表される。
	 *
	 *      I = L ÷ S
	 *        = 60 ÷ (4 × BASE_DURATION) × n ÷ (PRE_SCALE × p ÷ PCLK)
	 *        = 60 ÷ (4 × BASE_DURATION) × (PCLK ÷ PRE_SCALE ÷ p) × n
	 *
	 * 注1： PRE_SCALE は、ON-OFFの2回で1周期となるため、2倍する
	 * 注2： BASE_DURATION は同様の理由により、1/2とする
	 *-------------------------------------------------------------------*/
	int p = score->pitch ? score->pitch : Do4; // 休符の場合は代替の音階を設定

	// 整数演算によるオーバーフローを避けるため、除算を先に行う
	return (PLAY_PCLK / (2 * PLAY_PRE_SCALE * PLAY_PIT_SCALE * p)) * 60 / (2 * baseTempo) * score->duration;

#else // PLAY_MODE != PLAY_TIMER_MODE

	// PWMモードの場合は、ピッチによらずサイクルは一定
	return (PLAY_PCLK / (2 * PLAY_PRE_SCALE * PLAY_PWM_CYCLE    )) * 60 / (2 * baseTempo) * score->duration;

#endif // PLAY_MODE
}

/*----------------------------------------------------------------------
 * 楽譜の演奏
 * score: 楽譜データへのポインタ
 * num  : 楽譜データ中の音符数
 * tempo: 演奏する速さ（1分間に入る四分音符の数、メトロノーム記号）
 * loop : 0 = 繰り返し再生なし、0以外: loop[msec]後に再生を繰り返す
 *----------------------------------------------------------------------*/
void PLAY(const MusicScore_t *score, int num, int tempo, int loop) {
	unsigned long pitch;

	// バックグラウンド再生の設定
	scoreNum = num;
	scorePtr = score;
	baseTempo = tempo ? tempo : PLAY_BASE_TEMPO;
	countIRQ = CalcInterval(score);

	// 繰り返し再生の設定
	loopPlay = loop; // [msec]
	if (loopPtr == 0) {
		loopPtr = score;
		loopNum = num;
	}

#if	PLAY_MODE == PLAY_TIMER_MODE

	if (score->pitch > 0) {
		pitch = score->pitch * PLAY_PIT_SCALE - 1;
		playRest = 0;
	} else {
		pitch = Do4 * PLAY_PIT_SCALE - 1;
		playRest = 1; // 休符の場合
	}

	// 16.8.7 Match register (TMR32B0MRn)
	LPC_TMR32B0->MR3 = pitch;

#else // PLAY_MODE == PLAY_PWM_MODE

	// 16.8.7 Match register (TMR32B0MRn)
	playPitch = pitch = score->pitch * PLAY_PIT_SCALE - 1;

	LPC_TMR32B0->MR2 += pitch;// Add match counter to initial value

#endif // PLAY_MODE

	// 6.6.2 Interrupt Set-Enable Register 1
	// Bit 11 (ISE_CT32B0): Enable timer CT32B0 interrupt
	// Note: NVIC = Nested Vectored Interrupt Controller
	NVIC_EnableIRQ(TIMER_32_0_IRQn);

	// 16.8.2 Timer Control Register (TMR32B0TCR)
	LPC_TMR32B0->TCR = 2; // Bit1(CRES): TC and PC are synchronously reset on next PCLK positive edge
	LPC_TMR32B0->TCR = 1; // Bit0(CEN) : TC and PC are enabled for counting
}

/*----------------------------------------------------------------------
 * 演奏を停止する
 *----------------------------------------------------------------------*/
void PLAY_STOP(void) {
	// 6.6.4 Interrupt Clear-Enable Register 1 register
	// Bit 11 (ICE_CT32B0): Disable timer CT32B0 interrupt
	// Note: NVIC = Nested Vectored Interrupt Controller
	NVIC_DisableIRQ(TIMER_32_0_IRQn);

	// 16.8.2 Timer Control Register (TMR32B0TCR)
	// Bit 0 (CEN) : Disable TC and PC (Timer Stop)
	// Bit 1 (CRES): TC and PC remain reset
	LPC_TMR32B0->TCR = 0;

#if	PLAY_MODE == PLAY_PWM_MODE

	// 16.8.7 Match Registers (TMR32B0MRn)
	// Bit31:0(MATCH): Timer counter match value
	// Note: Initial value is required before timer start
	LPC_TMR32B0->MR2 = PLAY_PWM_CYCLE - 1;// MR2 match counter initial value
	LPC_TMR32B0->MR3 = PLAY_PWM_CYCLE - 1;// MR3 set cycle

#endif // PLAY_MODE
}

/*----------------------------------------------------------------------
 * 再生中か調べる
 *----------------------------------------------------------------------*/
int IS_PLAYING(void) {
	return LPC_TMR32B0->TCR & 1;
}

/*----------------------------------------------------------------------
 * 指定時間[msec]分のインターバルをとる
 *----------------------------------------------------------------------*/
static void PlayInterval(int msec) {
	const MusicScore_t interval = {Do5, N0};

	scorePtr = 0;
	countIRQ = CalcInterval(&interval) * msec * baseTempo / (1000 * 90);

#if	PLAY_MODE == PLAY_TIMER_MODE
	playRest = 1;
#endif

	// 6.6.2 Interrupt Set-Enable Register 1
	// Bit 11 (ISE_CT32B0): Enable timer CT32B0 interrupt
	// Note: NVIC = Nested Vectored Interrupt Controller
	NVIC_EnableIRQ(TIMER_32_0_IRQn);

	// 16.8.2 Timer Control Register (TMR32B0TCR)
	LPC_TMR32B0->TCR = 2; // Bit 1 (CRES): TC and PC are synchronously reset on next PCLK positive edge
	LPC_TMR32B0->TCR = 1; // Bit 0 (CEN) : TC and PC are enabled for counting
}

/*----------------------------------------------------------------------
 * マッチレジスタによる割込みハンドラ
 *----------------------------------------------------------------------*/
void TIMER32_0_IRQHandler(void) {
	static unsigned long toggle = 0;
	unsigned long IR = LPC_TMR32B0->IR;

	// 16.8.1 Interrupt Register (TMR32B0IR)
	// Reset the interrupt flag for MR0INT～MR3INT
	// Note: Writing '1' to the corresponding IR bit will reset the interrupt
	//       Writing '0' has no effect
	LPC_TMR32B0->IR = IR; // Required

	// バックグラウンド再生のシーケンス制御
	// 規定回数（countIRQ）分のPWMサイクル実行後に再生を停止し、楽譜データを進める
	// 楽譜データを最後まで再生した後に、指定時間（loop[msec]）後に再生を再開する
	if (IR & (1 << 3) && --countIRQ <= 0) {
		PLAY_STOP();

		if (scorePtr) {
			if (--scoreNum > 0) {
				// 楽譜データを次に進める
				PLAY(scorePtr + 1, scoreNum, baseTempo, loopPlay);
			} else if (loopPlay) {
				// 指定時間（loopPlay[msec]）だけインターバルをとる
				PlayInterval(loopPlay);
			}
		} else {
			// インターバル後（scorePtr=0 かつ countIRQ=0）に元データを設定し、再生を再開する
			PLAY(loopPtr, loopNum, baseTempo, loopPlay);
		}
	}

#if	PLAY_MODE == PLAY_TIMER_MODE

	// Toggle output
	else {
		// playRest=1の場合は無音にする
		SetGpioBit(LPC_GPIO1, 8, (playRest ? (toggle = 0) : (toggle = !toggle)));
	}

#else // PLAY_MODE == PLAY_PWM_MODE

	// PWM出力
	// scorePtr=0 または scorePtr->pitch=0 の場合は無音となる
	else if (LPC_TMR32B0->MR2 < LPC_TMR32B0->MR3) {
		if (scorePtr && scorePtr->pitch) {
			SetGpioBit(LPC_GPIO1, 8, (toggle = !toggle));
			LPC_TMR32B0->MR2 += playPitch;
		}
	}

	else if (LPC_TMR32B0->MR2 == LPC_TMR32B0->MR3) {
		if (scorePtr && scorePtr->pitch) {
			SetGpioBit(LPC_GPIO1, 8, (toggle = !toggle));
			LPC_TMR32B0->MR2 = playPitch;
		}
	}

	else {
		LPC_TMR32B0->MR2 -= LPC_TMR32B0->MR3;
	}

#endif // PLAY_MODE
}

#else // PLAY_MODE == PLAY_PWM_WRC103

/*======================================================================
 * ビュートローバーARM（VS-WRC103LV）に添付のオリジナルバージョン
 *
 * ※ バックグラウンド再生には未対応
 *======================================================================*/
#include "timer.h"

/*----------------------------------------------------------------------
 * 割込みハンドラ用変数
 *----------------------------------------------------------------------*/
static unsigned char bBuzzerFlag;
static unsigned int iBuzzerPitch;

/*----------------------------------------------------------------------
 * 割込みハンドラ用変数
 *----------------------------------------------------------------------*/
static void Timer32Init(void) {
	//Timer Stop
	LPC_TMR32B0->TCR &= ~0x01;

	//MAT2enable, MAT3 disable MAT2 is used Buzzer
	LPC_TMR32B0->PWMC = (0<<3|1<<2);

	//maximum value for the Prescale Counter. Divide Mainclk
	LPC_TMR32B0->PR = 0;

	//interrupt disable. TC clear MR3 matches TC reset enable
	LPC_TMR32B0->MCR = (1<<10|1<<9|1<<6);

	//interrupt flag clear
	LPC_TMR32B0->IR = 0;

	//PWM cycle set
	LPC_TMR32B0->MR3 = 0x00010000;	//MR3 set cycle
	LPC_TMR32B0->MR2 = 0x00011000;	//buzzer

	//TimerStart
	LPC_TMR32B0->TCR |= 0x01;
}

void TIMER32_0_IRQHandler(void) {

	//is Timer Reset
	static int8_t outbitvalue = 0;
	uint32_t tempIR = LPC_TMR32B0->IR;

	if (bBuzzerFlag == 1) {
		if (LPC_TMR32B0->MR2 < LPC_TMR32B0->MR3) {
			outbitvalue = (outbitvalue + 1) & 0x01;
			SetGpioBit(LPC_GPIO1, 8, (uint32_t) outbitvalue);
			LPC_TMR32B0->MR2 += iBuzzerPitch;
		} else if (LPC_TMR32B0->MR2 == LPC_TMR32B0->MR3) {
			outbitvalue = (outbitvalue + 1) & 0x01;
			SetGpioBit(LPC_GPIO1, 8, (uint32_t) outbitvalue);
			LPC_TMR32B0->MR2 = iBuzzerPitch;
		} else {
			LPC_TMR32B0->MR2 -= LPC_TMR32B0->MR3;
		}
	} else {
		NVIC_DisableIRQ(TIMER_32_0_IRQn);
	}
	LPC_TMR32B0->IR = tempIR;
}

//buzzer
void BuzzerSet(unsigned char pitch, unsigned char vol) {
	iBuzzerPitch = PLAY_PIT_SCALE * (unsigned int) pitch;
}

void BuzzerStart(void) {
	if (!isBuzzer()) {
		LPC_TMR32B0->MR2 += iBuzzerPitch;	//buzzer
		bBuzzerFlag = 1;
		LPC_TMR32B0->IR = 0;
		NVIC_EnableIRQ(TIMER_32_0_IRQn);
	}
}

void BuzzerStop(void) {
	LPC_TMR32B0->IR = 0;
	LPC_TMR32B0->MR2 = 0x00013000;	//buzzer
	bBuzzerFlag = 0;
}

unsigned char isBuzzer(void) {
	return bBuzzerFlag;
}

/*----------------------------------------------------------------------
 * I/Oポート、タイマの初期化
 *----------------------------------------------------------------------*/
void PLAY_INIT(void) {
	bBuzzerFlag = 0;

	SetGpioDir(LPC_GPIO1, 8, 1);	//set output port pin
	LPC_IOCON->PIO1_8 = 0;
	LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 9);

	Timer32Init();
}

/*----------------------------------------------------------------------
 * 楽譜の演奏
 * score: 楽譜データへのポインタ
 * num  : 楽譜データ中の音符数
 * loop : 0: 繰り返し再生なし、0以外: loop[msec]後に再生を繰り返す
 *----------------------------------------------------------------------*/
void PLAY(const MusicScore_t *score, int num, int tempo, int loop) {
	do {
		int i;
		for (i = 0; i < num; i++) {
			if (score[i].pitch) {
				BuzzerSet(score[i].pitch, 128);
				BuzzerStart();
			}
			WAIT((PLAY_PCLK / (2 * PLAY_PRE_SCALE * PLAY_PWM_CYCLE)) * 60 / (2 * tempo + 35) * score[i].duration);
			BuzzerStop();
		}
		WAIT(loop);
	} while (loop);
}

/*----------------------------------------------------------------------
 * 演奏を停止する
 *----------------------------------------------------------------------*/
void PLAY_STOP(void) {
	BuzzerStop();
}

/*----------------------------------------------------------------------
 * 再生中か調べる
 *----------------------------------------------------------------------*/
int IS_PLAYING(void) {
	return isBuzzer();
}

#endif // PLAY_MODE

#ifdef	EXAMPLE
/*===============================================================================
 * バックグラウンド演奏の動作確認
 * - フォアグラウンドでLチカ
 * - フォアグラウンド演奏としたい場合は、以下の様に IS_PLAYING() でブロックする
 *	PLAY(...);
 *	while (IS_PLAYING());
 *===============================================================================*/
#include "timer.h"
#include "ioport.h"
#include "play.h"

/*----------------------------------------------------------------------
 * バックグラウンド演奏の動作例
 *----------------------------------------------------------------------*/
void PLAY_EXAMPLE(void) {
	const MusicScore_t ms[] = {
//#include "doremi.dat"	// tempo = PLAY_BASE_DURATION
#include "truth.dat"	// tempo = 155
//#include "lupin.dat"	// tempo = 132
	};

	TIMER_INIT();	// WAIT()
	PORT_INIT();	// LED()
	PLAY_INIT();

	// バックグラウンドで演奏
	PLAY(ms, sizeof(ms) / sizeof(MusicScore_t), 155, -1);

	// フォアグラウンドでLチカ
	while (1) {
		LED(LED1); WAIT(250);
		LED(LED2); WAIT(250);
	}
}
#endif // EXAMPLE
