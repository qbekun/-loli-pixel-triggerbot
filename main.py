import time
import serial
import serial.tools.list_ports
import pyautogui
import numpy as np
import mss
import keyboard
from colorama import Fore, Back, Style, init
from termcolor import cprint

# 🎯 Configuration
COLOR_TOLERANCE = 20  # Color change tolerance (you can adjust this)
ACTIVATION_KEY = "x"  # Activation key for the triggerbot

# Initialize Colorama
init(autoreset=True)

def find_arduino():
    """Automatically detects Arduino through the COM port."""
    cprint("🔍 Searching for Arduino...", "yellow", attrs=["bold"])
    ports = list(serial.tools.list_ports.comports())
    for port in ports:
        if "Arduino" in port.description or "Leonardo" in port.description:
            cprint(f"💖 ㋛ Found Arduino on port {port.device} ㋛ 💖", "green", attrs=["bold"])
            return serial.Serial(port.device, 115200, timeout=1)
    cprint("❌ 𝓛𝓸𝓵𝓲! Arduino not found! ❌", "red", attrs=["bold"])
    return None

def get_pixel_color(x, y):
    """Gets the pixel color."""
    with mss.mss() as sct:
        region = {"left": x, "top": y, "width": 1, "height": 1}
        img = np.array(sct.grab(region))
        return img[0, 0, :3]

def colors_differ(color1, color2, tolerance):
    """Checks if the colors differ beyond the tolerance."""
    color_diff = np.sum(np.abs(np.array(color1) - np.array(color2)))
    return color_diff > tolerance

def triggerbot():
    """Pixel Triggerbot"""
    arduino = find_arduino()
    if not arduino:
        return

    cprint(f"🎯 𝓛𝓸𝓵𝓲! Press '{ACTIVATION_KEY}' to activate. 𝓛𝓸𝓵𝓲!", "blue", attrs=["bold"])

    try:
        while True:
            if keyboard.is_pressed(ACTIVATION_KEY):
                # Focus on a specific region of the crosshair (e.g. 1x1 pixel in the center)
                x, y = pyautogui.position()
                initial_color = get_pixel_color(x, y)

                while keyboard.is_pressed(ACTIVATION_KEY):
                    time.sleep(0.001)
                    current_color = get_pixel_color(x, y)

                    if colors_differ(initial_color, current_color, COLOR_TOLERANCE):
                        arduino.write(b"loli\n")
                        time.sleep(0.45)
                        initial_color = current_color

            time.sleep(0.1)

    except KeyboardInterrupt:
        cprint("🛑 𝓛𝓸𝓵𝓲! Initiating graceful shutdown...", "yellow", attrs=["bold"])
    finally:
        if arduino:
            arduino.close()
            cprint("Connection to Arduino has been closed.", "red", attrs=["bold"])

if __name__ == "__main__":
    triggerbot()
