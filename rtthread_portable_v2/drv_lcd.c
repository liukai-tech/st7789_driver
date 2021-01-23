/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-01-21     liukai       the first version(使用rt-thread studio打开fonts.c/.h中文会存在乱码，待解决。--2020/01/21)
 */
#include <rtdevice.h>
#include <board.h>
#include "drv_spi.h"
#include "drv_lcd.h"
//#include "drv_lcd_font.h"

#define DBG_TAG "drv.st7789"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define SPI_BUS_NAME "spi2"             //SPI设备总线名称
#define SPI_ST7789_DEVICE_NAME "spi20"  //SPI设备名称

#define LCD_BLK_PIN     GET_PIN(C, 4)
#define LCD_DC_PIN      GET_PIN(C, 5)
#define LCD_RES_PIN     GET_PIN(B, 0)
#define LCD_CS_PIN      GET_PIN(D, 7)

/* Basic operations */
#define ST7789_RST_Clr() rt_pin_write(LCD_RES_PIN, PIN_LOW)
#define ST7789_RST_Set() rt_pin_write(LCD_RES_PIN, PIN_HIGH)

#define ST7789_DC_Clr() rt_pin_write(LCD_DC_PIN, PIN_LOW)
#define ST7789_DC_Set() rt_pin_write(LCD_DC_PIN, PIN_HIGH)

#define ST7789_Select() rt_pin_write(LCD_CS_PIN, PIN_LOW)
#define ST7789_UnSelect() rt_pin_write(LCD_CS_PIN, PIN_HIGH)

#define ST7789_BLK_Off() rt_pin_write(LCD_CS_PIN, PIN_LOW)
#define ST7789_BLK_On() rt_pin_write(LCD_CS_PIN, PIN_HIGH)


static struct rt_spi_device *spi_dev_lcd;

static int rt_hw_lcd_config(void)
{
    spi_dev_lcd = (struct rt_spi_device *)rt_device_find(SPI_ST7789_DEVICE_NAME);

    /* config spi */
    {
        struct rt_spi_configuration cfg;
        cfg.data_width = 8;
        cfg.mode = RT_SPI_MASTER | RT_SPI_MODE_2 | RT_SPI_MSB;/* 原潘多拉此处配置为RT_SPI_MODE_0 */
        cfg.max_hz = 25 * 1000 * 1000; /* 25M,SPI max 25MHz,lcd 4-wire spi */

        rt_spi_configure(spi_dev_lcd, &cfg);
    }

    return RT_EOK;
}

/**
 * @brief Write command to ST7789 controller
 * @param cmd -> command to write
 * @return none
 */
static rt_err_t ST7789_WriteCommand(const rt_uint8_t cmd)
{
    rt_size_t len;

    ST7789_Select();
    ST7789_DC_Clr();

    len = rt_spi_send(spi_dev_lcd, &cmd, 1);

    ST7789_UnSelect();

    if (len != 1)
    {
        LOG_I("lcd_write_cmd error. %d", len);
        return -RT_ERROR;
    }
    else
    {
        return RT_EOK;
    }
}

/**
 * @brief Write data to ST7789 controller, simplify for 8bit data.
 * data -> data to write
 * @return none
 */
static rt_err_t ST7789_WriteSmallData(const rt_uint8_t data)
{
    rt_size_t len;

    ST7789_Select();
    ST7789_DC_Set();

    len = rt_spi_send(spi_dev_lcd, &data, 1);

    ST7789_UnSelect();

    if (len != 1)
    {
        LOG_I("lcd_write_data error. %d", len);
        return -RT_ERROR;
    }
    else
    {
        return RT_EOK;
    }
}

/**
 * @brief Write data to ST7789 controller
 * @param buff -> pointer of data buffer
 * @param buff_size -> size of the data buffer
 * @return none
 */
static rt_err_t ST7789_WriteData(const rt_uint8_t *buff, rt_size_t buff_size)
{
    rt_size_t len;

    ST7789_Select();
    ST7789_DC_Set();

    // split data in small chunks because HAL can't send more than 64K at once

    while (buff_size > 0) {
        uint16_t chunk_size = buff_size > 65535 ? 65535 : buff_size;
        len = rt_spi_send(spi_dev_lcd, buff, chunk_size);

        if (len != chunk_size)
        {
            ST7789_UnSelect();
            LOG_I("lcd_write_data error. %d", len);
            return -RT_ERROR;
        }
        buff += chunk_size;
        buff_size -= chunk_size;
    }
    ST7789_UnSelect();

    return RT_EOK;
}

