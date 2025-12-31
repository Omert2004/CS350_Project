import sys
import struct
import os
import hashlib
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad
from ecdsa import SigningKey

# --- Configuration ---
FOOTER_MAGIC = 0x454E4421  # "END!"
FIRMWARE_VERSION = 0x0100  # 1.0.0

KEY_FILE = "private.pem"
AES_KEY_FILE = "secret.key"
OUTPUT_FILE = "update_encrypted.bin"

def generate_update(input_file):
    if not os.path.exists(input_file):
        print(f"Error: Input file '{input_file}' not found.")
        return

    # 1. Load Keys
    print(f"Loading keys...")
    with open(KEY_FILE, "rb") as f:
        sk = SigningKey.from_pem(f.read())
    
    with open(AES_KEY_FILE, "rb") as f:
        aes_key = f.read()
        if len(aes_key) != 16:
            print(f"Error: AES key must be 16 bytes (currently {len(aes_key)}).")
            return

    # 2. Read Firmware
    print(f"Reading firmware: {input_file}")
    with open(input_file, "rb") as f:
        fw_data = f.read()
    
    # 3. Encrypt Firmware (AES-CBC)
    print("Encrypting firmware...")
    iv = os.urandom(16)
    cipher = AES.new(aes_key, AES.MODE_CBC, iv)
    padded_data = pad(fw_data, AES.block_size)
    encrypted_data = cipher.encrypt(padded_data)
    
    # Payload = IV + Encrypted Data
    payload = iv + encrypted_data
    payload_size = len(payload)
    print(f"  Encrypted Payload Size: {payload_size} bytes")

    # 4. Sign the Payload
    print("Signing payload...")
    h = hashlib.sha256(payload).digest()
    signature = sk.sign_digest(h)

    # 5. Create Footer (MATCHING C STRUCT)
    # C Struct:
    #   uint32_t version;
    #   uint32_t size;
    #   uint8_t  signature[64];
    #   uint32_t magic;
    
    footer = struct.pack('<II', FIRMWARE_VERSION, payload_size) + signature + struct.pack('<I', FOOTER_MAGIC)

    # 6. Write Output
    final_data = payload + footer
    with open(OUTPUT_FILE, "wb") as f:
        f.write(final_data)

    print(f"\n[SUCCESS] Update package created: {OUTPUT_FILE}")
    print(f"Total File Size: {len(final_data)} bytes")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python generate_update.py <application.bin>")
    else:
        generate_update(sys.argv[1])