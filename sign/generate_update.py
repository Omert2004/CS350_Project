import struct
import hashlib
import os
from ecdsa import SigningKey
from Crypto.Cipher import AES

# --- Configuration ---
INPUT_BIN = "Application1.bin"       # Put your bin file here
OUTPUT_BIN = "update_encrypted.bin"  # File to flash to Slot 6 (0x08080000)
PRIVATE_KEY = "private.pem"
AES_KEY = "secret.key"

FOOTER_MAGIC = 0x454E4421  # "END!" in hex
VERSION = 0x00000002       # Version 2 (or increment as needed)

def pad_data(data):
    # Pad with 0xFF to multiple of 16 bytes (AES Block Size)
    length = len(data)
    remainder = length % 16
    if remainder != 0:
        padding = b'\xFF' * (16 - remainder)
        return data + padding
    return data

def main():
    print(f"--- Generating Update for {INPUT_BIN} ---")

    # 1. Load Keys
    try:
        with open(PRIVATE_KEY, "rb") as f:
            sk = SigningKey.from_pem(f.read())
        with open(AES_KEY, "rb") as f:
            aes_key = f.read()
    except FileNotFoundError:
        print("ERROR: Keys not found! Run 'keygen.py' first.")
        return

    # 2. Read Firmware (Plaintext)
    try:
        with open(INPUT_BIN, "rb") as f:
            firmware = f.read()
    except FileNotFoundError:
        print(f"ERROR: Could not find '{INPUT_BIN}' in this folder.")
        return

    # 3. Sign the Plaintext
    # We hash the RAW firmware before encryption
    sha = hashlib.sha256()
    sha.update(firmware)
    digest = sha.digest()
    signature = sk.sign(digest, hashfunc=hashlib.sha256)
    
    print(f"[OK] Firmware Size: {len(firmware)} bytes")
    print(f"[OK] SHA-256 Digest: {digest.hex()[:10]}...")
    print(f"[OK] Signature generated.")

    # 4. Append Footer
    # Struct: [Version(4)] [Size(4)] [Signature(64)] [Magic(4)]
    # Size is the plaintext size (excluding footer)
    footer = struct.pack("<II64sI", VERSION, len(firmware), signature, FOOTER_MAGIC)
    plaintext_image = firmware + footer

    # 5. Encrypt (AES-128)
    # We pad the (Firmware + Footer) to 16-byte boundary
    plaintext_padded = pad_data(plaintext_image)
    
    cipher = AES.new(aes_key, AES.MODE_ECB)
    encrypted_image = cipher.encrypt(plaintext_padded)

    # 6. Save
    with open(OUTPUT_BIN, "wb") as f:
        f.write(encrypted_image)

    print(f"[OK] Encrypted update saved to: {OUTPUT_BIN}")
    print(f"     Total Size: {len(encrypted_image)} bytes")
    print("-" * 40)
    print("NEXT STEPS:")
    print(f"1. Flash '{OUTPUT_BIN}' to address 0x08080000 (Slot 6)")
    print("2. Reset the board.")

if __name__ == "__main__":
    main()