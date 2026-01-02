################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Libs/tinycrypt/Src/aes_decrypt.c \
../Libs/tinycrypt/Src/aes_encrypt.c \
../Libs/tinycrypt/Src/cbc_mode.c \
../Libs/tinycrypt/Src/ecc.c \
../Libs/tinycrypt/Src/ecc_dsa.c \
../Libs/tinycrypt/Src/ecc_platform_specific.c \
../Libs/tinycrypt/Src/sha256.c \
../Libs/tinycrypt/Src/utils.c 

OBJS += \
./Libs/tinycrypt/Src/aes_decrypt.o \
./Libs/tinycrypt/Src/aes_encrypt.o \
./Libs/tinycrypt/Src/cbc_mode.o \
./Libs/tinycrypt/Src/ecc.o \
./Libs/tinycrypt/Src/ecc_dsa.o \
./Libs/tinycrypt/Src/ecc_platform_specific.o \
./Libs/tinycrypt/Src/sha256.o \
./Libs/tinycrypt/Src/utils.o 

C_DEPS += \
./Libs/tinycrypt/Src/aes_decrypt.d \
./Libs/tinycrypt/Src/aes_encrypt.d \
./Libs/tinycrypt/Src/cbc_mode.d \
./Libs/tinycrypt/Src/ecc.d \
./Libs/tinycrypt/Src/ecc_dsa.d \
./Libs/tinycrypt/Src/ecc_platform_specific.d \
./Libs/tinycrypt/Src/sha256.d \
./Libs/tinycrypt/Src/utils.d 


# Each subdirectory must supply rules for building sources it contributes
Libs/tinycrypt/Src/%.o Libs/tinycrypt/Src/%.su Libs/tinycrypt/Src/%.cyclo: ../Libs/tinycrypt/Src/%.c Libs/tinycrypt/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../../Core/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../Drivers/CMSIS/Include -I"C:/Users/marda/Desktop/Bootloader_F4/STM32CubeIDE/Libs/lz4" -I"C:/Users/marda/Desktop/Bootloader_F4/STM32CubeIDE/Libs/tinycrypt" -I"C:/Users/marda/Desktop/Bootloader_F4/STM32CubeIDE/Libs/lz4/Inc" -I"C:/Users/marda/Desktop/Bootloader_F4/STM32CubeIDE/Libs/tinycrypt/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Libs-2f-tinycrypt-2f-Src

clean-Libs-2f-tinycrypt-2f-Src:
	-$(RM) ./Libs/tinycrypt/Src/aes_decrypt.cyclo ./Libs/tinycrypt/Src/aes_decrypt.d ./Libs/tinycrypt/Src/aes_decrypt.o ./Libs/tinycrypt/Src/aes_decrypt.su ./Libs/tinycrypt/Src/aes_encrypt.cyclo ./Libs/tinycrypt/Src/aes_encrypt.d ./Libs/tinycrypt/Src/aes_encrypt.o ./Libs/tinycrypt/Src/aes_encrypt.su ./Libs/tinycrypt/Src/cbc_mode.cyclo ./Libs/tinycrypt/Src/cbc_mode.d ./Libs/tinycrypt/Src/cbc_mode.o ./Libs/tinycrypt/Src/cbc_mode.su ./Libs/tinycrypt/Src/ecc.cyclo ./Libs/tinycrypt/Src/ecc.d ./Libs/tinycrypt/Src/ecc.o ./Libs/tinycrypt/Src/ecc.su ./Libs/tinycrypt/Src/ecc_dsa.cyclo ./Libs/tinycrypt/Src/ecc_dsa.d ./Libs/tinycrypt/Src/ecc_dsa.o ./Libs/tinycrypt/Src/ecc_dsa.su ./Libs/tinycrypt/Src/ecc_platform_specific.cyclo ./Libs/tinycrypt/Src/ecc_platform_specific.d ./Libs/tinycrypt/Src/ecc_platform_specific.o ./Libs/tinycrypt/Src/ecc_platform_specific.su ./Libs/tinycrypt/Src/sha256.cyclo ./Libs/tinycrypt/Src/sha256.d ./Libs/tinycrypt/Src/sha256.o ./Libs/tinycrypt/Src/sha256.su ./Libs/tinycrypt/Src/utils.cyclo ./Libs/tinycrypt/Src/utils.d ./Libs/tinycrypt/Src/utils.o ./Libs/tinycrypt/Src/utils.su

.PHONY: clean-Libs-2f-tinycrypt-2f-Src