static void lcd_gpio_init(void)
{
    rt_hw_lcd_config();

    rt_pin_mode(LCD_CS_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(LCD_DC_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(LCD_RES_PIN, PIN_MODE_OUTPUT);

    rt_pin_mode(LCD_BLK_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(LCD_BLK_PIN, PIN_LOW);

    rt_pin_write(LCD_RES_PIN, PIN_LOW);
    //wait at least 100ms for reset
    rt_thread_delay(RT_TICK_PER_SECOND / 10);
    rt_pin_write(LCD_RES_PIN, PIN_HIGH);
}



/**
 * @brief Set address of DisplayWindow
 * @param xi&yi -> coordinates of window
 * @return none
 */
static void ST7789_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    ST7789_Select();
    uint16_t x_start = x0 + X_SHIFT, x_end = x1 + X_SHIFT;
    uint16_t y_start = y0 + Y_SHIFT, y_end = y1 + Y_SHIFT;

    /* Column Address set */
    ST7789_WriteCommand(ST7789_CASET);
    {
        uint8_t data[] = {x_start >> 8, x_start & 0xFF, x_end >> 8, x_end & 0xFF};
        ST7789_WriteData(data, sizeof(data));
    }

    /* Row Address set */
    ST7789_WriteCommand(ST7789_RASET);
    {
        uint8_t data[] = {y_start >> 8, y_start & 0xFF, y_end >> 8, y_end & 0xFF};
        ST7789_WriteData(data, sizeof(data));
    }
    /* Write to RAM */
    ST7789_WriteCommand(ST7789_RAMWR);
    ST7789_UnSelect();
}



static int rt_hw_lcd_init(void)
{
    __HAL_RCC_GPIOD_CLK_ENABLE();
    rt_hw_spi_device_attach(SPI_BUS_NAME, SPI_ST7789_DEVICE_NAME, GPIOD, GPIO_PIN_7);
    lcd_gpio_init();

#if 0

    ST7789_WriteCommand(ST7789_COLMOD);     //  Set color mode
    ST7789_WriteSmallData(ST7789_COLOR_MODE_16bit);
    ST7789_WriteCommand(0xB2);              //  Porch control
    {
        uint8_t data[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
        ST7789_WriteData(data, sizeof(data));
    }
    ST7789_SetRotation(ST7789_ROTATION);    //  MADCTL (Display Rotation)

    /* Internal LCD Voltage generator settings */
    ST7789_WriteCommand(0XB7);              //  Gate Control
    ST7789_WriteSmallData(0x35);            //  Default value
    ST7789_WriteCommand(0xBB);              //  VCOM setting
    ST7789_WriteSmallData(0x19);            //  0.725v (default 0.75v for 0x20)
    ST7789_WriteCommand(0xC0);              //  LCMCTRL
    ST7789_WriteSmallData (0x2C);           //  Default value
    ST7789_WriteCommand (0xC2);             //  VDV and VRH command Enable
    ST7789_WriteSmallData (0x01);           //  Default value
    ST7789_WriteCommand (0xC3);             //  VRH set
    ST7789_WriteSmallData (0x12);           //  +-4.45v (defalut +-4.1v for 0x0B)
    ST7789_WriteCommand (0xC4);             //  VDV set
    ST7789_WriteSmallData (0x20);           //  Default value
    ST7789_WriteCommand (0xC6);             //  Frame rate control in normal mode
    ST7789_WriteSmallData (0x0F);           //  Default value (60HZ)
    ST7789_WriteCommand (0xD0);             //  Power control
    ST7789_WriteSmallData (0xA4);           //  Default value
    ST7789_WriteSmallData (0xA1);           //  Default value
    /**************** Division line ****************/

    ST7789_WriteCommand(0xE0);
    {
        uint8_t data[] = {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23};
        ST7789_WriteData(data, sizeof(data));
    }

    ST7789_WriteCommand(0xE1);
    {
        uint8_t data[] = {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23};
        ST7789_WriteData(data, sizeof(data));
    }
    ST7789_WriteCommand (ST7789_INVON);     //  Inversion ON
    ST7789_WriteCommand (ST7789_SLPOUT);    //  Out of sleep mode
    ST7789_WriteCommand (ST7789_NORON);     //  Normal Display on
    ST7789_WriteCommand (ST7789_DISPON);    //  Main screen turned on

    /* wait for power stability */
    rt_thread_mdelay(100);

    ST7789_Fill_Color(BLACK);               //  Fill with Black.

    /* Turn on backlight */
    ST7789_BLK_On();

#else

    /* Memory Data Access Control */
    ST7789_WriteCommand(0x36);
    ST7789_WriteSmallData(0x00);
    /* RGB 5-6-5-bit  */
    ST7789_WriteCommand(0x3A);
    ST7789_WriteSmallData(0x65);
    /* Porch Setting */
    ST7789_WriteCommand(0xB2);
    ST7789_WriteSmallData(0x0C);
    ST7789_WriteSmallData(0x0C);
    ST7789_WriteSmallData(0x00);
    ST7789_WriteSmallData(0x33);
    ST7789_WriteSmallData(0x33);
    /*  Gate Control */
    ST7789_WriteCommand(0xB7);
    ST7789_WriteSmallData(0x35);
    /* VCOM Setting */
    ST7789_WriteCommand(0xBB);
    ST7789_WriteSmallData(0x19);
    /* LCM Control */
    ST7789_WriteCommand(0xC0);
    ST7789_WriteSmallData(0x2C);
    /* VDV and VRH Command Enable */
    ST7789_WriteCommand(0xC2);
    ST7789_WriteSmallData(0x01);
    /* VRH Set */
    ST7789_WriteCommand(0xC3);
    ST7789_WriteSmallData(0x12);
    /* VDV Set */
    ST7789_WriteCommand(0xC4);
    ST7789_WriteSmallData(0x20);
    /* Frame Rate Control in Normal Mode */
    ST7789_WriteCommand(0xC6);
    ST7789_WriteSmallData(0x0F);
    /* Power Control 1 */
    ST7789_WriteCommand(0xD0);
    ST7789_WriteSmallData(0xA4);
    ST7789_WriteSmallData(0xA1);
    /* Positive Voltage Gamma Control */
    ST7789_WriteCommand(0xE0);
    ST7789_WriteSmallData(0xD0);
    ST7789_WriteSmallData(0x04);
    ST7789_WriteSmallData(0x0D);
    ST7789_WriteSmallData(0x11);
    ST7789_WriteSmallData(0x13);
    ST7789_WriteSmallData(0x2B);
    ST7789_WriteSmallData(0x3F);
    ST7789_WriteSmallData(0x54);
    ST7789_WriteSmallData(0x4C);
    ST7789_WriteSmallData(0x18);
    ST7789_WriteSmallData(0x0D);
    ST7789_WriteSmallData(0x0B);
    ST7789_WriteSmallData(0x1F);
    ST7789_WriteSmallData(0x23);
    /* Negative Voltage Gamma Control */
    ST7789_WriteCommand(0xE1);
    ST7789_WriteSmallData(0xD0);
    ST7789_WriteSmallData(0x04);
    ST7789_WriteSmallData(0x0C);
    ST7789_WriteSmallData(0x11);
    ST7789_WriteSmallData(0x13);
    ST7789_WriteSmallData(0x2C);
    ST7789_WriteSmallData(0x3F);
    ST7789_WriteSmallData(0x44);
    ST7789_WriteSmallData(0x51);
    ST7789_WriteSmallData(0x2F);
    ST7789_WriteSmallData(0x1F);
    ST7789_WriteSmallData(0x1F);
    ST7789_WriteSmallData(0x20);
    ST7789_WriteSmallData(0x23);
    /* Display Inversion On */
    ST7789_WriteCommand(0x21);
    /* Sleep Out */
    ST7789_WriteCommand(0x11);
    /* wait for power stability */
    rt_thread_mdelay(100);

    ST7789_Fill_Color(WHITE);

    /* display on */
    rt_pin_write(LCD_BLK_PIN, PIN_HIGH);
    ST7789_WriteCommand(0x29);

#endif

    return RT_EOK;
}

INIT_DEVICE_EXPORT(rt_hw_lcd_init);

/**
 * @brief Set the rotation direction of the display
 * @param m -> rotation parameter(please refer it in st7789.h)
 * @return none
 */
void ST7789_SetRotation(uint8_t m)
{
    ST7789_WriteCommand(ST7789_MADCTL); // MADCTL
    switch (m) {
    case 0:
        ST7789_WriteSmallData(ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB);
        break;
    case 1:
        ST7789_WriteSmallData(ST7789_MADCTL_MY | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
        break;
    case 2:
        ST7789_WriteSmallData(ST7789_MADCTL_RGB);
        break;
    case 3:
        ST7789_WriteSmallData(ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
        break;
    default:
        break;
    }
}

/**
 * @brief Fill the DisplayWindow with single color
 * @param color -> color to Fill with
 * @return none
 */
void ST7789_Fill_Color(uint16_t color)
{
    uint16_t i, j;
    ST7789_Select();
    ST7789_SetAddressWindow(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1);
    for (i = 0; i < ST7789_WIDTH; i++)
        for (j = 0; j < ST7789_HEIGHT; j++) {
            uint8_t data[] = {color >> 8, color & 0xFF};
            ST7789_WriteData(data, sizeof(data));
        }
    ST7789_UnSelect();
}

/**
 * @brief Draw a Pixel
 * @param x&y -> coordinate to Draw
 * @param color -> color of the Pixel
 * @return none
 */
void ST7789_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if ((x < 0) || (x >= ST7789_WIDTH) ||
         (y < 0) || (y >= ST7789_HEIGHT))   return;

    ST7789_Select();
    ST7789_SetAddressWindow(x, y, x, y);
    uint8_t data[] = {color >> 8, color & 0xFF};
    ST7789_WriteData(data, sizeof(data));
    ST7789_UnSelect();
}

/**
 * @brief Fill an Area with single color
 * @param xSta&ySta -> coordinate of the start point
 * @param xEnd&yEnd -> coordinate of the end point
 * @param color -> color to Fill with
 * @return none
 */
void ST7789_Fill(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd, uint16_t color)
{
    if ((xEnd < 0) || (xEnd >= ST7789_WIDTH) ||
         (yEnd < 0) || (yEnd >= ST7789_HEIGHT)) return;

    ST7789_Select();
    uint16_t i, j;
    ST7789_SetAddressWindow(xSta, ySta, xEnd, yEnd);
    for (i = ySta; i <= yEnd; i++)
        for (j = xSta; j <= xEnd; j++) {
            uint8_t data[] = {color >> 8, color & 0xFF};
            ST7789_WriteData(data, sizeof(data));
        }
    ST7789_UnSelect();
}

/**
 * @brief Draw a big Pixel at a point
 * @param x&y -> coordinate of the point
 * @param color -> color of the Pixel
 * @return none
 */
void ST7789_DrawPixel_4px(uint16_t x, uint16_t y, uint16_t color)
{
    if ((x <= 0) || (x > ST7789_WIDTH) ||
         (y <= 0) || (y > ST7789_HEIGHT))   return;

    ST7789_Select();
    ST7789_Fill(x - 1, y - 1, x + 1, y + 1, color);
    ST7789_UnSelect();
}

/**
 * @brief Draw a line with single color
 * @param x1&y1 -> coordinate of the start point
 * @param x2&y2 -> coordinate of the end point
 * @param color -> color of the line to Draw
 * @return none
 */
void ST7789_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
        uint16_t color) {
    uint16_t swap;
    uint16_t steep = ABS(y1 - y0) > ABS(x1 - x0);
    if (steep) {
        swap = x0;
        x0 = y0;
        y0 = swap;

        swap = x1;
        x1 = y1;
        y1 = swap;
        //_swap_int16_t(x0, y0);
        //_swap_int16_t(x1, y1);
    }

    if (x0 > x1) {
        swap = x0;
        x0 = x1;
        x1 = swap;

        swap = y0;
        y0 = y1;
        y1 = swap;
        //_swap_int16_t(x0, x1);
        //_swap_int16_t(y0, y1);
    }

    int16_t dx, dy;
    dx = x1 - x0;
    dy = ABS(y1 - y0);

    int16_t err = dx / 2;
    int16_t ystep;

    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;
    }

    for (; x0<=x1; x0++) {
        if (steep) {
            ST7789_DrawPixel(y0, x0, color);
        } else {
            ST7789_DrawPixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

/**
 * @brief Draw a Rectangle with single color
 * @param xi&yi -> 2 coordinates of 2 top points.
 * @param color -> color of the Rectangle line
 * @return none
 */
void ST7789_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    ST7789_Select();
    ST7789_DrawLine(x1, y1, x2, y1, color);
    ST7789_DrawLine(x1, y1, x1, y2, color);
    ST7789_DrawLine(x1, y2, x2, y2, color);
    ST7789_DrawLine(x2, y1, x2, y2, color);
    ST7789_UnSelect();
}

/**
 * @brief Draw a circle with single color
 * @param x0&y0 -> coordinate of circle center
 * @param r -> radius of circle
 * @param color -> color of circle line
 * @return  none
 */
void ST7789_DrawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    ST7789_Select();
    ST7789_DrawPixel(x0, y0 + r, color);
    ST7789_DrawPixel(x0, y0 - r, color);
    ST7789_DrawPixel(x0 + r, y0, color);
    ST7789_DrawPixel(x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        ST7789_DrawPixel(x0 + x, y0 + y, color);
        ST7789_DrawPixel(x0 - x, y0 + y, color);
        ST7789_DrawPixel(x0 + x, y0 - y, color);
        ST7789_DrawPixel(x0 - x, y0 - y, color);

        ST7789_DrawPixel(x0 + y, y0 + x, color);
        ST7789_DrawPixel(x0 - y, y0 + x, color);
        ST7789_DrawPixel(x0 + y, y0 - x, color);
        ST7789_DrawPixel(x0 - y, y0 - x, color);
    }
    ST7789_UnSelect();
}

/**
 * @brief Draw an Image on the screen
 * @param x&y -> start point of the Image
 * @param w&h -> width & height of the Image to Draw
 * @param data -> pointer of the Image array
 * @return none
 */
void ST7789_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data)
{
    if ((x >= ST7789_WIDTH) || (y >= ST7789_HEIGHT))
        return;
    if ((x + w - 1) >= ST7789_WIDTH)
        return;
    if ((y + h - 1) >= ST7789_HEIGHT)
        return;

    ST7789_Select();
    ST7789_SetAddressWindow(x, y, x + w - 1, y + h - 1);
    ST7789_WriteData((uint8_t *)data, sizeof(uint16_t) * w * h);
    ST7789_UnSelect();
}

/**
 * @brief Invert Fullscreen color
 * @param invert -> Whether to invert
 * @return none
 */
void ST7789_InvertColors(uint8_t invert)
{
    ST7789_Select();
    ST7789_WriteCommand(invert ? 0x21 /* INVON */ : 0x20 /* INVOFF */);
    ST7789_UnSelect();
}

/**
 * @brief Write a char
 * @param  x&y -> cursor of the start point.
 * @param ch -> char to write
 * @param font -> fontstyle of the string
 * @param color -> color of the char
 * @param bgcolor -> background color of the char
 * @return  none
 */
void ST7789_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor)
{
    uint32_t i, b, j;
    ST7789_Select();
    ST7789_SetAddressWindow(x, y, x + font.width - 1, y + font.height - 1);

    for (i = 0; i < font.height; i++) {
        b = font.data[(ch - 32) * font.height + i];
        for (j = 0; j < font.width; j++) {
            if ((b << j) & 0x8000) {
                uint8_t data[] = {color >> 8, color & 0xFF};
                ST7789_WriteData(data, sizeof(data));
            }
            else {
                uint8_t data[] = {bgcolor >> 8, bgcolor & 0xFF};
                ST7789_WriteData(data, sizeof(data));
            }
        }
    }
    ST7789_UnSelect();
}

/**
 * @brief Write a string
 * @param  x&y -> cursor of the start point.
 * @param str -> string to write
 * @param font -> fontstyle of the string
 * @param color -> color of the string
 * @param bgcolor -> background color of the string
 * @return  none
 */
void ST7789_WriteString(uint16_t x, uint16_t y, const char *str, FontDef font, uint16_t color, uint16_t bgcolor)
{
    ST7789_Select();
    while (*str) {
        if (x + font.width >= ST7789_WIDTH) {
            x = 0;
            y += font.height;
            if (y + font.height >= ST7789_HEIGHT) {
                break;
            }

            if (*str == ' ') {
                // skip spaces in the beginning of the new line
                str++;
                continue;
            }
        }
        ST7789_WriteChar(x, y, *str, font, color, bgcolor);
        x += font.width;
        str++;
    }
    ST7789_UnSelect();
}

/**
 * @brief Draw a filled Rectangle with single color
 * @param  x&y -> coordinates of the starting point
 * @param w&h -> width & height of the Rectangle
 * @param color -> color of the Rectangle
 * @return  none
 */
void ST7789_DrawFilledRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    ST7789_Select();
    uint8_t i;

    /* Check input parameters */
    if (x >= ST7789_WIDTH ||
        y >= ST7789_HEIGHT) {
        /* Return error */
        return;
    }

    /* Check width and height */
    if ((x + w) >= ST7789_WIDTH) {
        w = ST7789_WIDTH - x;
    }
    if ((y + h) >= ST7789_HEIGHT) {
        h = ST7789_HEIGHT - y;
    }

    /* Draw lines */
    for (i = 0; i <= h; i++) {
        /* Draw lines */
        ST7789_DrawLine(x, y + i, x + w, y + i, color);
    }
    ST7789_UnSelect();
}

