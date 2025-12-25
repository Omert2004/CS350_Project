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
#include "BL_Update_Part.h"      // For BL_RequestUpdate, BL_GetStatus, Bootloader_HandleUpdate
#include "BL_Functions.h"        // For Bootloader_InternalVerify
#include "bootloader_interface.h" // For Bootloader_API_t
#include "firmware_footer.h"     // For BL_Status_t, BL_OK

//Libs Include
#include "sha256.h"
#include "ecc_dsa.h"
#include "aes.h"
#include "lz4.h"
#include "cbc_mode.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define MAJOR 1			//Major Version Number
#define MINOR 2			//Minor Version Number

#define BKP_FLAG_UPDATE_REQ 0xCAFEBABE

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

RTC_HandleTypeDef hrtc;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_RTC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// [Fix: Add the Wrapper Function BEFORE the API Table]
// The API table expects a function pointer with signature: int (*VerifySlot)(uint32_t slot_id)
// But our internal function is: BL_Status_t Bootloader_InternalVerify(uint32_t slot_start, uint32_t slot_size)
// So we need this wrapper to bridge them.
int BL_VerifySlot_Wrapper(uint32_t slot_id) {
    // Note: We ignore slot_id for now and verify the active slot constants.
    // In a multi-slot system, you would switch address based on slot_id.
    if (Bootloader_InternalVerify(APP_ACTIVE_START_ADDR, APP_ACTIVE_SIZE) == BL_OK) {
        return 1; // Valid
    }
    return 0; // Invalid
}


/**
 * @brief   The Actual Shared API Table Instance.
 * @details Linker places this at 0x0800F000
 */
__attribute__((section(".shared_api_section")))
const Bootloader_API_t API_Table = {
    .magic_code = 0xDEADBEEF,
    .version = 0x0100,
    .RequestUpdate = BL_RequestUpdate,
    .GetBootStatus = BL_GetStatus,
    .VerifySlot = BL_VerifySlot_Wrapper
};


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
  MX_USART1_UART_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */

  tfp_init(&huart1);

  printf("\r\n========================================\r\n");
	printf("Starting Bootloader Version-(%d,%d)\r\n", MAJOR, 4);
	printf("API Table Location: %p\r\n", &API_Table); // Debug print
	printf("========================================\r\n");

	printf("Jumping");
	Bootloader_JumpToApp();

	// --- 1. Check for Update Request (Magic Flag) ---
	HAL_PWR_EnableBkUpAccess();
	if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) == BKP_FLAG_UPDATE_REQ) {

		printf("[BL] Update Request Detected. Clearing Flag...\r\n");

		/* Clear flag */
		HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, 0x00000000);
		HAL_PWREx_DisableBkUpReg();

		/* Perform Update (Decrypt -> Decompress -> Flash) */
		// This function will reboot on success. If it returns, update failed.
		Bootloader_HandleUpdate();
	}
	HAL_PWREx_DisableBkUpReg();

	// --- 2. Verify Application (Secure Boot) ---
	// Your friend's flow adds this check, which is CRITICAL for security.
	printf("[BL] Verifying Application integrity...\r\n");

	if (Bootloader_InternalVerify(APP_ACTIVE_START_ADDR, APP_ACTIVE_SIZE) == BL_OK) {

		printf("[BL] Verification Success! Jumping to App.\r\n");

		// Cleanup before jump
		HAL_MPU_Disable();

		Bootloader_JumpToApp();
	}
	else
	{
		// --- 3. Verification Failed (Rescue Mode) ---
		printf("[BL] CRITICAL: Verification Failed! Entering Rescue Mode.\r\n");
		printf("[BL] Status Code: 0x%08lX\r\n", BL_GetStatus());
		printf("[BL] HOLD USER BUTTON to force retry update from Download Slot.\r\n");
		// Infinite Error Loop

		while (1)
		{
			// 1. LED Blink (Hızlı Hata Modu - SOS)
			HAL_GPIO_TogglePin(GPIOI, USER_LED_Pin);
			HAL_Delay(100);

			// 2. Buton Kontrolü (User Button - Genelde PI11 veya PA0, şemaya bakın)
			// Eğer buton GPIO'su tanımlı değilse, sadece MX_GPIO_Init'e eklemeniz gerekir.
			// Örnek: Eğer Buton GPIOI Pin 11 ise:

			if (HAL_GPIO_ReadPin(GPIOI, GPIO_PIN_11) == GPIO_PIN_SET) {
				printf("[BL] User Button Detected! Forcing Update Retry...\r\n");

				// Magic Flag'i tekrar set et
				HAL_PWR_EnableBkUpAccess();
				HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, 0xCAFEBABE);
				HAL_PWREx_DisableBkUpReg();

				// Reset at (Bootloader tekrar açılacak ve güncellemeyi deneyecek)
				NVIC_SystemReset();
			}

		}
	}

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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

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

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : USER_LED_Pin */
  GPIO_InitStruct.Pin = USER_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USER_LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : B_USER_Pin */
  GPIO_InitStruct.Pin = B_USER_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B_USER_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */


void print_hardfault_reason(void) {
    volatile uint32_t cfsr  = SCB->CFSR;
    volatile uint32_t hfsr  = SCB->HFSR;
    volatile uint32_t mmfar = SCB->MMFAR;
    volatile uint32_t bfar  = SCB->BFAR;

    printf("\r\n--- Hard Fault Detected ---\r\n");
    printf("CFSR:  0x%08lX\r\n", cfsr);
    printf("HFSR:  0x%08lX\r\n", hfsr);
    printf("MMFAR: 0x%08lX\r\n", mmfar);
    printf("BFAR:  0x%08lX\r\n", bfar);

    if (cfsr & (1 << 24)) printf(" -> Unaligned Access UsageFault\r\n");
    if (cfsr & (1 << 25)) printf(" -> Divide by Zero UsageFault\r\n");
    if (cfsr & (1 << 18)) printf(" -> Integrity Check Error\r\n");
    if (cfsr & (1 << 17)) printf(" -> Invalid PC Load\r\n");
    if (cfsr & (1 << 16)) printf(" -> Invalid State\r\n");
    if (cfsr & (1 << 0))  printf(" -> Instruction Access Violation (MPU)\r\n");
    if (cfsr & (1 << 1))  printf(" -> Data Access Violation (MPU)\r\n");
    if (cfsr & (1 << 8))  printf(" -> Instruction Bus Error\r\n");
    if (cfsr & (1 << 9))  printf(" -> Precise Data Bus Error\r\n");
    if (cfsr & (1 << 15)) printf(" -> BFAR Address Valid: 0x%08lX\r\n", bfar);
}

/* USER CODE END 4 */

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
