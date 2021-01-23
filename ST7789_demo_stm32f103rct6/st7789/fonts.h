#ifndef __FONT_H
#define __FONT_H

#include "stdint.h"

#define  USE_FONT_7X10
#define  USE_FONT_11X18
#define  USE_FONT_16X26

#define  USE_GB_16X16
#define  USE_GB_24X24
#define  USE_GB_32X32

#ifdef 	USE_GB_16X16

	#define GB16_FONT_BYTES		32	//一个汉字占用多少字节
	#define GB16_FONT_WIDTH		16	//汉字宽度
	#define GB16_FONT_HEIGHT	16	//汉字高度
	#define GB16_FONT_NUM		13	//汉字数量
	
typedef struct _GB16  // 16x16汉字字模数据结构
{
	char index[2];						// 汉字内码索引
	const char data[GB16_FONT_BYTES];	// 点阵码数据
	
}GB16_Typedef;
	
#endif

#ifdef 	USE_GB_24X24

	#define GB24_FONT_BYTES		72	//一个汉字占用多少字节
	#define GB24_FONT_WIDTH		24	//汉字宽度
	#define GB24_FONT_HEIGHT	24	//汉字高度	
	#define GB24_FONT_NUM		18	//汉字数量

typedef struct _GB24  // 24x24汉字字模数据结构
{
	char index[2];  					// 汉字内码索引
	const char data[GB24_FONT_BYTES];   // 点阵码数据
	
}GB24_Typedef;

#endif

#ifdef 	USE_GB_32X32

	#define GB32_FONT_BYTES		128	//一个汉字占用多少字节
	#define GB32_FONT_WIDTH		32	//汉字宽度
	#define GB32_FONT_HEIGHT	32	//汉字高度	
	#define GB32_FONT_NUM		19	//汉字数量

typedef struct _GB32  // 32x32汉字字模数据结构
{
	char index[2];  					// 汉字内码索引
	const char data[GB32_FONT_BYTES];   // 点阵码数据
	
}GB32_Typedef;

#endif


typedef struct {
    const uint8_t width;
    uint8_t height;
    const uint16_t *data;
} FontDef;


extern FontDef Font_7x10;
extern FontDef Font_11x18;
extern FontDef Font_16x26;

extern uint8_t ST7789_CHN_16x16[];
extern uint8_t ST7789_CHN_24x24[];
extern uint8_t ST7789_CHN_32x32[];

#ifdef 	USE_GB_16X16
extern const GB16_Typedef GB16x16[GB16_FONT_NUM];
#endif
#ifdef 	USE_GB_24X24
extern const GB24_Typedef GB24x24[GB24_FONT_NUM];
#endif
#ifdef 	USE_GB_32X32
extern const GB32_Typedef GB32x32[GB32_FONT_NUM];
#endif

//16-bit(RGB565) Image lib.
/*******************************************
 *             CAUTION:
 *   If the MCU onchip flash cannot
 *  store such huge image data,please
 *           do not use it.
 * These pics are for test purpose only.
 *******************************************/

/* 128x128 pixel RGB565 image */
extern const uint16_t saber[][128];

/* 240x240 pixel RGB565 image 
extern const uint16_t knky[][240];
extern const uint16_t tek[][240];
extern const uint16_t adi1[][240];
*/
#endif
