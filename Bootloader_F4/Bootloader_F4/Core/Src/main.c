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
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "mem_layout.h"
#include "tiny_printf.h"
#include "tiny_scanf.h"
#include "jump_to_app.h"
#include "BL_Functions.h"
#include "Cryptology_Control.h"

#include "bootloader_interface.h"
#include "firmware_footer.h" // For struct definition
#include "keys.h"

//Left them for test Functions
#include "sha256.h"
#include "aes.h"
#include "cbc_mode.h"
#include "ecc_dsa.h"
#include "ecc.h"

#include <stdint.h>
#include <string.h>
#define BOOT_MAGIC  0x424F4F54UL   // 'BOOT'
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
BootConfig_t config;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
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
    printf("Starting Bootloader Version-(%d,%d)\r\n", 1, 7); // VERIFY THIS PRINTS 1,7
    printf("========================================\r\n");

    // 1. Read Config
    if (BL_ReadConfig(&config)) {
        printf("[BL] Config Invalid/Empty. Initialized to Defaults.\r\n");
        BL_WriteConfig(&config);
    }

    // 2. CHECK via terminal
    term_cmd_t cmd = BL_ReadTerminalCommand(HAL_MAX_DELAY); // infinitelly read the terminal unt,l resp

    if (cmd == TERM_INFO) {
        BL_PrintInfo(&config);
        cmd = BL_ReadTerminalCommand(HAL_MAX_DELAY); // allow another choice after info
    }

    else if (cmd == TERM_UPDATE) {
        // Optional: only allow if S6 valid
        FW_Status_t status = Firmware_Is_Valid(APP_DOWNLOAD_START_ADDR, SLOT_SIZE);
        if (status == BL_OK) {
            printf("[BL] Terminal requested UPDATE.\r\n");
            config.system_status = STATE_UPDATE_REQ;
            BL_WriteConfig(&config);
        } else {
            printf("[BL] No valid image in S6 (err=%d). Staying NORMAL.\r\n", status);
            config.system_status = STATE_NORMAL;
        }
    }
    else if (cmd == TERM_ROLLBACK) {
        printf("[BL] Terminal requested ROLLBACK.\r\n");
        config.system_status = STATE_ROLLBACK;
        BL_WriteConfig(&config);
    }
    else if (cmd == TERM_BOOT) {
        printf("[BL] Terminal requested BOOT.\r\n");
        config.system_status = STATE_NORMAL; // then fall into jump logic
    }
    else {
        // TERM_AUTO: keep whatever state is stored in config, your original design
        printf("[BL] AUTO mode.\r\n");
        // No New Update found.
		// We assume the user wants to SWAP (Rollback) to the other app.
		// Optional: Check if S6 is empty (0xFFFFFFFF) to prevent swapping with nothing.
		uint32_t *s6_ptr = (uint32_t*)APP_DOWNLOAD_START_ADDR;
		if (*s6_ptr == 0xFFFFFFFF) {
		   printf(" -> Download Slot is Empty. Cannot Swap.\r\n");
		   config.system_status = STATE_NORMAL;
		}
		else {
		   printf(" -> Download Slot has data (Backup). Requesting SWAP/ROLLBACK.\r\n");
		   config.system_status = STATE_ROLLBACK; // <--- This performs the swap
			}
    }
 // Bootloader_Run();//Bootloader_JumpToApp(); // for f4 app}

  switch(config.system_status){
      case STATE_UPDATE_REQ:
          printf("[BL] State: UPDATE REQUESTED.\r\n");
          BL_Swap_NoBuffer();

          printf("[BL] Update Failed. Reverting state.\r\n");
          config.system_status = STATE_NORMAL;
          BL_WriteConfig(&config);
          // Fall through to try booting S5

      case STATE_ROLLBACK:
          // Call new Rollback Function
          BL_Rollback();

          printf("[BL] Rollback Failed. Reverting state.\r\n");
          config.system_status = STATE_NORMAL;
          BL_WriteConfig(&config);
          HAL_NVIC_SystemReset();
          // Fall through

      case STATE_NORMAL:
      default:
          printf("[BL] State: NORMAL. Checking Active Application (S5)...\r\n");

          // Check if S5 contains valid code (Reset Vector check)
          uint32_t *app_reset_vector = (uint32_t*)(APP_ACTIVE_START_ADDR + 4);
          uint32_t app_entry_point = *app_reset_vector;
          HAL_Delay(500);
          printf("app_reset_vector: %x\r\n", app_reset_vector);
          HAL_Delay(500);

          if (app_entry_point > APP_ACTIVE_START_ADDR && app_entry_point < (APP_ACTIVE_START_ADDR + SLOT_SIZE))
          {
              printf("[BL] Valid App found at 0x%X. Jumping...\r\n", (unsigned int)APP_ACTIVE_START_ADDR);
              Bootloader_JumpToApp();
          }
          else
          {
              printf("[BL] S5 Empty or Invalid! Checking S6 for Auto-Provisioning...\r\n");

              // Auto-Provision: If S5 empty, check if we uploaded file to S6
              if (Firmware_Is_Valid(APP_DOWNLOAD_START_ADDR, SLOT_SIZE) == 1)
              {
                  printf("[BL] Valid Image found in S6! Triggering Update...\r\n");
                  config.system_status = STATE_UPDATE_REQ;
                  BL_WriteConfig(&config);
                  HAL_NVIC_SystemReset();
              }
              else
              {
                  printf("[ERROR] No valid app in S5, and no update in S6.\r\n");
                  printf("[ERROR] System Halted.\r\n");
              }
          }
          break;
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
  // }
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
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
