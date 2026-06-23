import os, subprocess
Import("env")

def erase_otadata(source, target, env):
    port = env.subst("$UPLOAD_PORT")
    if not port:
        print("[OTA] No upload port found")
        return

    # Find esptool.py in PlatformIO packages
    home = env.subst("$PROJECT_PACKAGES_DIR")
    esptool = os.path.join(home, "tool-esptoolpy", "esptool.py")
    if not os.path.exists(esptool):
        fallback = os.path.join(os.path.dirname(os.path.dirname(home)), "packages", "tool-esptoolpy", "esptool.py")
        if os.path.exists(fallback):
            esptool = fallback

    if not os.path.exists(esptool):
        print(f"[OTA] esptool not found at {esptool}")
        return

    cmd = [env.subst("$PYTHONEXE"), esptool, "--chip", "esp32s3",
           "--port", port, "--baud", "921600",
           "erase_region", "0xD000", "0x2000"]
    print("[OTA] Erasing otadata...")
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode == 0:
        print("[OTA] otadata erased. Device will boot from ota_0.")
    else:
        print(f"[OTA] Failed: {result.stderr}")

env.AddPostAction("upload", erase_otadata)
