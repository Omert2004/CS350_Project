# ===================== USER CONFIG =====================
ELF        := build/stm32_app.elf
BIN        := build/stm32_app.bin
UPDATE     := build/update.enc

PYTHON     := python
PORT       := COM5          # Linux: /dev/ttyUSB0, need to change (i think com8 on windows)
BAUD       := 115200

GEN_SCRIPT := generate_update.py
SEND_SCRIPT:= send_update.py
KEY        := private.pem
# ======================================================

all: build

build:
	@echo "[MAKE] Building firmware"
	@make -C build_dir   # or whatever your build command is

bin: $(ELF)
	@echo "[MAKE] ELF → BIN"
	arm-none-eabi-objcopy -O binary $(ELF) $(BIN)

update: bin
	@echo "[MAKE] Generating signed update"
	$(PYTHON) $(GEN_SCRIPT) $(BIN) $(UPDATE) $(KEY)

upload: update
	@echo "[MAKE] Uploading update via UART"
	$(PYTHON) $(SEND_SCRIPT) $(PORT) $(BAUD) $(UPDATE)

# one-command flow
flash: upload

clean:
	rm -f $(BIN) $(UPDATE)

.PHONY: all build bin update upload flash clean
