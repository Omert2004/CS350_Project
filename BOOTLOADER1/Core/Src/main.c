/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "mem_layout.h"
#include "tiny_printf.h"
#include "jump_to_app.h"
#include "BL_Functions.h"

#include "bootloader_interface.h" // For Bootloader_API_t

//#include <stdio.h>
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

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
BootConfig_t config;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

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
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  	tfp_init(&huart1);
	printf("\r\n========================================\r\n");
	printf("Starting Bootloader Version-(%d,%d)\r\n", 1, 6);
	printf("========================================\r\n");

	/*config.magic_number = 0xDEADBEEF;
		config.system_status = STATE_UPDATE_REQ;
		config.boot_failure_count = 0;

		// COMMIT: Save to Flash before we Reset
		BL_WriteConfig(&config);
*/
	// 1. Read Config (This loads defaults into RAM if Flash is empty)
	if (BL_ReadConfig(&config)) {
	      // Config was empty/invalid.
	      // We modified RAM defaults inside the function.
	      // COMMIT: Save defaults to Flash immediately.
	      BL_WriteConfig(&config);
	  }

	config.system_status = STATE_UPDATE_REQ;
	config.boot_failure_count = 0;

	// COMMIT: Save to Flash before we Reset
	BL_WriteConfig(&config);

	if (config.system_status == STATE_NORMAL) {
	    printf("[TEST] Current State is NORMAL. Forcing Update Request...\r\n");

	    config.system_status = STATE_UPDATE_REQ;
	    config.boot_failure_count = 0;

	    // Write the Request to Flash
	    BL_WriteConfig(&config);

	    // RESET immediately to process the request cleanly
	    //NVIC_SystemReset();
	}

	// 2. Logic Check
	if (config.system_status == STATE_UPDATE_REQ) {
		printf("[BL] Update Request Detected. Swapping...\r\n");
		// Perform Verification & Copy/Swap (Slot 2 -> Slot 1)
		BL_Swap_NoBuffer();

		// Mark as TESTING so next boot we watch for crashes
		config.system_status = STATE_TESTING;
		config.boot_failure_count = 0;
		BL_WriteConfig(&config);
		printf("[BL] Swap Done. Resetting to Test New App...\r\n");
		//NVIC_SystemReset(); // Reboot into new app
	}
	else if (config.system_status == STATE_TESTING) {
		printf("[BL] Verifying New Update (Attempt %lu)...\r\n", config.boot_failure_count);
		if (config.boot_failure_count >= 3) {
			printf("[BL] CRASH LIMIT REACHED! Rolling Back...\r\n");
			BL_Swap_NoBuffer();

			config.system_status = STATE_NORMAL;
			config.boot_failure_count = 0;
			BL_WriteConfig(&config);
			printf("New Application could not ne \r\n");
		} else {
			// Still testing. Increment crash counter.
			config.boot_failure_count++;
			BL_WriteConfig(&config);

			// Enable Watchdog here to catch the crash
			// MX_IWDG_Init();
		}
	}
	printf("[BL] Jumping to Application...\r\n");
	Bootloader_JumpToApp();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 10;
  RCC_OscInitStruct.PLL.PLLN = 210;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();

  /*Configure GPIO pin : USER_LED_Pin */
  GPIO_InitStruct.Pin = USER_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USER_LED_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/**
 * @brief   Tests MPU protection of the Bootloader region.
 *
 * @details Attempts to write to the Bootloader flash start address
 *          (0x08000000), which is configured as Read-Only.
 *          A correct MPU configuration must trigger a MemManage Fault.
 *
 * @warning This test is expected to crash the system.
 *
 * @retval  None
 */
void Test_MPU_Violation(void)
{
    printf("\r\n[TEST] 1. Testing Bootloader Protection (Expect Crash)...\r\n");
    HAL_Delay(100); // Wait for UART to finish sending

    // Point to the start of Bootloader (Region 0 - Read Only)
    volatile uint32_t *pFlash = (uint32_t *)0x08000000;

    // TRY TO WRITE (This should trigger MemManage Fault)
    *pFlash = 0xDEADBEEF;

    // If code reaches here, MPU FAILED!
    printf("[FAIL] MPU did NOT block the write!\r\n");
}

/**
 * @brief   Tests write access to the configuration flash sector.
 *
 * @details Erases and writes a test word to the configuration sector
 *          at 0x08010000 (Sector 2), which is configured as Read-Write.
 *          Successful execution confirms correct MPU permissions.
 *
 * @retval  None
 *
 Created on: 22 Ara 2025
 *      Author: Oguzm
 */
void Test_Config_Write(void)
{
    printf("\r\n[TEST] 2. Testing Config Sector Write (Expect Success)...\r\n");

    // 1. Unlock Flash
    HAL_FLASH_Unlock();

    // 2. Try to write to 0x08010000 (Region 1)
    // Note: We use HAL function. If MPU was RO, this would fail.
    // Since MPU is RW, this should work.
    uint32_t test_addr = 0x08010000;
    uint32_t test_data = 0x12345678;

    // Erase sector first (Standard Flash procedure)
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t SectorError;
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    EraseInitStruct.Sector = FLASH_SECTOR_2; // Sector 2 is 0x08010000
    EraseInitStruct.NbSectors = 1;

    if(HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) == HAL_OK)
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, test_addr, test_data) == HAL_OK)
        {
             printf("[PASS] Successfully wrote to Config Sector.\r\n");
        }
        else
        {
             printf("[FAIL] HAL Flash Program failed (Not MPU related).\r\n");
        }
    }
    else
    {
        printf("[FAIL] Flash Erase failed.\r\n");
    }

    HAL_FLASH_Lock();
}

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x08000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_64KB;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_PRIV_RO_URO;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.BaseAddress = 0x08010000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_32KB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1; // TEX=1, C=0, B=0 -> Normal, Non-Cacheable

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
