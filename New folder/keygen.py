from ecdsa import SigningKey, NIST256p
import os

# 1. Generate Private Key (ECDSA P-256)
sk = SigningKey.generate(curve=NIST256p)
with open("private.pem", "wb") as f:
    f.write(sk.to_pem())
print("Generated private.pem")

# 2. Generate Public Key
vk = sk.verifying_key
with open("public.pem", "wb") as f:
    f.write(vk.to_pem())
print("Generated public.pem")

# 3. Generate AES Secret Key (Random 16 bytes)
aes_key = os.urandom(16)
with open("secret.key", "wb") as f:
    f.write(aes_key)

print("Generated secret.key")
print(f"AES Key (Hex): {aes_key.hex().upper()}")