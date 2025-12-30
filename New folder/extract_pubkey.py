from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.backends import default_backend

# Load public key
with open("public.pem", "rb") as f:
    public_key = serialization.load_pem_public_key(f.read(), backend=default_backend())

# Get public numbers (X and Y)
numbers = public_key.public_numbers()
x = numbers.x.to_bytes(32, byteorder='big')
y = numbers.y.to_bytes(32, byteorder='big')

# Print as C array
print("const uint8_t public_key_x[32] = {")
print(', '.join(f'0x{b:02X}' for b in x) + " };")

print("\nconst uint8_t public_key_y[32] = {")
print(', '.join(f'0x{b:02X}' for b in y) + " };")
