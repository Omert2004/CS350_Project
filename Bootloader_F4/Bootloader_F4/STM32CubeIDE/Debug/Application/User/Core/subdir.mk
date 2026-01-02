################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Application/User/Core/BL_Functions.c \
../Application/User/Core/Cyptology_Control.c \
C:/Users/marda/Desktop/Bootloader_F4/Core/Src/gpio.c \
../Application/User/Core/jump_to_app.c \
../Application/User/Core/keys.c \
C:/Users/marda/Desktop/Bootloader_F4/Core/Src/main.c \
C:/Users/marda/Desktop/Bootloader_F4/Core/Src/stm32f4xx_hal_msp.c \
C:/Users/marda/Desktop/Bootloader_F4/Core/Src/stm32f4xx_it.c \
../Application/User/Core/syscalls.c \
../Application/User/Core/sysmem.c \
../Application/User/Core/tiny_printf.c \
../Application/User/Core/tiny_scanf.c \
C:/Users/marda/Desktop/Bootloader_F4/Core/Src/usart.c 

OBJS += \
./Application/User/Core/BL_Functions.o \
./Application/User/Core/Cyptology_Control.o \
./Application/User/Core/gpio.o \
./Application/User/Core/jump_to_app.o \
./Application/User/Core/keys.o \
./Application/User/Core/main.o \
./Application/User/Core/stm32f4xx_hal_msp.o \
./Application/User/Core/stm32f4xx_it.o \
./Application/User/Core/syscalls.o \
./Application/User/Core/sysmem.o \
./Application/User/Core/tiny_printf.o \
./Application/User/Core/tiny_scanf.o \
./Application/User/Core/usart.o 

C_DEPS += \
./Application/User/Core/BL_Functions.d \
./Application/User/Core/Cyptology_Control.d \
./Application/User/Core/gpio.d \
./Application/User/Core/jump_to_app.d \
./Application/User/Core/keys.d \
./Application/User/Core/main.d \
./Application/User/Core/stm32f4xx_hal_msp.d \
./Application/User/Core/stm32f4xx_it.d \
./Application/User/Core/syscalls.d \
./Application/User/Core/sysmem.d \
./Application/User/Core/tiny_printf.d \
./Application/User/Core/tiny_scanf.d \
./Application/User/Core/usart.d 


# Each subdirectory must supply rules for building sources it contributes
Application/User/Core/%.o Application/User/Core/%.su Application/User/Core/%.cyclo: ../Application/User/Core/%.c Application/User/Core/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../../Core/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../Drivers/CMSIS/Include -I"C:/Users/marda/Desktop/Bootloader_F4/STM32CubeIDE/Libs/lz4" -I"C:/Users/marda/Desktop/Bootloader_F4/STM32CubeIDE/Libs/tinycrypt" -I"C:/Users/marda/Desktop/Bootloader_F4/STM32CubeIDE/Libs/lz4/Inc" -I"C:/Users/marda/Desktop/Bootloader_F4/STM32CubeIDE/Libs/tinycrypt/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Application/User/Core/gpio.o: C:/Users/marda/Desktop/Bootloader_F4/Core/Src/gpio.c Application/User/Core/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../../Core/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../Drivers/CMSIS/Include -I"C:/Users/marda/Desktop/Bootloader_F4/STM32CubeIDE/Libs/lz4" -I"C:/Users/marda/Desktop/Bootloader_F4/STM32CubeIDE/Libs/tinycrypt" -I"C:/Users/marda/Desktop/Bootloader_F4/STM32CubeIDE/Libs/lz4/Inc" -I"C:/Users/marda/Desktop/Bootloader_F4/STM32CubeIDE/Libs/tinycrypt/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Application/User/Core/main.o: C:/Users/marda/Desktop/Bootloader_F4/Core/Src/main.c Application/User/Core/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../../Core/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../Drivers/CMSIS/Include -I"C:/Users/marda/Desktop/Bootloader_F4/STM32CubeIDE/Libs/lz4" -I"C:/Users/marda/Desktop/Bootloader_F4/STM32CubeIDE/Libs/tinycrypt" -I"C:/Users/marda/Desktop/Bootloader_F4/STM32CubeIDE/Libs/lz4/Inc" -I"C:/Users/marda/Desktop/Bootloader_F4/STM32CubeIDE/Libs/tinycrypt/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Application/User/Core/stm32f4xx_hal_msp.o: C:/Users/marda/Desktop/Bootloader_F4/Core/Src/stm32f4xx_hal_msp.c Application/User/Core/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../../Core/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../Drivers/CMSIS/Include -I"C:/Users/marda/Desktop/Bootloader_F4/STM32CubeIDE/Libs/lz4" -I"C:/Users/marda/Desktop/Bootloader_F4/STM32CubeIDE/Libs/tinycrypt" -I"C:/Users/marda/Desktop/Bootloader_F4/STM32CubeIDE/Libs/lz4/Inc" -I"C:/Users/marda/Desktop/Bootloader_F4/STM32CubeIDE/Libs/tinycrypt/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Application/User/Core/stm32f4xx_it.o: C:/Users/marda/Desktop/Bootloader_F4/Core/Src/stm32f4xx_it.c Application/User/Core/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../../Core/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../Drivers/CMSIS/Include -I"C:/Users/marda/Desktop/Bootloader_F4/STM32CubeIDE/Libs/lz4" -I"C:/Users/marda/Desktop/Bootloader_F4/STM32CubeIDE/Libs/tinycrypt" -I"C:/Users/marda/Desktop/Bootloader_F4/STM32CubeIDE/Libs/lz4/Inc" -I"C:/Users/marda/Desktop/Bootloader_F4/STM32CubeIDE/Libs/tinycrypt/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Application/User/Core/usart.o: C:/Users/marda/Desktop/Bootloader_F4/Core/Src/usart.c Application/User/Core/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../../Core/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../Drivers/CMSIS/Include -I"C:/Users/marda/Desktop/Bootloader_F4/STM32CubeIDE/Libs/lz4" -I"C:/Users/marda/Desktop/Bootloader_F4/STM32CubeIDE/Libs/tinycrypt" -I"C:/Users/marda/Desktop/Bootloader_F4/STM32CubeIDE/Libs/lz4/Inc" -I"C:/Users/marda/Desktop/Bootloader_F4/STM32CubeIDE/Libs/tinycrypt/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Application-2f-User-2f-Core

