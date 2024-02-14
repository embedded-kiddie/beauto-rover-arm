/*===============================================================================
 * Name        : adc.c
 * Author      : $(author)
 * Version     :
 * CPU type    : ARM Cortex-M3 LPC1343
 * Copyright   : $(copyright)
 * Description : A/D conversion functions
 *===============================================================================*/
// Cortex Microcontroller Software Interface Standard
#ifdef __USE_CMSIS
#include "LPC13xx.h"
#endif

#include "type.h"
#include "adc.h"

/*----------------------------------------------------------------------
 * A/D変換 - 動作設定の定義
 *----------------------------------------------------------------------*/
#define	ADC_MODE_SINGLE	(0)			// 単一モード
#define	ADC_MODE_BURST	(1)			// 連続モード

#if		0
#define	ADC_MODE		ADC_MODE_SINGLE	// 1回だけA/D変換
#else
#define	ADC_MODE		ADC_MODE_BURST	// 順次連続してA/D変換
#endif

// 20.6.2 A/D Global Data Register (AD0GDR)
// 20.6.4 A/D Data Registers (AD0DR0～AD0DR7)
// bit 30: OVERRUN, bit 31: DONE
#define ADC_OVERRUN		0x40000000
#define ADC_DONE		0x80000000

/*----------------------------------------------------------------------
 * A/D変換 - 初期化
 *----------------------------------------------------------------------*/
void ADC_INIT(void) {
	// 7.4.28 IOCON_R_PIO0_11
	// I/O configuration for pin R/PIO0_11/AD0/CT32B0_MAT3 (Reset value: 0xD0 = 1101 0000)
	// Bit 0:2 (FUNC)  : 001(Selects function PIO0_11)
	// Bit 4:3 (MODE)  : 00(No pull-down/pull-up resistor enabled)
	// Bit   5 (HYS)   : 0(Hysteresis disable)
	// Bit   7 (ADMODE): 0(Analog input mode)
	LPC_IOCON->R_PIO0_11 &= 0x67; // 0110 0111: Analog input mode,  pull-up disable
	LPC_IOCON->R_PIO0_11 |= 0x01; // Selects function PIO0_11

	// 7.4.29 IOCON_R_PIO1_0
	// I/O configuration for pin R/PIO1_0/AD1/CT32B1_CAP0 (Reset value: 0xD0 = 1101 0000)
	// Bit 0:2 (FUNC)  : 001(Selects function PIO0_11)
	// Bit 4:3 (MODE)  : 00(No pull-down/pull-up resistor enabled)
	// Bit   5 (HYS)   : 0(Hysteresis disable)
	// Bit   7 (ADMODE): 0(Analog input mode)
	LPC_IOCON->R_PIO1_0 &= 0x67; // 0110 0111: Analog input mode,  pull-up disable
	LPC_IOCON->R_PIO1_0 |= 0x01; // Selects function PIO0_11

	// 3.5.47 Power-down configuration register
	// Bit 4 (ADC_PD): 0=Powered, 1=Powered down
	LPC_SYSCON->PDRUNCFG &= ~(1<<4); // Disable Power down bit to the ADC block

	// 3.5.18 System AHB clock control register
	// Bit 13 (ADC): Enables clock for ADC, 0=Disable, 1=Enable
	// Note: LPC_SYSCON->SYSAHBCLKDIV = 1 --> PCLK = 72MHz
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<13); // Enable AHB clock to the ADC

	// 20.6.1 A/D Control Register (AD0CR)
	LPC_ADC->CR =

	// Bit 7:0 (SEL): Selects which of the AD7:0 pins is (are) to be sampled and converted
#if	(ADC_MODE == ADC_MODE_SINGLE)
	(1<<0) |	// BURST = 0, only one channel can be selected
#else
	(3<<0) |	// BURST = 1, any numbers of channels can be selected
#endif

	// Bit 15:8 (CLKDIV): PCLK is divided by CLKDIV +1 to produce the clock for the ADC
#if	0
	(47<<8) |	// CLKDIV = 72MHz ÷ (47 + 1) = 1.5MHz
#else
	(15<<8) |	// CLKDIV = 72MHz ÷ (15 + 1) = 4.5MHz
#endif

	// Bit 16 (BURST): Burst select
#if	(ADC_MODE == ADC_MODE_SINGLE)
	(0<<16) |	// BURST = 0, software-controlled mode
