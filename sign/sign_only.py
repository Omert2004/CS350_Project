import sys
import struct
import hashlib
from ecdsa import SigningKey, NIST256p

# Configuration (Must match Bootloader)
FOOTER_MAGIC = 0x454E4421
FIRMWARE_VERSION = 0x0100

def main():
    if len(sys.argv) != 4:
        print("Usage: python sign_only.py <input_bin> <output_signed_bin> <private_key.pem>")
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]
    key_path = sys.argv[3]

    print(f"[1] Reading: {input_path}")
    with open(input_path, "rb") as f:
        firmware_bin = f.read()
    
    firmware_len = len(firmware_bin)

    # 2. Sign
    print("[2] Signing...")
    sha256 = hashlib.sha256(firmware_bin).digest()
    
    with open(key_path) as f:
        sk = SigningKey.from_pem(f.read())
    
    # Deterministic signature (same as generate_update.py)
    signature = sk.sign_digest_deterministic(sha256, sigencode=lambda r, s, order: r.to_bytes(32, 'big') + s.to_bytes(32, 'big'))
    
    # 3. Append Footer
    # Footer Format: [Signature (64)] + [Version (4)] + [Size (4)] + [Magic (4)]
    footer = struct.pack('<64sIII', signature, FIRMWARE_VERSION, firmware_len, FOOTER_MAGIC)
    
    final_image = firmware_bin + footer

    print(f"[3] Saving to: {output_path}")
    print(f"    Original Size: {firmware_len} bytes")
    print(f"    Signed Size:   {len(final_image)} bytes")
    
    with open(output_path, "wb") as f:
        f.write(final_image)

if __name__ == "__main__":
    main()