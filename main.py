import time
import serial
import serial.tools.list_ports
import numpy as np
import mss
import ctypes
import random
from colorama import Fore, Back, Style, init
from termcolor import cprint
import uuid
import cv2

# 🎯 Configuration
COLOR_TOLERANCE = 20  # Color change tolerance
ACTIVATION_KEY = 160  # Virtual-Key Code for Left Shift (VK_LSHIFT)
SAMPLING_SIZE = 5  # Number of pixels to sample in a region

# Initialize Colorama
init(autoreset=True)

# Generate a random UID
random_uid = str(uuid.uuid4())[:8] 

def find_arduino():
    """Automatically detects Arduino through the COM port."""
    cprint("🔍 Searching for Arduino...", "yellow", attrs=["bold"])
    ports = list(serial.tools.list_ports.comports())
    for port in ports:
        if "Arduino" in port.description or "Leonardo" in port.description:
            cprint(f"💖 ㋛ Found Arduino on port {port.device} ㋛ 💖", "green", attrs=["bold"])
            return serial.Serial(port.device, 115200, timeout=1)
    cprint("❌ 𝓁𝓸𝓁𝒾! Arduino not found! ❌", "red", attrs=["bold"])
    return None

def get_cursor_position():
    """Gets the current cursor position without using pyautogui."""
    pt = ctypes.windll.user32.GetCursorPos
    cursor = ctypes.wintypes.POINT()
    pt(ctypes.byref(cursor))
    return cursor.x, cursor.y

def get_average_color(x, y, size=SAMPLING_SIZE):
    """Gets the average color of a region of pixels around the given coordinates."""
    with mss.mss() as sct:
        region = {"left": x - size // 2, "top": y - size // 2, "width": size, "height": size}
        img = np.array(sct.grab(region))
        avg_color = np.mean(img[:, :, :3], axis=(0, 1))
        return avg_color

def colors_differ(color1, color2, tolerance):
    """Checks if the colors differ beyond the tolerance."""
    color_diff = np.sum(np.abs(np.array(color1) - np.array(color2)))
    return color_diff > tolerance

def is_key_pressed(key_code):
    """Check if a key is currently pressed (Windows API)."""
    return ctypes.windll.user32.GetAsyncKeyState(key_code) & 0x8000

def triggerbot():
    """Pixel Triggerbot with randomness and Windows API for input handling."""
    arduino = find_arduino()
    if not arduino:
        return
    cprint(f"🎯 𝓁𝓸𝓁𝒾! Press Left Shift to activate. (UID: {random_uid})", "blue", attrs=["bold"])
    try:
        while True:
            if is_key_pressed(ACTIVATION_KEY):
                x, y = get_cursor_position()
                initial_color = get_average_color(x, y)
                while is_key_pressed(ACTIVATION_KEY):
                    time.sleep(random.uniform(0.01, 0.03))
                    current_color = get_average_color(x, y)
                    if colors_differ(initial_color, current_color, COLOR_TOLERANCE):
                        arduino.write(b"loli\n")
                        time.sleep(random.uniform(0.3, 0.6))
                        initial_color = current_color
            time.sleep(random.uniform(0.05, 0.15))
    except KeyboardInterrupt:
        cprint("🛑 𝓁𝓸𝓁𝒾! Initiating graceful shutdown...", "yellow", attrs=["bold"])
    finally:
        if arduino:
            arduino.close()
            cprint("Connection to Arduino has been closed.", "red", attrs=["bold"])

if __name__ == "__main__":
    cprint("💻 𝓁𝓸𝓁𝒾! Starting the lolikuza Triggerbot... (lolikuza.ovh)", "green", attrs=["bold"])
    triggerbot()
