################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Libs/tinycrypt/Src/subdir.mk
-include Libs/lz4/Src/subdir.mk
-include Drivers/STM32F7xx_HAL_Driver/Src/subdir.mk
-include Core/Startup/subdir.mk
-include Core/Src/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := BOOTLOADER_NEW
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
BOOTLOADER_NEW.elf \

MAP_FILES += \
BOOTLOADER_NEW.map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
BOOTLOADER_NEW.list \

OBJCOPY_BIN += \
BOOTLOADER_NEW.bin \


# All Target
all:
	+@$(MAKE) --no-print-directory main-build && $(MAKE) --no-print-directory post-build

# Main-build Target
main-build: BOOTLOADER_NEW.elf secondary-outputs

# Tool invocations
BOOTLOADER_NEW.elf BOOTLOADER_NEW.map: $(OBJS) $(USER_OBJS) C:\Users\Oguzm\OneDrive\ -\ ozyegin.edu.tr\Desktop\Github_Projects\CS350_Project1\CS350_Project1\BOOTLOADER_NEW\STM32F746NGHX_FLASH.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "BOOTLOADER_NEW.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m7 -T"C:\Users\Oguzm\OneDrive - ozyegin.edu.tr\Desktop\Github_Projects\CS350_Project1\CS350_Project1\BOOTLOADER_NEW\STM32F746NGHX_FLASH.ld" --specs=nosys.specs -Wl,-Map="BOOTLOADER_NEW.map" -Wl,--gc-sections -static -flto --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

BOOTLOADER_NEW.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "BOOTLOADER_NEW.list"
	@echo 'Finished building: $@'
	@echo ' '

BOOTLOADER_NEW.bin: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objcopy  -O binary $(EXECUTABLES) "BOOTLOADER_NEW.bin"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) BOOTLOADER_NEW.bin BOOTLOADER_NEW.elf BOOTLOADER_NEW.list BOOTLOADER_NEW.map default.size.stdout
	-@echo ' '

post-build:
	arm-none-eabi-objcopy -O binary "BOOTLOADER_NEW.elf" "BOOTLOADER_NEW.bin"
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_BIN)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified post-build

-include ../makefile.targets
