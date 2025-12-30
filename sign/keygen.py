import os
from ecdsa import SigningKey, NIST256p

# Files to generate
PRIVATE_KEY_FILE = "private.pem"
PUBLIC_KEY_FILE = "public.pem"
AES_KEY_FILE = "secret.key"

def generate_keys():
    print("--- Generating New Security Keys ---")

    # 1. Generate ECDSA P-256 Key Pair
    sk = SigningKey.generate(curve=NIST256p)
    vk = sk.verifying_key
    
    # Save to files
    with open(PRIVATE_KEY_FILE, "wb") as f:
        f.write(sk.to_pem())
    with open(PUBLIC_KEY_FILE, "wb") as f:
        f.write(vk.to_pem())

    # 2. Generate AES-128 Key (16 Bytes)
    aes_key = os.urandom(16)
    with open(AES_KEY_FILE, "wb") as f:
        f.write(aes_key)

    print(f"[OK] Saved {PRIVATE_KEY_FILE} and {PUBLIC_KEY_FILE}")
    print(f"[OK] Saved {AES_KEY_FILE}")
    print("-" * 60)
    
    # 3. Print Content for keys.c
    print(">>> COPY THE TEXT BELOW INTO 'BOOTLOADER1/Core/Src/keys.c' <<<")
    print("-" * 60)
    
    # Format AES Key
    aes_str = ", ".join([f"0x{b:02x}" for b in aes_key])
    
    # Format ECDSA Public Key (X + Y)
    vk_bytes = vk.to_string()
    ecdsa_str = ", ".join([f"0x{b:02x}" for b in vk_bytes])

    c_code = f"""#include "keys.h"

/* AES-128 Secret Key (16 bytes) */
const uint8_t AES_SECRET_KEY[16] = {{ {aes_str} }};

/* ECDSA P-256 Public Key (64 bytes: X + Y) */
const uint8_t ECDSA_public_key_xy[64] = {{ {ecdsa_str} }};
"""
    print(c_code)
    print("-" * 60)

if __name__ == "__main__":
    generate_keys()