clean-Application-2f-User-2f-Core:
	-$(RM) ./Application/User/Core/BL_Functions.cyclo ./Application/User/Core/BL_Functions.d ./Application/User/Core/BL_Functions.o ./Application/User/Core/BL_Functions.su ./Application/User/Core/Cyptology_Control.cyclo ./Application/User/Core/Cyptology_Control.d ./Application/User/Core/Cyptology_Control.o ./Application/User/Core/Cyptology_Control.su ./Application/User/Core/gpio.cyclo ./Application/User/Core/gpio.d ./Application/User/Core/gpio.o ./Application/User/Core/gpio.su ./Application/User/Core/jump_to_app.cyclo ./Application/User/Core/jump_to_app.d ./Application/User/Core/jump_to_app.o ./Application/User/Core/jump_to_app.su ./Application/User/Core/keys.cyclo ./Application/User/Core/keys.d ./Application/User/Core/keys.o ./Application/User/Core/keys.su ./Application/User/Core/main.cyclo ./Application/User/Core/main.d ./Application/User/Core/main.o ./Application/User/Core/main.su ./Application/User/Core/stm32f4xx_hal_msp.cyclo ./Application/User/Core/stm32f4xx_hal_msp.d ./Application/User/Core/stm32f4xx_hal_msp.o ./Application/User/Core/stm32f4xx_hal_msp.su ./Application/User/Core/stm32f4xx_it.cyclo ./Application/User/Core/stm32f4xx_it.d ./Application/User/Core/stm32f4xx_it.o ./Application/User/Core/stm32f4xx_it.su ./Application/User/Core/syscalls.cyclo ./Application/User/Core/syscalls.d ./Application/User/Core/syscalls.o ./Application/User/Core/syscalls.su ./Application/User/Core/sysmem.cyclo ./Application/User/Core/sysmem.d ./Application/User/Core/sysmem.o ./Application/User/Core/sysmem.su ./Application/User/Core/tiny_printf.cyclo ./Application/User/Core/tiny_printf.d ./Application/User/Core/tiny_printf.o ./Application/User/Core/tiny_printf.su ./Application/User/Core/tiny_scanf.cyclo ./Application/User/Core/tiny_scanf.d ./Application/User/Core/tiny_scanf.o ./Application/User/Core/tiny_scanf.su ./Application/User/Core/usart.cyclo ./Application/User/Core/usart.d ./Application/User/Core/usart.o ./Application/User/Core/usart.su

.PHONY: clean-Application-2f-User-2f-Core

