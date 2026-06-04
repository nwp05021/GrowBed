import os
import subprocess
import time

Import("env")


def before_upload(target, source, env):
    configured_port = env.GetProjectOption("upload_port", "")
    if configured_port:
        env.Replace(UPLOAD_PORT=configured_port)
        print("Using platformio.ini upload_port: %s" % configured_port)

    if env.subst("$UPLOAD_PROTOCOL") != "picotool":
        return

    picotool_dir = env.PioPlatform().get_package_dir(
        "tool-picotool-rp2040-earlephilhower"
    ) or ""
    picotool = os.path.join(picotool_dir, "picotool")
    if os.name == "nt" and os.path.exists(picotool + ".exe"):
        picotool += ".exe"

    result = subprocess.run(
        [picotool, "reboot", "-f", "-u"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        check=False,
    )

    if result.returncode == 0:
        print("Forced Pico reboot into BOOTSEL mode with picotool.")
        time.sleep(1.0)
    elif result.stdout.strip():
        print(result.stdout.strip())


env.AddPreAction("upload", before_upload)
