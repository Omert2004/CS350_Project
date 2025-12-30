import sys
import struct
import hashlib
import os
from ecdsa import SigningKey, NIST256p
import lz4.frame
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import padding

# Configuration (Must match C code!)
AES_KEY = b'\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F' # 16 bytes
FOOTER_MAGIC = 0x454E4421  # "END!" in hex
FIRMWARE_VERSION = 0x0100  # Version 1.0

def main():
    if len(sys.argv) != 4:
        print("Usage: python generate_update.py <input_bin> <output_enc> <private_key.pem>")
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]
    key_path = sys.argv[3]

    print(f"[1] Reading Firmware: {input_path}")
    with open(input_path, "rb") as f:
        firmware_bin = f.read()
    
    firmware_len = len(firmware_bin)
    print(f"    Original Size: {firmware_len} bytes")

    # --- STEP 1: SIGNING (ECDSA) ---
    # We sign the RAW firmware before compression/encryption
    print("[2] Calculating Signature...")
    sha256 = hashlib.sha256(firmware_bin).digest()
    
    with open(key_path) as f:
        sk = SigningKey.from_pem(f.read())
    
    # Deterministic signature (matches TinyCrypt expectations)
    signature = sk.sign_digest_deterministic(sha256, sigencode=lambda r, s, order: r.to_bytes(32, 'big') + s.to_bytes(32, 'big'))
    
    # Create the Footer Struct
    # Struct format: 64s (Sig) + I (Version) + I (Size) + I (Magic)
    footer = struct.pack('<64sIII', 
                         signature, 
                         FIRMWARE_VERSION, 
                         firmware_len, 
                         FOOTER_MAGIC)
    
    combined_data = firmware_bin + footer
    print(f"    Footer appended. New Size: {len(combined_data)} bytes")

    # --- STEP 2: COMPRESSION (LZ4) ---
    print("[3] Compressing (LZ4)...")
    # We use block compression to be compatible with standard LZ4
    compressed_data = lz4.frame.compress(combined_data, compression_level=9)
    print(f"    Compressed Size: {len(compressed_data)} bytes")

    # --- STEP 3: ENCRYPTION (AES-128-CBC) ---
    print("[4] Encrypting (AES-128-CBC)...")
    
    # Generate a random IV (16 bytes)
    iv = os.urandom(16)
    
    # Pad data to 16-byte block size (PKCS7)
    padder = padding.PKCS7(128).padder()
    padded_data = padder.update(compressed_data) + padder.finalize()
    
    cipher = Cipher(algorithms.AES(AES_KEY), modes.CBC(iv), backend=default_backend())
    encryptor = cipher.encryptor()
    encrypted_data = encryptor.update(padded_data) + encryptor.finalize()

    # Prepend IV to the file (so Bootloader can read it)
    final_payload = iv + encrypted_data
    
    print(f"[5] Writing Output: {output_path}")
    with open(output_path, "wb") as f:
        f.write(final_payload)
    
    print("Done! Ready to flash.")

if __name__ == "__main__":
    main()