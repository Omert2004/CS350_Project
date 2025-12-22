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
#include "stm32f7xx_hal_flash.h"
#include <string.h>
#include "mem_layout.h"
#include "bootloader_interface.h"
#include "firmware_footer.h"


#include "sha256.h"
#include "ecc_dsa.h"
#include "aes.h"
#include "lz4.h"
#include "cbc_mode.h"

#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ENC_CHUNK_SIZE  		1024  // Size of encrypted chunk on Flash
#define DEC_CHUNK_SIZE  		1024  // Size after decryption (same as enc)
#define RAW_CHUNK_SIZE  		4096  // Max size after decompression (safe margin)
#define BL_STATUS_UPDATED   	0x01
#define BL_STATUS_ERROR     	0x02
#define AXIM_Addr_Msk			0x2000
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define MAJOR 1			//Major Version Number
#define MINOR 2			//Minor Version Number

extern const uint8_t AES_SECRET_KEY[16];
extern const uint8_t* Get_Public_Key_X(void);
extern const uint8_t* Get_Public_Key_Y(void);
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

CRC_HandleTypeDef hcrc;

RTC_HandleTypeDef hrtc;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
const uint8_t BL_Version[2] = { MAJOR , MINOR};
const uint8_t g_pubkey_xy[64] = {0}; // Empty key for testing

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_CRC_Init(void);
static void MX_RTC_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
void Bootloader_JumpToApp(void);
void Bootloader_HandleUpdate(void);
BL_Status_t Bootloader_InternalVerify(uint32_t slot_addr, uint32_t slot_size);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void Bootloader_SetStatus(uint32_t status) {
    HAL_PWR_EnableBkUpAccess();
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, status);
    HAL_PWREx_DisableBkUpReg();
}

void BL_RequestUpdate(void) {
    HAL_PWR_EnableBkUpAccess();
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, 0xCAFEBABE); // Magic Flag
    NVIC_SystemReset();
    HAL_PWREx_DisableBkUpReg();
}

uint32_t BL_GetStatus(void) {
    return HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);
}

int BL_VerifySlot(uint32_t slot_id) {
    if(slot_id == 1) return Bootloader_InternalVerify(APP_DOWNLOAD_START_ADDR, APP_DOWNLOAD_SIZE);
    return Bootloader_InternalVerify(APP_ACTIVE_START_ADDR, APP_ACTIVE_SIZE);
}

/**
 * @brief   The Actual Shared API Table Instance.
 * @details This structure is populated with the real addresses of the Bootloader
 * functions. The `__attribute__((section))` forces the Linker to place
 * this variable at address 0x0800F000 (defined in .ld script).
 */
__attribute__((section(".shared_api_section")))
const Bootloader_API_t API_Table = {
    .magic_code = 0xDEADBEEF,
    .version = 0x0100,
    .RequestUpdate = BL_RequestUpdate,
    .GetBootStatus = BL_GetStatus,
    .VerifySlot = BL_VerifySlot
};



