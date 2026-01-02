import serial
import time
import os
import struct
import zlib

output_path = "C:/Users/marda/Desktop/F4_Application/Debug/F4_Application.bin"

CHUNK_SIZE = 1024
MAGIC = b"BOOT"  # 0x424F4F54UL

def uart_upload(port, baud, filename, address):
    print(f"[UART] Opening {port} @ {baud}")
    ser = serial.Serial(port, baud, timeout=2)
    time.sleep(2)  # STM32 reset / USB settle

    # Read firmware
    with open(filename, "rb") as f:
        fw = f.read()

    fw_size = len(fw)
    #fw_crc  = zlib.crc32(fw) & 0xFFFFFFFF

    print(f"[UART] Firmware size : {fw_size} bytes")
    #print(f"[UART] Firmware CRC  : 0x{fw_crc:08X}")

    # ---- STEP 0: MUST SEND STARTING ADDRESS ----
    addressHEX = int(address, 16)
    print(f"[PY] Starting address = {address}")
    ser.write(struct.pack("<I", addressHEX))

    addrResp = ser.readline().decode(errors="ignore").strip()
    print("[UART] Response:", addrResp)

    # ---- STEP 1: MAGIC ----
    time.sleep(2)
    print("[UART] Sending MAGIC")
    ser.write(MAGIC)
    Magresp = ser.readline().decode(errors="ignore").strip()
    if Magresp == "MAGIC OK":
        print("[UART] MAGIC NUMBER SUCCESSFULLY READ")
        
    else:
        print("[UART] MAGIC FAILED")
        ser.close()
        return
    
    starting_process = ser.readline().decode(errors="ignore").strip()
    print("[UART] Response:", starting_process)

    # ---- STEP 3: MUST SEND FIRMWARE SIZE ----
    time.sleep(2)

    print(f"[PY] Firmware size = {fw_size} bytes")
    ser.write(struct.pack("<I", fw_size))  
    
    status = ser.readline().decode(errors="ignore").strip()
    print("[UART] Response:", status)
    status = ser.readline().decode(errors="ignore").strip()
    print("[UART] Response:", status)

    appAddr = ser.readline().decode(errors="ignore").strip()
    print("[UART] Response:", appAddr)
    
    
    if status == "SIZE OK":
        print("[UART] FIRMWARE SIZE SUCCESSFULLY SENT")
        print("[UART] Sending DATA")
        
        idx = 0
        total_len = fw_size
        
        while idx < total_len:
            # STM32'den 'A' veya 'BUFFER_DOLU' bekliyoruz
            status = ser.readline().decode(errors="ignore").strip()
            
            if status == 'A':
                # MCU hazir, byte'i gonder ve siradakine gec
                ser.write(bytes([fw[idx]]))
                idx += 1 

            elif status == "BUFFER_DOLU":
                print("[UART] Response:", status)
                # DEVAM mesajini bekle
                status = ser.readline().decode(errors="ignore").strip()
                print("[UART] Response:", status)

                checkAppsize = ser.readline().decode(errors="ignore").strip()
                print("[UART] Response:", checkAppsize)
                
                if status != "DEVAM":
                    print("[HATA] STM32 DEVAM demedi!")
                    break
                # DIKKAT: idx artirmiyoruz! Dongu basa doner, MCU tekrar 'A' gonderir, byte yazilir.
            
            elif status == "":
                print("[HATA] Timeout. STM32 cevap vermiyor.")
                break
        # --- DUZELTME BITTI ---
        
    else:
        print("[UART] SIZE FAILED")
    

    # ---- STEP 4: DATA ----
    starting_process = ser.readline().decode(errors="ignore").strip()
    print("[UART] sResponse:", starting_process)

    starting_process = ser.readline().decode(errors="ignore").strip()
    print("[UART] sResponse:", starting_process)

    starting_process = ser.readline().decode(errors="ignore").strip()
    print("[UART] sResponse:", starting_process)

    starting_process = ser.readline().decode(errors="ignore").strip()
    print("[UART] sResponse:", starting_process)
    
    print("[UART] Closing port")
    ser.close() 


# -------------------- MAIN --------------------

uart_upload_option = input("Do you want to upload via UART? (y/n): ").strip().lower()
if uart_upload_option == 'y':
    port = input("Enter UART port (e.g., COM7): ").strip()
    baud = int(input("Enter baud rate (115200 recommended): ").strip()) # 115200 to match baud at stm32 side
    startAdress = input("Enter starting address (hex, e.g. 8008000): ").strip()
    uart_upload(port, baud, output_path, startAdress)
    #uart_upload_alt()