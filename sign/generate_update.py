import sys
import struct
import hashlib
import os
from ecdsa import SigningKey, NIST256p
import lz4.block  # Changed from lz4.frame to lz4.block
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import padding
import serial
import time


# Configuration
FOOTER_MAGIC = 0x454E4421
FIRMWARE_VERSION = 0x0100

# Constants for the "Chunking" Protocol
# We read 512 bytes of firmware -> Compress it -> Encrypt it into a 1024 byte chunk
RAW_CHUNK_IN_SIZE = 512  
ENC_CHUNK_OUT_SIZE = 1024

def main():
    if len(sys.argv) != 4:
        print("Usage: python generate_update.py <input_bin> <output_enc> <private_key.pem>")
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]
    key_path = sys.argv[3]

    # 1. Load Keys
    if not os.path.exists("secret.key"):
        print("Error: secret.key not found! Run keygen.py first.")
        sys.exit(1)
        
    with open("secret.key", "rb") as f:
        aes_key = f.read()

    print(f"[1] Reading Firmware: {input_path}")
    with open(input_path, "rb") as f:
        firmware_bin = f.read()
    
    firmware_len = len(firmware_bin)
    print(f"    Original Size: {firmware_len} bytes")

    # 2. Sign the Raw Firmware
    print("[2] Calculating Signature...")
    sha256 = hashlib.sha256(firmware_bin).digest()
    
    with open(key_path) as f:
        sk = SigningKey.from_pem(f.read())
    
    # Deterministic signature
    signature = sk.sign_digest_deterministic(sha256, sigencode=lambda r, s, order: r.to_bytes(32, 'big') + s.to_bytes(32, 'big'))
    
    # Create Footer
    footer = struct.pack('<64sIII', signature, FIRMWARE_VERSION, firmware_len, FOOTER_MAGIC)
    combined_data = firmware_bin + footer
    total_len = len(combined_data)

    # 3. Process in Chunks (Compress -> Pad -> Encrypt)
    print("[3] Processing Chunks (Compress -> Pad -> Encrypt)...")
    
    # Generate random IV for the first block
    iv = os.urandom(16)
    current_iv = iv
    
    final_payload = bytearray()
    final_payload += iv # Header IV
    
    # Iterate through firmware in 512-byte chunks
    for i in range(0, total_len, RAW_CHUNK_IN_SIZE):
        chunk = combined_data[i : i + RAW_CHUNK_IN_SIZE]
        
        # A. Compress (Block Mode)
        # return_bytearray=True ensures we get raw bytes without extra headers
        compressed_chunk = lz4.block.compress(chunk, store_size=False)
        
        comp_len = len(compressed_chunk)
        if comp_len >= (ENC_CHUNK_OUT_SIZE - 2):
            print("Error: Compressed data is larger than encryption container!")
            sys.exit(1)
            
        # B. Construct Payload: [Len (2 bytes)] [Compressed Data] [Padding (0s)]
        # This fits exactly into ENC_CHUNK_OUT_SIZE (1024)
        payload = struct.pack('<H', comp_len) + compressed_chunk
        padding_len = ENC_CHUNK_OUT_SIZE - len(payload)
        payload += b'\x00' * padding_len
        
        # C. Encrypt (AES-CBC)
        # We encrypt the 1024-byte payload. 
        # Note: No PKCS7 needed because we manually padded to 1024 (which is multiple of 16)
        cipher = Cipher(algorithms.AES(aes_key), modes.CBC(current_iv), backend=default_backend())
        encryptor = cipher.encryptor()
        encrypted_chunk = encryptor.update(payload) + encryptor.finalize()
        
        final_payload += encrypted_chunk
        
        # Update IV for next chunk (Chain)
        # The last 16 bytes of ciphertext become the IV for the next block
        current_iv = encrypted_chunk[-16:]

    print(f"[4] Writing Output: {output_path}")
    with open(output_path, "wb") as f:
        f.write(final_payload)
    
    print(f"Done! Encrypted Update Size: {len(final_payload)} bytes")
    uart_upload_option = input("Do you want to upload via UART? (y/n): ").strip().lower()
    if uart_upload_option == 'y':
        port = input("Enter UART port (e.g., COM3 or /dev/ttyUSB0): ").strip()
        baud = int(input("Enter baud rate (e.g., 115200): ").strip())
        uart_upload(port, baud, output_path)
        


# Optional UART upload
def uart_upload(port, baud, filename):
    print(f"[UART] Opening {port} @ {baud}")
    ser = serial.Serial(port, baud, timeout=1)
    time.sleep(2)  # STM32 reset / USB settle

    size = os.path.getsize(filename)
    print(f"[UART] Sending {filename} ({size} bytes)")

    with open(filename, "rb") as f:
        data = f.read()

    ser.write(size.to_bytes(4, "little"))  # önce boyut
    time.sleep(0.05)
    ser.write(data)

    print("[UART] Upload finished")
    ser.close() 

if __name__ == "__main__":
    main()