static uint32_t Find_Footer(uint32_t slot_start, uint32_t slot_size)
{
//    if (slot_size < sizeof(fw_footer_t)) return 0;
//
//    uint32_t footer_addr = slot_start + slot_size - sizeof(fw_footer_t);
//    if (footer_addr < slot_start) return 0; // overflow/wrap protection
//
//    const fw_footer_t *f = (const fw_footer_t *)footer_addr;
//
//    if (f->magic != FOOTER_MAGIC) return 0;
//    if (f->image_size == 0) return 0;
//    if (f->image_size & 0x3u) return 0; // word aligned
//
//    uint32_t image_end = slot_start + f->image_size;
//    if (image_end > footer_addr) return 0; // must not overlap footer
//
//    return footer_addr;

	// 1. Basic sanity checks on input
	if (slot_size < sizeof(fw_footer_t)) return 0;

	uint32_t slot_end = slot_start + slot_size;

	// 2. Scan backwards for the Magic Number
	// We step by 4 bytes because flash writes and ARM instructions are typically word-aligned.
	for (uint32_t addr = slot_end - 4; addr >= slot_start + sizeof(fw_footer_t); addr -= 4)
	{
		// --- CHECK 1: Magic Number ---
		if (*(uint32_t*)addr == FOOTER_MAGIC)
		{
			// Calculate where the footer would start if this is the magic number
			// Struct: [Signature (64)] [Version (4)] [Size (4)] [Magic (4)]
			// Footer Start = Magic_Address - (Sizeof_Struct - Sizeof_Magic)
			uint32_t footer_start = addr - (sizeof(fw_footer_t) - 4);
			const fw_footer_t *f = (const fw_footer_t *)footer_start;

			// --- CHECK 2: Version Sanity ---
			// Version 0 or 0xFFFFFFFF (erased flash) is likely invalid
			if (f->version == 0 || f->version == 0xFFFFFFFF) continue;

			// --- CHECK 3: Image Size Alignment ---
			// ARM Cortex-M vectors and flash programming are word-aligned.
			// If the size isn't a multiple of 4, it's likely garbage data.
			if ((f->image_size % 4) != 0) continue;

			// --- CHECK 4: Image Size Bounds ---
			// The image cannot be size 0, and it cannot be larger than the slot.
			if (f->image_size == 0 || f->image_size > slot_size) continue;

			// --- CHECK 5: Geometric Consistency ---
			// This is the most important check.
			// The footer MUST be located exactly at: [Slot_Start + Image_Size]
			// If the math doesn't add up, this is just a random "END!" string in the binary code.
			uint32_t expected_footer_addr = slot_start + f->image_size;

			if (expected_footer_addr == footer_start)
			{
				return footer_start; // All checks passed!
			}
		}
	}

	return 0; // No valid footer found

}

// STUB 1: Pretend to hash the flash memory
// Returns 1 (true) to simulate success
int Crypto_SHA256_Flash(const uint8_t *addr, uint32_t size, uint8_t *digest)
{
    // TODO: Later, implement the loop using tc_sha256_init/update/final
    // For now, do nothing and return success
    return 1;
}

// STUB 2: Pretend to verify the signature
// Returns 1 (true) to simulate a "valid" signature
int Crypto_ECDSA_P256_VerifyHash(const uint8_t *pubkey, const uint8_t *hash, const uint8_t *sig)
{
    // TODO: Later, implement using tc_ecdsa_verify
    // For now, return 1 so the bootloader thinks the image is valid
    return 1;
}

BL_Status_t Bootloader_InternalVerify(uint32_t slot_start, uint32_t slot_size)
{
    // 1) Find footer (deterministic)
    uint32_t footer_addr = Find_Footer(slot_start, slot_size);
    if (footer_addr == 0) return BL_ERR_FOOTER_NOT_FOUND;

    const fw_footer_t *f = (const fw_footer_t *)footer_addr;

    // 2) Image range checks (again, explicit)
    uint32_t slot_end = slot_start + slot_size;
    uint32_t image_end = slot_start + f->image_size;

    if (slot_end < slot_start) return BL_ERR_IMAGE_RANGE_BAD;    // overflow protection
    if (image_end > footer_addr) return BL_ERR_IMAGE_RANGE_BAD;
    if (image_end > slot_end) return BL_ERR_IMAGE_RANGE_BAD;


    // 4) Hash plaintext image bytes [slot_start .. slot_start+image_size)

    uint8_t hash[32];
    if (!Crypto_SHA256_Flash((const uint8_t *)slot_start, f->image_size, hash)) {
        return BL_ERR_HASH_FAIL;
    }

    // 5) Verify signature over hash
    if (!Crypto_ECDSA_P256_VerifyHash(g_pubkey_xy, hash, f->signature)) {
        return BL_ERR_SIG_FAIL;
    }


    return BL_OK;
}

