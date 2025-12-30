import sys
import struct
import os
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad
from ecdsa import SigningKey, NIST256p
from cryptography.hazmat.primitives.ciphers import modes
import hashlib

# --- Configuration ---
INPUT_FIRMWARE = sys.argv[1] if len(sys.argv) > 1 else "deneme_application.bin"
OUTPUT_UPDATE  = "update_encrypted.bin"
KEY_FILE       = "secret.key"
PRIV_KEY_FILE  = "private.pem"

# Footer Constants (Must match C code firmware_footer.h)
HEADER_MAGIC     = 0xDEADBEEF 
FOOTER_MAGIC     = 0x454E4421 # "END!"
FIRMWARE_VERSION = 2  

def generate_update():
    print(f"--- Generating Secure Update for {INPUT_FIRMWARE} ---")

    if not os.path.exists(INPUT_FIRMWARE):
        print(f"Error: {INPUT_FIRMWARE} not found.")
        return

    with open(INPUT_FIRMWARE, "rb") as f:
        fw_data = f.read()
    
    # 1. Pad Code to 16 bytes (AES requirement)
    fw_data_padded = pad(fw_data, 16)
    
    # 2. Sign the Code
    if not os.path.exists(PRIV_KEY_FILE):
        print("Error: private.pem not found.")
        return

    with open(PRIV_KEY_FILE, "rb") as f:
        sk = SigningKey.from_pem(f.read())

    # Calculate Hash of the padded code
    fw_hash = hashlib.sha256(fw_data_padded).digest()
    signature = sk.sign_digest(fw_hash)

    # 3. Create Footer [Signature(64) | Version(4) | Size(4) | Magic(4)]
    # This structure MUST match your C code's Firmware_Footer_t
    footer = struct.pack('<64sIII', 
                         signature, 
                         FIRMWARE_VERSION, 
                         len(fw_data_padded), # Size of code verified
                         FOOTER_MAGIC)
    
    # 4. Create Signed Binary (Code + Footer)
    signed_binary = fw_data_padded + footer
    final_verify_size = len(signed_binary)
    
    print(f"[OK] Signed Binary Size: {final_verify_size} bytes")
    print(f"     (Code: {len(fw_data_padded)} + Footer: {len(footer)})")

    # 5. Encrypt the Signed Binary
    # We must pad again because the footer (76 bytes) might misalign the total size
    signed_binary_padded = pad(signed_binary, 16)
    
    if not os.path.exists(KEY_FILE):
        print("Error: secret.key not found.")
        return

    with open(KEY_FILE, "rb") as f:
        aes_key = f.read(16) 

    iv = os.urandom(16) 
    cipher = AES.new(aes_key, AES.MODE_CBC, iv)
    
    encrypted_data = cipher.encrypt(signed_binary_padded)
    
    # 6. Create Update Header
    # fw_size = Size of (Code + Footer). Verification uses this to find the footer.
    header = struct.pack('<IIII16s32x', 
                         HEADER_MAGIC, 
                         FIRMWARE_VERSION, 
                         final_verify_size,   # fw_size
                         len(encrypted_data), # padded_size (for decryption loop)
                         iv)

    # 7. Save File (Header + Encrypted Data)
    # Note: We added 64 bytes of dummy padding to skip the "Signature" area 
    # if your C code expects offset 128 for data.
    dummy_gap = b'\x00' * 64 
    final_package = header + dummy_gap + encrypted_data

    with open(OUTPUT_UPDATE, "wb") as f:
        f.write(final_package)

    print(f"[OK] Update saved to: {OUTPUT_UPDATE}")
    print("----------------------------------------")
    print("NEXT STEP: Flash this file to Slot 6 (0x08080000)")

if __name__ == "__main__":
    generate_update()