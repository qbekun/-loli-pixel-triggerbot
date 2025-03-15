import time
import serial
import serial.tools.list_ports
import pyautogui
import numpy as np
import mss
import keyboard
from colorama import Fore, Back, Style, init
from termcolor import cprint
import uuid
import cv2

# 🎯 Configuration
COLOR_TOLERANCE = 20  # Color change tolerance (you can adjust this)
ACTIVATION_KEY = "shift"  # Activation key for the triggerbot
SAMPLING_SIZE = 5  # Number of pixels to sample in a region (more pixels -> more stable result)

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
    cprint("❌ 𝓁𝓸𝓁𝓲! Arduino not found! ❌", "red", attrs=["bold"])
    return None

def get_average_color(x, y, size=SAMPLING_SIZE):
    """Gets the average color of a region of pixels around the given coordinates."""
    with mss.mss() as sct:
        # Create a region around the target pixel (size x size)
        region = {"left": x - size // 2, "top": y - size // 2, "width": size, "height": size}
        img = np.array(sct.grab(region))
        
        # Calculate the average color of the sampled region
        avg_color = np.mean(img[:, :, :3], axis=(0, 1))
        return avg_color

def colors_differ(color1, color2, tolerance):
    """Checks if the colors differ beyond the tolerance."""
    color_diff = np.sum(np.abs(np.array(color1) - np.array(color2)))
    return color_diff > tolerance

def triggerbot():
    """Pixel Triggerbot"""
    arduino = find_arduino()
    if not arduino:
        return
    cprint(f"🎯 𝓁𝓸𝓁𝓲! Press '{ACTIVATION_KEY}' to activate. 𝓁𝓸𝓁𝓲! (UID: {random_uid})", "blue", attrs=["bold"])
    try:
        while True:
            if keyboard.is_pressed(ACTIVATION_KEY):
                # Focus on a specific region of the crosshair (e.g. 5x5 pixel region centered on the crosshair)
                x, y = pyautogui.position()
                initial_color = get_average_color(x, y)
                while keyboard.is_pressed(ACTIVATION_KEY):
                    time.sleep(0.001)
                    current_color = get_average_color(x, y)
                    if colors_differ(initial_color, current_color, COLOR_TOLERANCE):
                        arduino.write(b"loli\n")
                        time.sleep(0.45)
                        initial_color = current_color
            time.sleep(0.1)
    except KeyboardInterrupt:
        cprint("🛑 𝓁𝓸𝓁𝓲! Initiating graceful shutdown...", "yellow", attrs=["bold"])
    finally:
        if arduino:
            arduino.close()
            cprint("Connection to Arduino has been closed.", "red", attrs=["bold"])

if __name__ == "__main__":
    cprint("💻 𝓁𝓸𝓁𝓲! Starting the lolikuza Triggerbot... (lolikuza.ovh)", "green", attrs=["bold"])
    triggerbot()
