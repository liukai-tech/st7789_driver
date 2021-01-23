#include "ugui.h"
#include "st7789.h"

/* GUI structure */
UG_GUI gui;

uint8_t ugui_is_initialize = 0;
uint8_t benchmark_is_ready = 1;

#define MAX_OBJECTS        10

/* Window 1 */
UG_WINDOW window_1;
UG_OBJECT obj_buff_wnd_1[MAX_OBJECTS];
UG_BUTTON button1_1;
UG_BUTTON button1_2;
UG_BUTTON button1_3;
UG_BUTTON button1_4;
UG_BUTTON button1_5;
UG_BUTTON button1_6;

/* Window 2 */
UG_WINDOW window_2;
UG_OBJECT obj_buff_wnd_2[MAX_OBJECTS];
UG_BUTTON button2_1;
UG_TEXTBOX textbox2_1;
UG_TEXTBOX textbox2_2;
UG_IMAGE image2_1;

/* Window 3 */
UG_WINDOW window_3;
UG_OBJECT obj_buff_wnd_3[MAX_OBJECTS];
UG_BUTTON button3_1;
UG_TEXTBOX textbox3_1;

/* FSM */
#define STATE_MAIN_MENU                0
#define STATE_BENCHMARK_RUN            1
#define STATE_BENCHMARK_RESULT         2
volatile UG_U32 state;

/* Benchmark */
volatile UG_U32 timer;
volatile UG_U32 hw_acc = 1;
char result_str[30];
UG_S16 xs,ys;
UG_S16 xe,ye;
UG_COLOR c;

void pset(UG_S16 x, UG_S16 y, UG_COLOR col)
{
	ST7789_DrawPixel(x, y, col);
}

/* Hardware accelerator for UG_DrawLine (Platform: STM32F4x9) */
UG_RESULT _HW_DrawLine( UG_S16 x1, UG_S16 y1, UG_S16 x2, UG_S16 y2, UG_COLOR c )
{
   ST7789_DrawLine(x1, y1, x2, y2, c);	
   return UG_RESULT_OK;
}

/* Hardware accelerator for UG_FillFrame (Platform: STM32F4x9) */
UG_RESULT _HW_FillFrame( UG_S16 x1, UG_S16 y1, UG_S16 x2, UG_S16 y2, UG_COLOR c )
{
	ST7789_Fill(x1, y1, x2, y2, c);
	return UG_RESULT_OK;
}

void uGUI_Test_Init(void)
{
	 /* Init ¦ÌGUI */
	UG_Init(&gui,(void(*)(UG_S16,UG_S16,UG_COLOR))pset,ST7789_WIDTH,ST7789_HEIGHT);

	/* Register hardware acceleration */
	UG_DriverRegister( DRIVER_DRAW_LINE, (void*)_HW_DrawLine );
	UG_DriverRegister( DRIVER_FILL_FRAME, (void*)_HW_FillFrame );
	UG_DriverEnable( DRIVER_DRAW_LINE );
	UG_DriverEnable( DRIVER_FILL_FRAME );
	
	ugui_is_initialize = 1;
	
	UG_FillScreen( C_BLACK );
	
	state = STATE_MAIN_MENU;
}

/* better rand() function */
UG_U32 randx( void )
{
   static UG_U32 z1 = 12345, z2 = 12345, z3 = 12345, z4 = 12345;
   UG_U32 b;
   b  = ((z1 << 6) ^ z1) >> 13;
   z1 = ((z1 & 4294967294U) << 18) ^ b;
   b  = ((z2 << 2) ^ z2) >> 27;
   z2 = ((z2 & 4294967288U) << 2) ^ b;
   b  = ((z3 << 13) ^ z3) >> 21;
   z3 = ((z3 & 4294967280U) << 7) ^ b;
   b  = ((z4 << 3) ^ z4) >> 12;
   z4 = ((z4 & 4294967168U) << 13) ^ b;
   return (z1 ^ z2 ^ z3 ^ z4);
}

void uGUI_Test_Poll(void)
{
	static int frm_cnt;
	benchmark_is_ready = 1;
	
	while(benchmark_is_ready)
	{
		/* FSM */
      switch ( state )
      {
         /* Run the benchmark */
         case STATE_BENCHMARK_RUN:
         {
            xs = randx() % ST7789_WIDTH;
            xe = randx() % ST7789_WIDTH;
            ys = randx() % ST7789_HEIGHT;
            ye = randx() % ST7789_HEIGHT;
            c = randx() % 0xFFFFFF;
            UG_FillFrame( xs, ys, xe, ye, c );
            frm_cnt++;

            if ( !timer ) state = STATE_BENCHMARK_RESULT;
            break;
         }
         /* Show benchmark result */
         case STATE_BENCHMARK_RESULT:
         {
            sprintf( result_str, "Result:%u frm/sec", frm_cnt/10 );
			UG_FillScreen(C_WHITE);
			 UG_FontSelect(&FONT_12X20);
			 UG_SetBackcolor(C_WHITE);
			 UG_SetForecolor(C_RED);
			UG_PutString(0, 5, result_str);
//			 ST7789_WriteString(0, 30, result_str, Font_11x18, C_RED, C_WHITE);
			HAL_Delay(1000);

            state = STATE_MAIN_MENU;
			 
			benchmark_is_ready = 0; 
            break;
         }
         case STATE_MAIN_MENU:
         {
			UG_FillScreen( C_BLACK );
			UG_FontSelect(&FONT_12X20);
			UG_SetBackcolor(C_BLACK);
			UG_SetForecolor(C_GREEN);
			UG_PutString(0, 5, "Benchmark Test.");
			HAL_Delay(1000);
			UG_FillScreen(C_WHITE);

            /* Let ¦ÌGUI do the job! */
			/* Reset the frame counter */
			frm_cnt = 0;

			/* Run benchmark for 10 seconds */
			timer = 10000; 
			 
			state =  STATE_BENCHMARK_RUN;

            break;
         }
      }
	}
	
	
}
