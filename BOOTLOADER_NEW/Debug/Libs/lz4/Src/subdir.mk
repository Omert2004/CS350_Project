################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Libs/lz4/Src/lz4.c 

OBJS += \
./Libs/lz4/Src/lz4.o 

C_DEPS += \
./Libs/lz4/Src/lz4.d 


# Each subdirectory must supply rules for building sources it contributes
Libs/lz4/Src/%.o Libs/lz4/Src/%.su Libs/lz4/Src/%.cyclo: ../Libs/lz4/Src/%.c Libs/lz4/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F746xx -c -I../Core/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F7xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/Oguzm/OneDrive - ozyegin.edu.tr/Desktop/Github_Projects/CS350_Project1/CS350_Project1/BOOTLOADER_NEW/Libs/lz4/Inc" -I"C:/Users/Oguzm/OneDrive - ozyegin.edu.tr/Desktop/Github_Projects/CS350_Project1/CS350_Project1/BOOTLOADER_NEW/Libs/tinycrypt/Inc" -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Libs-2f-lz4-2f-Src

clean-Libs-2f-lz4-2f-Src:
	-$(RM) ./Libs/lz4/Src/lz4.cyclo ./Libs/lz4/Src/lz4.d ./Libs/lz4/Src/lz4.o ./Libs/lz4/Src/lz4.su

.PHONY: clean-Libs-2f-lz4-2f-Src