/**
 * @brief Draw a Triangle with single color
 * @param  xi&yi -> 3 coordinates of 3 top points.
 * @param color ->color of the lines
 * @return  none
 */
void ST7789_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color)
{
    ST7789_Select();
    /* Draw lines */
    ST7789_DrawLine(x1, y1, x2, y2, color);
    ST7789_DrawLine(x2, y2, x3, y3, color);
    ST7789_DrawLine(x3, y3, x1, y1, color);
    ST7789_UnSelect();
}

/**
 * @brief Draw a filled Triangle with single color
 * @param  xi&yi -> 3 coordinates of 3 top points.
 * @param color ->color of the triangle
 * @return  none
 */
void ST7789_DrawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint16_t color)
{
    ST7789_Select();
    int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0,
            yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0,
            curpixel = 0;

    deltax = ABS(x2 - x1);
    deltay = ABS(y2 - y1);
    x = x1;
    y = y1;

    if (x2 >= x1) {
        xinc1 = 1;
        xinc2 = 1;
    }
    else {
        xinc1 = -1;
        xinc2 = -1;
    }

    if (y2 >= y1) {
        yinc1 = 1;
        yinc2 = 1;
    }
    else {
        yinc1 = -1;
        yinc2 = -1;
    }

    if (deltax >= deltay) {
        xinc1 = 0;
        yinc2 = 0;
        den = deltax;
        num = deltax / 2;
        numadd = deltay;
        numpixels = deltax;
    }
    else {
        xinc2 = 0;
        yinc1 = 0;
        den = deltay;
        num = deltay / 2;
        numadd = deltax;
        numpixels = deltay;
    }

    for (curpixel = 0; curpixel <= numpixels; curpixel++) {
        ST7789_DrawLine(x, y, x3, y3, color);

        num += numadd;
        if (num >= den) {
            num -= den;
            x += xinc1;
            y += yinc1;
        }
        x += xinc2;
        y += yinc2;
    }
    ST7789_UnSelect();
}

