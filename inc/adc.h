/*===============================================================================
 * Name        : adc.h
 * Author      : $(author)
 * Version     :
 * CPU type    : ARM Cortex-M3 LPC1343
 * Copyright   : $(copyright)
 * Description : A/D conversion definitions
 *===============================================================================*/
#ifndef	_ADC_H_
#define	_ADC_H_

/*----------------------------------------------------------------------
 * A/D変換のチャネル
 *----------------------------------------------------------------------*/
#define	ADC_CH0		(0)		// AN1, 左センサ
#define	ADC_CH1		(1)		// AN2, 右センサ
#define	ADC_LEFT	ADC_CH0	// AN1, 左センサ
#define	ADC_RIGHT	ADC_CH1	// AN2, 左センサ

/*----------------------------------------------------------------------
 * 関数のプロトタイプ宣言
 *----------------------------------------------------------------------*/
extern void ADC_INIT(void);
extern unsigned short ADC_READ(unsigned char ch);
extern void ADC_READ2(unsigned short *L, unsigned short *R);

#ifdef	EXAMPLE
/*===============================================================================
 * A/D変換の動作確認
 *	- バーストモードをサポート（ADC_MODE = ADC_MODE_BURST）
 *===============================================================================*/
extern void ADC_EXAMPLE(void);
#endif // EXAMPLE

#endif // _ADC_H_