/* Returns 1 on Success, 0 on Failure */
int Install_Update_Stream(uint8_t is_dry_run) {
    static uint8_t enc_buffer[ENC_CHUNK_SIZE];
    static uint8_t dec_buffer[DEC_CHUNK_SIZE];
    static uint8_t raw_buffer[RAW_CHUNK_SIZE];

    uint32_t read_addr = APP_DOWNLOAD_START_ADDR;
    uint32_t write_addr = APP_ACTIVE_START_ADDR;

    /* 1. Setup AES */
    struct tc_aes_key_sched_struct sched;
    uint8_t iv[16];
    uint8_t next_iv[16];

    /* Read Initial IV */
    memcpy(iv, (void*)read_addr, 16);
    read_addr += 16;
    tc_aes128_set_decrypt_key(&sched, AES_SECRET_KEY);

    /* 2. Process Loop */
    // Limit loop to Download Size to prevent reading garbage at end of flash
    uint32_t end_addr = APP_DOWNLOAD_START_ADDR + APP_DOWNLOAD_SIZE;

    while (read_addr < end_addr) {

        // A. Read Encrypted Chunk
        memcpy(enc_buffer, (void*)read_addr, ENC_CHUNK_SIZE);

        // Check for empty flash (0xFF) to stop early
        if (*(uint32_t*)enc_buffer == 0xFFFFFFFF) break;

        // B. Save IV for next chunk (CBC Chain)
        memcpy(next_iv, &enc_buffer[ENC_CHUNK_SIZE - 16], 16);

        // C. Decrypt
        if (tc_cbc_mode_decrypt(dec_buffer, ENC_CHUNK_SIZE, enc_buffer, ENC_CHUNK_SIZE, iv, &sched) == 0) {
             return 0; // Decrypt Error
        }
        memcpy(iv, next_iv, 16); // Update IV

        // D. Decompress (LZ4)
        int bytes_out = LZ4_decompress_safe((const char*)dec_buffer,
                                            (char*)raw_buffer,
                                            ENC_CHUNK_SIZE,
                                            RAW_CHUNK_SIZE);

        if (bytes_out < 0) return 0; // CORRUPTION DETECTED!

        // E. Write (ONLY IF NOT DRY RUN)
        if (!is_dry_run) {
            for (int i = 0; i < bytes_out; i += 4) {
                if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, write_addr + i, *(uint32_t*)(raw_buffer + i)) != HAL_OK) {
                    return 0; // Flash Write Error
                }
            }
        }

        read_addr += ENC_CHUNK_SIZE;
        write_addr += bytes_out;
    }

    return 1; // Stream is valid
}

void Bootloader_HandleUpdate(void) {

    // --- STEP 1: SAFETY CHECK (DRY RUN) ---
    // We process the whole file but DO NOT erase or write anything.
    // This checks if the file is corrupt, truncated, or has the wrong key.

    if (Install_Update_Stream(1) == 0) {
        // DRY RUN FAILED!
        // The download slot contains garbage.
        // We abort immediately. The Active Slot is still perfectly valid.
        Bootloader_SetStatus(BL_STATUS_ERROR);

        // Optional: Erase the bad download slot so we don't try again
        // Erase_Download_Slot();

        NVIC_SystemReset(); // Reboot back to old App
        return;
    }

    // --- STEP 2: PREPARE ACTIVE SLOT ---
    // If we got here, we know the Download Slot is readable and valid LZ4.

    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef EraseInit;
    uint32_t SectorError;
    EraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInit.Sector = FLASH_SECTOR_2;
    EraseInit.NbSectors = 4; // Sectors 2,3,4,5
    EraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    if (HAL_FLASHEx_Erase(&EraseInit, &SectorError) != HAL_OK) {
        HAL_FLASH_Lock();
        Error_Handler(); // Hardware failure
    }

    // --- STEP 3: REAL INSTALLATION ---
    // This time we pass '0' to enable writing.
    if (Install_Update_Stream(0) == 1) {
        HAL_FLASH_Lock();

        // --- STEP 4: FINAL SIGNATURE VERIFY ---
        // We verified the stream structure (LZ4), but now we verify
        // the cryptographic signature of the code we just wrote.
        if (Bootloader_InternalVerify(APP_ACTIVE_START_ADDR, APP_ACTIVE_SIZE) == 1) {
            Bootloader_SetStatus(BL_STATUS_UPDATED);
            NVIC_SystemReset();
        } else {
            // Signature mismatch (Malicious file?)
            // We are now bricked (Active Slot is erased/written but invalid).
            Error_Handler();
        }
    } else {
        // Write failed halfway?
        HAL_FLASH_Lock();
        Error_Handler();
    }
}