/**
 * @brief Draw a Filled circle with single color
 * @param x0&y0 -> coordinate of circle center
 * @param r -> radius of circle
 * @param color -> color of circle
 * @return  none
 */
void ST7789_DrawFilledCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    ST7789_Select();
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    ST7789_DrawPixel(x0, y0 + r, color);
    ST7789_DrawPixel(x0, y0 - r, color);
    ST7789_DrawPixel(x0 + r, y0, color);
    ST7789_DrawPixel(x0 - r, y0, color);
    ST7789_DrawLine(x0 - r, y0, x0 + r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        ST7789_DrawLine(x0 - x, y0 + y, x0 + x, y0 + y, color);
        ST7789_DrawLine(x0 + x, y0 - y, x0 - x, y0 - y, color);

        ST7789_DrawLine(x0 + y, y0 + x, x0 - y, y0 + x, color);
        ST7789_DrawLine(x0 + y, y0 - x, x0 - y, y0 - x, color);
    }
    ST7789_UnSelect();
}


/**
 * @brief Open/Close tearing effect line
 * @param tear -> Whether to tear
 * @return none
 */
void ST7789_TearEffect(uint8_t tear)
{
    ST7789_Select();
    ST7789_WriteCommand(tear ? 0x35 /* TEON */ : 0x34 /* TEOFF */);
    ST7789_UnSelect();
}

