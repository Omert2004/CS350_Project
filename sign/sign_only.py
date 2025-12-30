import sys
import struct
import hashlib
from ecdsa import SigningKey, NIST256p
from Crypto.Cipher import AES

# --- CONFIGURATION ---
FOOTER_MAGIC = 0x454E4421
FIRMWARE_VERSION = 0x0100 
PRIVATE_KEY_PATH = "private.pem"

# AES Key (Must match the one in Core/Src/keys.c)
# 0x29, 0x38, 0x8C, 0x76, 0x89, 0xE8, 0x9D, 0xA1, 0x0F, 0x57, 0xE5, 0xA1, 0xDE, 0xE0, 0xE5, 0x7F
AES_KEY = b'\x29\x38\x8C\x76\x89\xE8\x9D\xA1\x0F\x57\xE5\xA1\xDE\xE0\xE5\x7F'
# ---------------------

def sign_and_encrypt_firmware(input_bin, output_bin):
    try:
        print(f"[+] Processing: {input_bin}")
        
        # 1. Read the Input Application
        with open(input_bin, 'rb') as f:
            firmware_data = f.read()

        fw_size = len(firmware_data)
        print(f"    Original Firmware Size: {fw_size} bytes")
        
        # 2. Calculate SHA-256 Hash of the Firmware ONLY
        sha256 = hashlib.sha256()
        sha256.update(firmware_data)
        digest = sha256.digest()

        # 3. Sign the Hash (ECDSA P-256)
        try:
            sk = SigningKey.from_pem(open(PRIVATE_KEY_PATH).read())
        except FileNotFoundError:
            print(f"Error: Private key file '{PRIVATE_KEY_PATH}' not found!")
            sys.exit(1)
            
        signature = sk.sign_digest(digest, sigencode=None) # Raw 64 bytes (r+s)

        # 4. Create the Footer
        # struct fw_footer_t {
        #     uint32_t version;
        #     uint32_t size;
        #     uint8_t signature[64];
        #     uint32_t magic;
        # }
        footer = struct.pack('<II64sI', 
                             FIRMWARE_VERSION, 
                             fw_size, 
                             signature, 
                             FOOTER_MAGIC)

        # 5. Combine Firmware + Footer
        payload = firmware_data + footer
        payload_len = len(payload)
        print(f"    Payload Size (App + Footer): {payload_len} bytes")

        # 6. Apply Padding for AES (Block size = 16 bytes)
        # We perform standard padding (or 0xFF padding since it's flash memory)
        # Here we use 0xFF because that is the state of erased flash.
        pad_needed = 16 - (payload_len % 16)
        if pad_needed != 16:
            payload += b'\xFF' * pad_needed
            print(f"    Added {pad_needed} bytes of padding for AES alignment.")
        
        # 7. Encrypt (AES-128 ECB)
        # Note: We use ECB to match the simple block-by-block decryption in the Bootloader.
        cipher = AES.new(AES_KEY, AES.MODE_ECB)
        encrypted_blob = cipher.encrypt(payload)

        # 8. Write the Final Output
        with open(output_bin, 'wb') as f:
            f.write(encrypted_blob)

        print(f"[+] Success! Encrypted update generated at: {output_bin}")
        print(f"    Final File Size: {len(encrypted_blob)} bytes")
        print("    (This file can be safely transmitted to Slot 2)")

    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python sign_only.py <input_app.bin> <output_encrypted.bin>")
    else:
        sign_and_encrypt_firmware(sys.argv[1], sys.argv[2])