void Bootloader_JumpToApp(void) {
    uint32_t app_addr = APP_ACTIVE_START_ADDR;
    uint32_t app_stack_addr = *(volatile uint32_t*)app_addr;
    uint32_t app_reset_handler = *(volatile uint32_t*)(app_addr + 4);

    if ((app_stack_addr & 0x20000000) != 0x20000000) {
        return;
    }

    // 1. CLEAR BUFFERS
    // Wait for UART to finish sending (Important for IMPRECISERR)
    HAL_UART_Transmit(&huart1, (uint8_t*)"Jump\r\n", 6, 100);

    // 2. DISABLE MPU & CACHE
    HAL_MPU_Disable();

    // 3. DISABLE SYSTICK (Crucial!)
    SysTick->CTRL = 0;

    // 5. DE-INIT
    HAL_DeInit();

    // 6. DISABLE INTERRUPTS
    __disable_irq();

    // 7. CLEAR PENDING INTERRUPTS
    for (int i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    // 8. RELOCATE VECTOR TABLE
    SCB->VTOR = app_addr;

    // 9. JUMP
    __set_MSP(app_stack_addr);
    void (*pJump)(void) = (void (*)(void))app_reset_handler;
    pJump();
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

  /* MPU Configuration--------------------------------------------------------*/
  //MPU_Config();

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
  MX_CRC_Init();
  MX_RTC_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  printf("Starting Bootloader Version-(%d,%d)\r\n",BL_Version[0],BL_Version[1]);

  /* Infinite loop */
    /* USER CODE BEGIN WHILE */


  /* 1. Check for Update Request (Magic Flag) */

    /* if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) == 0xCAFEBABE) {
  	  /* Clear flag */
  /*
  	  HAL_PWR_EnableBkUpAccess();
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, 0x00);

        /* Perform Update (Decrypt -> Decompress -> Flash) */
   /*     Bootloader_HandleUpdate();
    }
    */
  // --- BYPASS MODE: Skip Footer/Crypto Checks ---
    printf("Bypassing verification... Jumping to App!\r\n");

    // Try to jump to the Active Slot
    Bootloader_JumpToApp();

    // If the code reaches here, the Jump FAILED (e.g., Flash was empty)
    printf("Jump Failed! (No valid app found at 0x08010000)\r\n");



/*    if (Bootloader_InternalVerify(APP_ACTIVE_START_ADDR, APP_ACTIVE_SIZE) == BL_OK) {
          Bootloader_JumpToApp();
      }
    else {
           Verification Failed or Empty Slot
          printf("No valid application found. Halting.\r\n");

          // We must stay here! Do not exit main.
          while (1)
          {
          	HAL_GPIO_TogglePin(GPIOI, GPIO_PIN_1);
          	HAL_Delay(500);
          }
         }*/
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
  hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
  hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
  hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

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

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

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

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 0x1;
  sDate.Year = 0x0;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
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

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */


/**
 * @brief Redirects standard output to USART1 using polling.
 *
 * Sends a buffer over USART1 and inserts '\r' before '\n'
 * for proper terminal formatting.
 *
 * @param file File descriptor (unused)
 * @param ptr  Data buffer to send
 * @param len  Number of bytes to send
 * @return Number of bytes written
 *
 * @author Omert2004
 */
int _write(int file, char *ptr, int len)
{
    for (int i = 0; i < len; i++)
    {
        // Check for new line character to fix terminal formatting
        if (ptr[i] == '\n')
        {
            while (!(USART1->ISR & USART_ISR_TXE));
            USART1->TDR = '\r'; // Send Carriage Return
        }
        // 1. Wait for the Transmit Data Register Empty (TXE) flag
        while (!(USART1->ISR & USART_ISR_TXE));

        // 2. Write the character to the Transmit Data Register (TDR)
        USART1->TDR = (uint8_t)ptr[i];
    }
    return len;
}


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
  MPU_InitStruct.Size = MPU_REGION_SIZE_1MB;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_PRIV_RW_URO;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