//显示汉字
//index ->汉字的序号，需要在字库查找
//size ->字号, 16/32
void ST7789_ShowChinese(uint16_t x, uint16_t y, uint8_t index, uint8_t size, uint16_t color, uint16_t bgcolor)
{
    uint8_t i,j;
    uint8_t *temp,size1;
    if (size == 16)
        temp = ST7789_CHN_16x16;//选择字号
    else if (size == 24)
        temp = ST7789_CHN_24x24;
    else if (size == 32)
        temp = ST7789_CHN_32x32;
    else
        temp = ST7789_CHN_16x16;//默认选择字号16

    ST7789_Select();
    ST7789_SetAddressWindow(x,y,x+size-1,y+size-1);     //设置一个汉字的区域
    size1=size*size/8;                              //一个汉字所占的字节
    temp+=index*size1;                              //写入的起始位置
    for(j=0;j<size1;j++)
    {
        for(i=0;i<8;i++)
        {
            if((*temp&(1<<i))!=0)   //从数据的低位开始读
            {
                uint8_t data[] = {color >> 8, color & 0xFF};
                ST7789_WriteData(data, sizeof(data));   //点亮
            }
            else
            {
                uint8_t data[] = { bgcolor >> 8, bgcolor & 0xFF};
                ST7789_WriteData(data, sizeof(data));   //填充背景色
            }
        }
        temp++;
     }
    ST7789_UnSelect();
}

