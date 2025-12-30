import sys
import struct
import os
import hashlib

# --- Library Imports ---
# Uses 'ecdsa' for signing (matches your keygen)
# Uses 'cryptography' for AES (modern, standard library)
from ecdsa import SigningKey, NIST256p
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives import padding
from cryptography.hazmat.backends import default_backend

def generate_update(input_file, output_file, key_file, secret_key_file):
    print(f"Processing: {input_file} -> {output_file}")

    # 1. Load the Private Key (for Signing)
    with open(key_file, "rb") as f:
        sk = SigningKey.from_pem(f.read())

    # 2. Load the AES Secret Key (for Encryption)
    with open(secret_key_file, "rb") as f:
        aes_key = f.read()
        # Ensure key is 16, 24, or 32 bytes
        if len(aes_key) not in [16, 24, 32]:
            print(f"Error: AES key must be 16, 24, or 32 bytes. Found {len(aes_key)}")
            return

    # 3. Read the Input Firmware
    with open(input_file, "rb") as f:
        firmware_data = f.read()

    # 4. Apply Padding (REQUIRED for AES-CBC)
    # The bootloader typically decrypts in 16-byte blocks.
    # PKCS7 is the standard padding method.
    padder = padding.PKCS7(128).padder() # 128-bit block size for AES
    padded_data = padder.update(firmware_data) + padder.finalize()

    # 5. Encrypt using AES-CBC
    iv = os.urandom(16) # Generate random 16-byte Initialization Vector
    cipher = Cipher(algorithms.AES(aes_key), modes.CBC(iv), backend=default_backend())
    encryptor = cipher.encryptor()
    encrypted_data = encryptor.update(padded_data) + encryptor.finalize()

    # 6. Construct the Payload
    # The payload is what the bootloader actually writes to flash (excluding the header)
    # Structure: [IV (16 bytes)] + [Encrypted Firmware]
    payload = iv + encrypted_data
    payload_size = len(payload)

    # 7. Sign the Payload
    # We sign the HASH of the payload.
    # The C code must calculate the hash of (IV + Encrypted) and verify this signature.
    signature = sk.sign(payload, hashfunc=hashlib.sha256)

    # 8. Write the Update File (Binary Format)
    # Header Format:
    # - Size (4 bytes, Little Endian) -> Size of (IV + Encrypted Data)
    # - Signature (64 bytes, Raw R+S)
    # - Payload (IV + Encrypted Data)
    with open(output_file, "wb") as f:
        # Write Size
        f.write(struct.pack('<I', payload_size))
        
        # Write Signature
        f.write(signature)
        
        # Write IV and Encrypted Data
        f.write(payload)

    # --- Debug Info ---
    print("\n--- Summary ---")
    print(f"Original Size:  {len(firmware_data)} bytes")
    print(f"Padded Size:    {len(padded_data)} bytes (Multiple of 16)")
    print(f"Payload Size:   {payload_size} bytes (Size written to header)")
    print(f"Signature Len:  {len(signature)} bytes")
    print(f"IV:             {iv.hex()}")
    print("----------------\nFile generated successfully.")

if __name__ == "__main__":
    # --- Configuration ---
    # Change these filenames if necessary
    INPUT_BIN = "Application1.bin"       # Your compiled application binary
    OUTPUT_BIN = "update_encrypted.bin"  # The file to flash to Sector 6
    PRIV_KEY = "private.pem"             # From keygen.py
    AES_KEY = "secret.key"               # Raw binary key file

    # Run generation
    if not os.path.exists(INPUT_BIN):
        print(f"Error: {INPUT_BIN} not found. Please compile Application1 first.")
    else:
        generate_update(INPUT_BIN, OUTPUT_BIN, PRIV_KEY, AES_KEY)