#else
	(1<<16) |	// BURST = 1, hardware scan mode
#endif

	(0<<17) |	// Bit 19:17 (CLKS) : CLKS  = 0, 11 clocks/10 bits
	(0<<24);	// Bit 26:24 (START): START = 0, No start
}

/*----------------------------------------------------------------------
 * A/D変換 - センサ値の読み込み
 *----------------------------------------------------------------------*/
unsigned short ADC_READ(unsigned char ch) {
	unsigned int DR; // Data Register

	// A/D変換のチャネルは 0 あるいは 1 のみ
	ch &= 0x01;

#if	(ADC_MODE == ADC_MODE_SINGLE)

	// 20.6.1 A/D Control Register (AD0CR)
	// Bit  7: 0 (SEL)  : Selects which of the AD7:0 pins is (are) to be sampled and converted
	// Bit 26:24 (START): START = 1, Start conversion now
	LPC_ADC->CR &= 0xFFFFFF00;// A/D変換前に書き込みが必要
	LPC_ADC->CR |= (1<<24)|(1<<ch);// 指定チャネルのA/D変換をスタート

	// 20.6.4 A/D Data Registers (AD0DR0～AD0DR7)
	switch (ch) {
	  case ADC_LEFT:
		while (!((DR = LPC_ADC->DR0) & ADC_DONE));
		break;
	  case ADC_RIGHT:
	  default:
		while (!((DR = LPC_ADC->DR1) & ADC_DONE));
		break;
	}

	/* Stop ADC now */
	LPC_ADC->CR &= 0xF8FFFFFF;

	if (DR & ADC_OVERRUN) {
		return 0;
	}

#else // ADC_MODE == ADC_MODE_BURST

	// 20.6.4 A/D Data Registers (AD0DR0～AD0DR7)
	switch (ch) {
	  case ADC_LEFT:
		DR = LPC_ADC->DR0;
		break;
	  case ADC_RIGHT:
	  default:
		DR = LPC_ADC->DR1;
		break;
	}

#endif // ADC_MODE

	// Bit 5:0 (Reserved), result is 10 bits
	return (unsigned short) ((DR >> 6) & 0x3FF);
}

/*----------------------------------------------------------------------
 * A/D変換 - 2チャンネル同時読み込み
 *----------------------------------------------------------------------*/
inline void ADC_READ2(unsigned short *L, unsigned short *R) {
#if	(ADC_MODE == ADC_MODE_SINGLE)

	*L = ADC_READ(ADC_LEFT );
	*R = ADC_READ(ADC_RIGHT);

#else // ADC_MODE == AD_MODE_BURST

	*L = (unsigned short) ((LPC_ADC->DR0 >> 6) & 0x3FF);
	*R = (unsigned short) ((LPC_ADC->DR1 >> 6) & 0x3FF);

#endif // ADC_MODE
}

#ifdef	EXAMPLE
/*===============================================================================
 * A/D変換の動作確認
 *	- バーストモードをサポート（ADC_MODE = ADC_MODE_BURST）
 *	- 赤外センサの出力特性
 *		・反射光低（黒地の反射率低） --> センサ出力 = 高
 *		・反射光高（白地の反射率高） --> センサ出力 = 低
 *	- 白地に黒線を跨いだ時
 *		・左寄り（右センサが黒線に近い） --> 左センサ出力 - 右センサ出力 < 0
 *		・右寄り（左センサが黒線に近い） --> 左センサ出力 - 右センサ出力 > 0
 *	- 座標系の考え方
 *		・「X = 左センサ出力 - 右センサ出力」とすると、黒線の中心を原点とし、
 *		  右向きがx軸の正の向きとなる座標系で自車の位置を表すことができる。
 *     Y
 *     ^
 *     |
 * ----+--->X
 *===============================================================================*/
#include "type.h"
#include "ioport.h"
#include "adc.h"

void ADC_EXAMPLE(void) {
	PORT_INIT();	// LED()
	ADC_INIT();

	while (1) {
		short X, C = 100;
		unsigned short L, R;

		L = ADC_READ(ADC_LEFT );
		R = ADC_READ(ADC_RIGHT);
		X = L - R;

		// 中央付近
		if (-C < X && X < C) {
			LED(LED_ON);
		}
		// 左寄り
		else if (X <= -C) {
			LED(LED1);
		}
		// 右寄り
		else if (C <= X) {
			LED(LED2);
		}
	}
}
#endif // EXAMPLE
