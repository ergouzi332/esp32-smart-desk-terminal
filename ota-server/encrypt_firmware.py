#!/usr/bin/env python3
"""
ESP32 OTA ??????
? firmware.bin ?? AES-128-CBC ??? firmware.enc
??? firmware.sha256?????????

?????
  1. ??? ESP32 ????? .pio/build/.../firmware.bin ??
  2. ??????python encrypt_firmware.py
  3. ??? serve.bat ????

???pip install pycryptodome
"""

import os
import sys
import hashlib

try:
    from Crypto.Cipher import AES
except ImportError:
    print("[FAIL] ?? pycryptodome ?????: pip install pycryptodome")
    sys.exit(1)

# ===================== AES-128-CBC ???? =====================
# ??? ESP32 ? ota.h ?? OTA_AES_KEY_DATA / OTA_AES_IV_DATA ??
AES_KEY = bytes([
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10
])
AES_IV = bytes([
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
])
# ===============================================================

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))


def pkcs7_pad(data: bytes, block_size: int = 16) -> bytes:
    pad_len = block_size - (len(data) % block_size)
    return data + bytes([pad_len] * pad_len)


def encrypt_file(bin_path: str, enc_path: str, sha_path: str):
    if not os.path.exists(bin_path):
        print(f"[FAIL] ???????: {bin_path}")
        return False

    with open(bin_path, "rb") as f:
        firmware = f.read()

    print(f"[INFO] ????: {len(firmware)} ??")

    sha256_hash = hashlib.sha256(firmware).hexdigest()
    print(f"[INFO] SHA256: {sha256_hash}")

    padded = pkcs7_pad(firmware)
    print(f"[INFO] ???: {len(padded)} ?? (?? {len(padded) - len(firmware)} ??)")

    cipher = AES.new(AES_KEY, AES.MODE_CBC, AES_IV)
    encrypted = cipher.encrypt(padded)

    with open(enc_path, "wb") as f:
        f.write(encrypted)
    print(f"[OK] ????: {enc_path} ({len(encrypted)} ??)")

    with open(sha_path, "w") as f:
        f.write(sha256_hash + "\n")
    print(f"[OK] SHA256 ???: {sha_path}")

    return True


if __name__ == "__main__":
    bin_path  = os.path.join(SCRIPT_DIR, "firmware.bin")
    enc_path  = os.path.join(SCRIPT_DIR, "firmware.enc")
    sha_path  = os.path.join(SCRIPT_DIR, "firmware.sha256")

    if len(sys.argv) >= 2:
        bin_path = sys.argv[1]
    if len(sys.argv) >= 3:
        enc_path = sys.argv[2]
    if len(sys.argv) >= 4:
        sha_path = sys.argv[3]

    success = encrypt_file(bin_path, enc_path, sha_path)
    sys.exit(0 if success else 1)