/*
其中定义的汉字结构体元素中index[2]存放汉字，而data用于存放点阵码。
具体程序如下：
*/
#ifdef  USE_GB_16X16
void ST7789_ShowGBK16(uint16_t x, uint16_t y, uint8_t *pstr, uint16_t color, uint16_t bgcolor)
{
    uint8_t bit_cnt, byte_cnt, wordNum;
    uint16_t word_index;
    const GB16_Typedef* p_tab;

    p_tab = GB16x16;
    wordNum = sizeof(GB16x16) / sizeof(GB16_Typedef);//汉字的个数

//    read_data = read_data;

    ST7789_Select();

    while(*pstr != '\0')
    {
        for(word_index=0; word_index < wordNum; word_index++)
        {
            if((*pstr==p_tab[word_index].index[0]) && (*(pstr+1)==p_tab[word_index].index[1]))
            {
                ST7789_SetAddressWindow(x, y, x+GB16_FONT_WIDTH-1, y+GB16_FONT_HEIGHT-1);  //设置要操作的窗口范围

                for(byte_cnt=0; byte_cnt<GB16_FONT_BYTES; byte_cnt++)
                {
                    uint8_t read_data = p_tab[word_index].data[byte_cnt];
                    for (bit_cnt=0; bit_cnt<8; bit_cnt++)
                    {
                        if((read_data&0x80) == 0x80)
                        {
                            uint8_t data[] = {color >> 8, color & 0xFF};
                            ST7789_WriteData(data, sizeof(data));   //点亮
                        }
                        else
                        {
                            uint8_t data[] = { bgcolor >> 8, bgcolor & 0xFF};
                            ST7789_WriteData(data, sizeof(data));   //填充背景色
                        }
                        read_data <<= 1;
                    }
                }
                pstr+=2;
                x += GB16_FONT_WIDTH;
                if(x > ST7789_WIDTH)
                {
                    x = 0;
                    y += GB16_FONT_HEIGHT;
                }
            }
        }
    }

    ST7789_UnSelect();
}
#endif

