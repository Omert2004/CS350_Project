################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/BL_Functions.c \
../Core/Src/BL_Update_Part.c \
../Core/Src/deneme.c \
../Core/Src/jump_to_app.c \
../Core/Src/keys.c \
../Core/Src/main.c \
../Core/Src/stm32f7xx_hal_msp.c \
../Core/Src/stm32f7xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f7xx.c \
../Core/Src/tiny_printf.c 

OBJS += \
./Core/Src/BL_Functions.o \
./Core/Src/BL_Update_Part.o \
./Core/Src/deneme.o \
./Core/Src/jump_to_app.o \
./Core/Src/keys.o \
./Core/Src/main.o \
./Core/Src/stm32f7xx_hal_msp.o \
./Core/Src/stm32f7xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f7xx.o \
./Core/Src/tiny_printf.o 

C_DEPS += \
./Core/Src/BL_Functions.d \
./Core/Src/BL_Update_Part.d \
./Core/Src/deneme.d \
./Core/Src/jump_to_app.d \
./Core/Src/keys.d \
./Core/Src/main.d \
./Core/Src/stm32f7xx_hal_msp.d \
./Core/Src/stm32f7xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f7xx.d \
./Core/Src/tiny_printf.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F746xx -c -I../Core/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F7xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/Oguzm/OneDrive - ozyegin.edu.tr/Desktop/Github_Projects/CS350_Project1/CS350_Project1/BOOTLOADER_NEW/Libs/lz4/Inc" -I"C:/Users/Oguzm/OneDrive - ozyegin.edu.tr/Desktop/Github_Projects/CS350_Project1/CS350_Project1/BOOTLOADER_NEW/Libs/tinycrypt/Inc" -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/BL_Functions.cyclo ./Core/Src/BL_Functions.d ./Core/Src/BL_Functions.o ./Core/Src/BL_Functions.su ./Core/Src/BL_Update_Part.cyclo ./Core/Src/BL_Update_Part.d ./Core/Src/BL_Update_Part.o ./Core/Src/BL_Update_Part.su ./Core/Src/deneme.cyclo ./Core/Src/deneme.d ./Core/Src/deneme.o ./Core/Src/deneme.su ./Core/Src/jump_to_app.cyclo ./Core/Src/jump_to_app.d ./Core/Src/jump_to_app.o ./Core/Src/jump_to_app.su ./Core/Src/keys.cyclo ./Core/Src/keys.d ./Core/Src/keys.o ./Core/Src/keys.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/stm32f7xx_hal_msp.cyclo ./Core/Src/stm32f7xx_hal_msp.d ./Core/Src/stm32f7xx_hal_msp.o ./Core/Src/stm32f7xx_hal_msp.su ./Core/Src/stm32f7xx_it.cyclo ./Core/Src/stm32f7xx_it.d ./Core/Src/stm32f7xx_it.o ./Core/Src/stm32f7xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32f7xx.cyclo ./Core/Src/system_stm32f7xx.d ./Core/Src/system_stm32f7xx.o ./Core/Src/system_stm32f7xx.su ./Core/Src/tiny_printf.cyclo ./Core/Src/tiny_printf.d ./Core/Src/tiny_printf.o ./Core/Src/tiny_printf.su

.PHONY: clean-Core-2f-Src

