/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "st7789.h"
#include "testimg.h"
#include "ugui.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
extern void uGUI_Test_Init(void);
extern void uGUI_Test_Poll(void);
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void loop() {
    // Check border
    ST7789_Fill_Color(BLACK);

    for(int x = 0; x < ST7789_WIDTH; x++) {
        ST7789_DrawPixel(x, 0, RED);
        ST7789_DrawPixel(x, ST7789_HEIGHT-1, RED);
    }

    for(int y = 0; y < ST7789_HEIGHT; y++) {
        ST7789_DrawPixel(0, y, RED);
        ST7789_DrawPixel(ST7789_WIDTH-1, y, RED);
    }

    HAL_Delay(3000);

    // Check fonts
    ST7789_Fill_Color(BLACK);
    ST7789_WriteString(0, 0, "Font_7x10, red on black, lorem ipsum dolor sit amet", Font_7x10, RED, BLACK);
    ST7789_WriteString(0, 3*10, "Font_11x18, green, lorem ipsum", Font_11x18, GREEN, BLACK);
    ST7789_WriteString(0, 3*10+3*18, "Font_16x26, blue, hello world.", Font_16x26, BLUE, BLACK);
    HAL_Delay(2000);

    // Check colors
    ST7789_Fill_Color(BLACK);
    ST7789_WriteString(0, 0, "BLACK", Font_11x18, WHITE, BLACK);
    HAL_Delay(500);

    ST7789_Fill_Color(BLUE);
    ST7789_WriteString(0, 0, "BLUE", Font_11x18, BLACK, BLUE);
    HAL_Delay(500);

    ST7789_Fill_Color(RED);
    ST7789_WriteString(0, 0, "RED", Font_11x18, BLACK, RED);
    HAL_Delay(500);

    ST7789_Fill_Color(GREEN);
    ST7789_WriteString(0, 0, "GREEN", Font_11x18, BLACK, GREEN);
    HAL_Delay(500);

    ST7789_Fill_Color(CYAN);
    ST7789_WriteString(0, 0, "CYAN", Font_11x18, BLACK, CYAN);
    HAL_Delay(500);

    ST7789_Fill_Color(MAGENTA);
    ST7789_WriteString(0, 0, "MAGENTA", Font_11x18, BLACK, MAGENTA);
    HAL_Delay(500);

    ST7789_Fill_Color(YELLOW);
    ST7789_WriteString(0, 0, "YELLOW", Font_11x18, BLACK, YELLOW);
    HAL_Delay(500);

    ST7789_Fill_Color(WHITE);
    ST7789_WriteString(0, 0, "WHITE", Font_11x18, BLACK, WHITE);
    HAL_Delay(500);

    // Display test image 128x128
    ST7789_DrawImage(0, 0, 128, 128, (uint16_t*)parrot_128x128);
	
	HAL_Delay(2000);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI2_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
	ST7789_Init();
	uGUI_Test_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  uGUI_Test_Poll();
	  ST7789_Test();
	  loop();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