/*
其中定义的汉字结构体元素中index[2]存放汉字，而data用于存放点阵码。
具体程序如下：
*/
#ifdef  USE_GB_24X24
void ST7789_ShowGBK24(uint16_t x, uint16_t y, uint8_t *pstr, uint16_t color, uint16_t bgcolor)
{
    uint8_t bit_cnt, byte_cnt, wordNum;
    uint16_t word_index;
    const GB24_Typedef* p_tab;

    p_tab = GB24x24;
    wordNum = sizeof(GB24x24) / sizeof(GB24_Typedef);//汉字的个数

//    read_data = read_data;

    ST7789_Select();

    while(*pstr != '\0')
    {
        for(word_index=0; word_index<wordNum; word_index++)
        {
            if(*pstr==p_tab[word_index].index[0] && *(pstr+1)==p_tab[word_index].index[1])
            {
                ST7789_SetAddressWindow(x, y, x+GB24_FONT_WIDTH-1, y+GB24_FONT_HEIGHT-1);  //设置要操作的窗口范围

                for(byte_cnt=0; byte_cnt<GB24_FONT_BYTES; byte_cnt++)
                {
                    uint8_t read_data = p_tab[word_index].data[byte_cnt];
                    for (bit_cnt=0; bit_cnt<8; bit_cnt++)
                    {
                        if((read_data&0x80) == 0x80)
                        {
                            uint8_t data[] = {color >> 8, color & 0xFF};
                            ST7789_WriteData(data, sizeof(data));   //点亮
                        }
                        else
                        {
                            uint8_t data[] = { bgcolor >> 8, bgcolor & 0xFF};
                            ST7789_WriteData(data, sizeof(data));   //填充背景色
                        }
                        read_data <<= 1;
                    }
                }
                pstr+=2;
                x += GB24_FONT_WIDTH;
                if(x > ST7789_WIDTH)
                {
                    x = 0;
                    y += GB24_FONT_HEIGHT;
                }
            }
        }
    }

    ST7789_UnSelect();
}
#endif

/*
其中定义的汉字结构体元素中index[2]存放汉字，而data用于存放点阵码。
具体程序如下：
*/
#ifdef  USE_GB_32X32
void ST7789_ShowGBK32(uint16_t x, uint16_t y, uint8_t *pstr, uint16_t color, uint16_t bgcolor)
{
    uint8_t bit_cnt, byte_cnt, wordNum;
    uint16_t word_index;
    const GB32_Typedef* p_tab;

    p_tab = GB32x32;
    wordNum = sizeof(GB32x32) / sizeof(GB32_Typedef);//汉字的个数

    ST7789_Select();

    while(*pstr != '\0')
    {
        for(word_index=0; word_index<wordNum; word_index++)
        {
            if(*pstr==p_tab[word_index].index[0] && *(pstr+1)==p_tab[word_index].index[1])
            {
                ST7789_SetAddressWindow(x, y, x+GB32_FONT_WIDTH-1, y+GB32_FONT_HEIGHT-1);  //设置要操作的窗口范围

                for(byte_cnt=0; byte_cnt<GB32_FONT_BYTES; byte_cnt++)
                {
                    uint8_t read_data = p_tab[word_index].data[byte_cnt];
                    for (bit_cnt=0; bit_cnt<8; bit_cnt++)
                    {
                        if((read_data&0x80) == 0x80)
                        {
                            uint8_t data[] = {color >> 8, color & 0xFF};
                            ST7789_WriteData(data, sizeof(data));   //点亮
                        }
                        else
                        {
                            uint8_t data[] = { bgcolor >> 8, bgcolor & 0xFF};
                            ST7789_WriteData(data, sizeof(data));   //填充背景色
                        }
                        read_data <<= 1;
                    }
                }
                pstr+=2;
                x += GB32_FONT_WIDTH;
                if(x > ST7789_WIDTH)
                {
                    x = 0;
                    y += GB32_FONT_HEIGHT;
                }
            }
        }
    }

    ST7789_UnSelect();
}
#endif

