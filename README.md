# Lolikuza Triggerbot

## Description

The **Lolikuza Triggerbot** is a pixel-based triggerbot designed to detect color changes in a specified screen area and trigger actions via an Arduino device when a change is detected. The bot continuously monitors for changes in pixel color and sends a signal to the Arduino device when the difference exceeds a predefined tolerance. It is activated via a hotkey, and you can customize the screen area and color tolerance.

This bot can be useful in a variety of applications where actions are triggered based on visual changes in real-time (e.g., gaming, automation tasks, or interactive projects).

## Features

- **Pixel Color Detection**: Monitors a small region on the screen (around the mouse cursor) and detects color changes.
- **Arduino Integration**: Sends a signal (e.g., "loli") to the connected Arduino when a color change is detected.
- **Hotkey Activation**: Activate the bot by pressing a user-defined hotkey (`x` by default).
- **Unique Session ID**: Each session is assigned a random unique identifier (UID) for tracking and debugging purposes.
- **Graceful Shutdown**: Ensures proper closure of the Arduino connection upon termination.

## Configuration

- **COLOR_TOLERANCE**: Adjusts the sensitivity of the bot to color changes.
- **ACTIVATION_KEY**: The hotkey to activate the bot (default: `x`).

## Installation

### Requirements

- Python 3.x
- Required Python libraries:
  - `serial`
  - `pyautogui`
  - `numpy`
  - `mss`
  - `keyboard`
  - `colorama`
  - `termcolor`
  - `uuid`

You can install the required libraries using pip:

```bash
pip install pyserial pyautogui numpy mss keyboard colorama termcolor
