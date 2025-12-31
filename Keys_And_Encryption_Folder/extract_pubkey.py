from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.backends import default_backend
import os

# --- 1. Extract ECDSA Public Key (X and Y) ---
try:
    with open("public.pem", "rb") as f:
        public_key = serialization.load_pem_public_key(f.read(), backend=default_backend())

    # Get public numbers (X and Y)
    numbers = public_key.public_numbers()
    x = numbers.x.to_bytes(32, byteorder='big')
    y = numbers.y.to_bytes(32, byteorder='big')

    print("/* ECDSA Public Key (from public.pem) */")
    print("const uint8_t ECDSA_public_key_x[32] = {")
    print("    " + ', '.join(f'0x{b:02X}' for b in x))
    print("};")

    print("\nconst uint8_t ECDSA_public_key_y[32] = {")
    print("    " + ', '.join(f'0x{b:02X}' for b in y))
    print("};")

except FileNotFoundError:
    print("Error: 'public.pem' not found. Run keygen.py first.")

# --- 2. Extract AES Secret Key ---
try:
    with open("secret.key", "rb") as f:
        aes_key = f.read()

    print("\n/* AES Secret Key (from secret.key) */")
    print(f"const uint8_t AES_SECRET_KEY[{len(aes_key)}] = {{")
    print("    " + ', '.join(f'0x{b:02X}' for b in aes_key))
    print("};")

except FileNotFoundError:
    print("\nError: 'secret.key' not found. Run keygen.py first.")