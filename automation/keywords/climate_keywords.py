import subprocess
import re


# ---------------------------------------------------
# Common helper
# ---------------------------------------------------

def run_adb(cmd):
    result = subprocess.run(
        cmd,
        shell=True,
        text=True,
        capture_output=True
    )

    print(result.stdout)
    print(result.stderr)

    # adb command itself failed
    if result.returncode != 0:
        raise Exception(result.stderr)

    # no emulator/device connected
    if "no devices/emulators found" in result.stderr:
        raise Exception(result.stderr)

    # property set/get failed
    if "Cannot set a property" in result.stdout:
        raise Exception(result.stdout)

    if "Cannot get property value" in result.stdout:
        raise Exception(result.stdout)

    return result.stdout


# ---------------------------------------------------
# HVAC POWER
# Property ID = 354419984
# ---------------------------------------------------

def set_hvac_power(value):

    value = 1 if value else 0

    cmd = (
        "adb shell cmd car_service "
        f"set-property-value 354419984 1 {value}"
    )

    run_adb(cmd)


# ---------------------------------------------------
# AC ON
# Property ID = 354419973
# ---------------------------------------------------

def set_ac(value):

    value = 1 if value else 0

    cmd = (
        "adb shell cmd car_service "
        f"set-property-value 354419973 1 {value}"
    )

    run_adb(cmd)


# ---------------------------------------------------
# FAN SPEED
# Property ID = 356517120
# ---------------------------------------------------

def set_fan_speed(speed):

    cmd = (
        "adb shell cmd car_service "
        f"set-property-value 356517120 1 {speed}"
    )

    run_adb(cmd)


def get_fan_speed():

    cmd = (
        "adb shell cmd car_service "
        "get-property-value 356517120 1"
    )

    output = run_adb(cmd)

    print("RAW OUTPUT:")
    print(output)

    # extract numeric value
    match = re.search(r"Value:\s*(\d+)", output)

    if not match:
        raise Exception("Fan speed value not found")

    return int(match.group(1))


# ---------------------------------------------------
# TEMPERATURE
# Property ID = 358614275
# ---------------------------------------------------

def set_temperature(temp):

    cmd = (
        "adb shell cmd car_service "
        f"set-property-value 358614275 1 {temp}"
    )

    run_adb(cmd)


# ---------------------------------------------------
# DUAL MODE
# Property ID = 354419977
# ---------------------------------------------------

def set_dual_mode(value):

    value = 1 if value else 0

    cmd = (
        "adb shell cmd car_service "
        f"set-property-value 354419977 1 {value}"
    )

    run_adb(cmd)
