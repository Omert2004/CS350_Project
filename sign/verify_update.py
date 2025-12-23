import sys
import struct
import hashlib
import os
import lz4.block
from ecdsa import VerifyingKey, NIST256p, BadSignatureError
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend

# Configuration must match generate_update.py
FOOTER_MAGIC = 0x454E4421
ENC_CHUNK_SIZE = 1024

def main():
    if len(sys.argv) != 3:
        print("Usage: python verify_update.py <encrypted_update.bin> <public_key.pem>")
        sys.exit(1)

    input_path = sys.argv[1]
    key_path = sys.argv[2]

    # 1. Load AES Key
    if not os.path.exists("secret.key"):
        print("Error: secret.key not found!")
        sys.exit(1)
        
    with open("secret.key", "rb") as f:
        aes_key = f.read()

    print(f"[1] Reading Encrypted Update: {input_path}")
    with open(input_path, "rb") as f:
        encrypted_data = f.read()

    # 2. Setup Decryption
    # First 16 bytes are the Initial IV
    iv = encrypted_data[:16]
    ciphertext_body = encrypted_data[16:]
    
    print(f"    Total Size: {len(encrypted_data)} bytes")
    print(f"    IV: {iv.hex()}")

    decrypted_firmware = bytearray()
    
    # 3. Decrypt Loop (Mimics Bootloader C Code)
    # We process in 1024-byte chunks
    current_iv = iv
    
    for i in range(0, len(ciphertext_body), ENC_CHUNK_SIZE):
        chunk = ciphertext_body[i : i + ENC_CHUNK_SIZE]
        
        # A. Decrypt
        cipher = Cipher(algorithms.AES(aes_key), modes.CBC(current_iv), backend=default_backend())
        decryptor = cipher.decryptor()
        decrypted_chunk = decryptor.update(chunk) + decryptor.finalize()
        
        # Update IV for next chunk (Chain Rule: IV is previous Ciphertext)
        current_iv = chunk[-16:]
        
        # B. Parse Header [Len (2 bytes)]
        payload_len = struct.unpack('<H', decrypted_chunk[:2])[0]
        
        # C. Decompress
        compressed_payload = decrypted_chunk[2 : 2 + payload_len]
        
        try:
            raw_data = lz4.block.decompress(compressed_payload)
            decrypted_firmware += raw_data
        except Exception as e:
            print(f"ERROR: Decompression failed at chunk {i // ENC_CHUNK_SIZE}!")
            print(e)
            sys.exit(1)

    print(f"[2] Decryption & Decompression Successful.")
    print(f"    Reconstructed Size: {len(decrypted_firmware)} bytes")

    # 4. Extract Footer and Verify
    # Footer is the last 76 bytes (64 Sig + 4 Ver + 4 Len + 4 Magic)
    footer_fmt = '<64sIII'
    footer_size = struct.calcsize(footer_fmt)
    
    firmware_content = decrypted_firmware[:-footer_size]
    footer_data = decrypted_firmware[-footer_size:]
    
    signature, version, fw_len, magic = struct.unpack(footer_fmt, footer_data)
    
    print(f"[3] Analyzing Footer:")
    print(f"    Magic: 0x{magic:08X} (Expected: 0x{FOOTER_MAGIC:08X})")
    print(f"    Version: 0x{version:04X}")
    print(f"    Size in Footer: {fw_len} (Actual: {len(firmware_content)})")

    if magic != FOOTER_MAGIC:
        print("ERROR: Invalid Magic Code!")
        sys.exit(1)

    if fw_len != len(firmware_content):
        print("ERROR: Size mismatch!")
        sys.exit(1)

    # 5. Verify Signature
    print("[4] Verifying Signature...")
    
    # Recalculate SHA256 of the raw firmware data
    sha256_hash = hashlib.sha256(firmware_content).digest()
    
    with open(key_path) as f:
        vk = VerifyingKey.from_pem(f.read())
        
    try:
        # Verify using the raw signature bytes
        if vk.verify_digest(signature, sha256_hash, sigdecode=lambda sig, order: (int.from_bytes(sig[:32], 'big'), int.from_bytes(sig[32:], 'big'))):
            print("SUCCESS: Signature Verified! The firmware is authentic.")
            
            # Save the result to check manually
            with open("restored_firmware.bin", "wb") as f:
                f.write(firmware_content)
            print("    Saved 'restored_firmware.bin' for inspection.")
            
    except BadSignatureError:
        print("ERROR: Signature Verification FAILED! File may be tampered.")
        sys.exit(1)

if __name__ == "__main__":
    main()