# Lolikuza Triggerbot

🎯 A simple triggerbot that activates when the SHIFT key is pressed, detects color changes in a specific area of the screen, and sends a command to an Arduino device when a change is detected. The bot uses random delays to simulate human behavior.

## Features

- Detects color changes in a specified screen area.
- Activates when the SHIFT key is pressed.
- Sends a command to an Arduino device when a color change is detected.
- Random delay to mimic human-like actions.
- Configurable color tolerance and sampling area size.

## Requirements

- Windows OS (since it uses Windows-specific APIs)
- Arduino device (connected to the specified COM port)
- C++ compiler (MSVC or MinGW)
- Arduino IDE (for uploading the sketch to your Arduino)

## Installation

1. Clone the repository:

   ```bash
   git clone https://github.com/qbekun/-loli-pixel-triggerbot.git
   cd lolikuza-triggerbot