void ST7789_ShowGBK(uint16_t x, uint16_t y, uint8_t font_size, uint8_t *pstr, uint16_t color, uint16_t bgcolor)
{
    switch(font_size)
    {
        case 16:
            #ifdef  USE_GB_16X16
            ST7789_ShowGBK16(x, y, pstr, color, bgcolor);
            #endif
            break;
        case 24:
            #ifdef  USE_GB_24X24
            ST7789_ShowGBK24(x, y, pstr, color, bgcolor);
            #endif
            break;
        case 32:
            #ifdef  USE_GB_32X32
            ST7789_ShowGBK32(x, y, pstr, color, bgcolor);
            #endif
            break;
        default:
            break;
    }
}



/**
 * @brief A Simple test function for ST7789
 * @param  none
 * @return  none
 */
void ST7789_Test(void)
{
    ST7789_Fill_Color(WHITE);
    HAL_Delay(1000);
    ST7789_WriteString(10, 20, "Speed Test", Font_11x18, RED, WHITE);
    HAL_Delay(1000);
    ST7789_Fill_Color(CYAN);
    ST7789_Fill_Color(RED);
    ST7789_Fill_Color(BLUE);
    ST7789_Fill_Color(GREEN);
    ST7789_Fill_Color(YELLOW);
    ST7789_Fill_Color(BROWN);
    ST7789_Fill_Color(DARKBLUE);
    ST7789_Fill_Color(MAGENTA);
    ST7789_Fill_Color(LIGHTGREEN);
    ST7789_Fill_Color(LGRAY);
    ST7789_Fill_Color(LBBLUE);
    ST7789_Fill_Color(WHITE);
    HAL_Delay(500);

    ST7789_WriteString(10, 10, "Font test.", Font_16x26, GBLUE, WHITE);
    ST7789_WriteString(10, 50, "Hello Caesar!", Font_7x10, RED, WHITE);
    ST7789_WriteString(10, 75, "Hello Caesar!", Font_11x18, YELLOW, WHITE);
    ST7789_WriteString(10, 120, "Hello Caesar!", Font_16x26, MAGENTA, WHITE);
    HAL_Delay(1000);

    ST7789_Fill_Color(RED);
    ST7789_WriteString(10, 10, "Rect./Line.", Font_11x18, YELLOW, RED);
    ST7789_DrawRectangle(30, 30, 100, 100, WHITE);
    HAL_Delay(1000);

    ST7789_Fill_Color(RED);
    ST7789_WriteString(10, 10, "Filled Rect.", Font_11x18, YELLOW, RED);
    ST7789_DrawFilledRectangle(30, 30, 50, 50, WHITE);
    HAL_Delay(1000);


    ST7789_Fill_Color(RED);
    ST7789_WriteString(10, 10, "Circle.", Font_11x18, YELLOW, RED);
    ST7789_DrawCircle(60, 60, 25, WHITE);
    HAL_Delay(1000);

    ST7789_Fill_Color(RED);
    ST7789_WriteString(10, 10, "Filled Cir.", Font_11x18, YELLOW, RED);
    ST7789_DrawFilledCircle(60, 60, 25, WHITE);
    HAL_Delay(1000);

    ST7789_Fill_Color(RED);
    ST7789_WriteString(10, 10, "Triangle", Font_11x18, YELLOW, RED);
    ST7789_DrawTriangle(30, 30, 30, 70, 60, 40, WHITE);
    HAL_Delay(1000);

    ST7789_Fill_Color(RED);
    ST7789_WriteString(10, 10, "Filled Tri", Font_11x18, YELLOW, RED);
    ST7789_DrawFilledTriangle(30, 30, 30, 70, 60, 40, WHITE);
    HAL_Delay(1000);

    ST7789_Fill_Color(BLACK);
    ST7789_WriteString(100, 50, "Hello World.", Font_11x18, GBLUE, BLACK);
    ST7789_ShowGBK(20, 50, 16, (uint8_t *)"中国智造", RED, BLACK);
    ST7789_ShowGBK(20, 100, 24, (uint8_t *)"让世界看见中国力量", BLUE, BLACK);
    ST7789_ShowGBK(20, 150, 32, (uint8_t *)"卫星平地系统", YELLOW, BLACK);
    HAL_Delay(1000);

    //  If FLASH cannot storage anymore datas, please delete codes below.
    ST7789_Fill_Color(WHITE);
    ST7789_DrawImage(0, 0, 128, 128, (uint16_t *)saber);
    HAL_Delay(3000);
}

