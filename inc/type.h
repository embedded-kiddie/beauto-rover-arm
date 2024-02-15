/*===============================================================================
 Name        : type.h
 Author      : $(author)
 Version     :
 Model type  : VS-WRC103LV
 Copyright   : $(copyright)
 Description : Standard type definitions
===============================================================================*/
#ifndef __TYPE_H__
#define __TYPE_H__

// CodeRed - ifdef for GNU added to avoid potential clash with stdint.h
#if defined   (  __GNUC__  )
#include <stdint.h>
#else

/* exact-width signed integer types */
typedef   signed          char int8_t;
typedef   signed short     int int16_t;
typedef   signed           int int32_t;
typedef   signed       __int64 int64_t;

 /* exact-width unsigned integer types */
typedef unsigned          char uint8_t;
typedef unsigned short     int uint16_t;
typedef unsigned           int uint32_t;
typedef unsigned       __int64 uint64_t;

#endif // __GNUC__ 

/*----------------------------------------------------------------------
 * マクロ定義
 *----------------------------------------------------------------------*/
#ifndef NULL
#define NULL    ((void *)0)
#endif

#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef TRUE
#define TRUE    (1)
#endif

#ifndef	MIN
#define	MIN(a, b)	((a) < (b) ? (a) : (b))
#endif

#ifndef	MAX
#define	MAX(a, b)	((a) > (b) ? (a) : (b))
#endif

#ifndef	ABS
#define	ABS(a)		((a) > (0) ? (a) : -(a))
#endif

#endif  /* __TYPE_H__ */
