import os
from ecdsa import SigningKey, NIST256p

# Output filenames
PRIVATE_KEY_FILE = "private.pem"
PUBLIC_KEY_FILE = "public.pem"
AES_KEY_FILE = "secret.key"

def generate_keys():
    print("--- Generating New Security Keys ---")

    # 1. Generate ECDSA P-256 Key Pair (for Signing)
    print(f"Generating ECDSA (NIST256p) key pair...")
    sk = SigningKey.generate(curve=NIST256p)
    vk = sk.verifying_key

    # Save Private Key
    with open(PRIVATE_KEY_FILE, "wb") as f:
        f.write(sk.to_pem())
    print(f"  [+] Saved {PRIVATE_KEY_FILE}")

    # Save Public Key
    with open(PUBLIC_KEY_FILE, "wb") as f:
        f.write(vk.to_pem())
    print(f"  [+] Saved {PUBLIC_KEY_FILE}")

    # 2. Generate AES-128 Key (for Encryption)
    # Generates 16 bytes (128 bits) of random data
    print(f"Generating AES-128 secret key...")
    aes_key = os.urandom(16) 

    with open(AES_KEY_FILE, "wb") as f:
        f.write(aes_key)
    print(f"  [+] Saved {AES_KEY_FILE} (16 bytes)")

    print("\nKeys generated successfully!")

if __name__ == "__main__":
    generate_keys()