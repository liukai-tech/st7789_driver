/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-01-21     RT-Thread    first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "drv_lcd.h"

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/* PLEASE DEFINE the LED0 pin for your board, such as: PA5 */
#define LED0_PIN    GET_PIN(A, 5)

int main(void)
{
    int count = 1;

    /* set LED0 pin mode to output */
    rt_pin_mode(LED0_PIN, PIN_MODE_OUTPUT);

    /* clear lcd */
    ST7789_Fill_Color(WHITE);

    /* show some string on lcd */
    ST7789_WriteString(10, 20,  "STM32F411RET6&ST7789",  Font_11x18, RED, WHITE);
    ST7789_WriteString(10, 69,  "Hello, RT-Thread!",  Font_7x10, BLACK, WHITE);
    ST7789_WriteString(10, 69+16, "RT-Thread", Font_11x18, BLACK, WHITE);
    ST7789_WriteString(10, 69+16+24, "RT-Thread", Font_16x26, BLACK, WHITE);

    /* draw a line on lcd */
    ST7789_DrawLine(0, 69+16+24+32, 240, 69+16+24+32+4, GBLUE);

    /* draw a concentric circles */
    ST7789_DrawPixel(120, 194, RED);
    for (int i = 0; i < 46; i += 4)
    {
        ST7789_DrawCircle(120, 194, i, GREEN);
    }

    while (count++)
    {
        ST7789_Test();

        /* set LED0 pin level to high or low */
        rt_pin_write(LED0_PIN, count % 2);
//        LOG_D("Hello RT-Thread!");
        rt_thread_mdelay(1000);
    }

    return RT_EOK;
}
