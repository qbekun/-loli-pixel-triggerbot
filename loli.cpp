#include <iostream>
#include <windows.h>
#include <thread>
#include <random>
#include <fstream>
#include <vector>
#include <string>
#include <list>

// 🎯 Configuration
const int COLOR_TOLERANCE = 20;  // Color change tolerance
const int SAMPLING_SIZE = 5;      // Number of color samples in the area
const char* ACTIVATION_KEY = "VK_SHIFT";  // Key that activates the triggerbot
const char* ARDUINO_PORT = "COM8"; // Arduino port (change according to your setup)

// Random delays
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<double> delay_dist(0.05, 0.15); // Delay range 50-150 ms

// Function for a random delay
void random_delay() {
    std::this_thread::sleep_for(std::chrono::duration<double>(delay_dist(gen)));
}

// Gets the color of a pixel at a given screen point using BitBlt
COLORREF get_pixel_color(int x, int y) {
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hbmScreen = CreateCompatibleBitmap(hdcScreen, SAMPLING_SIZE, SAMPLING_SIZE);
    SelectObject(hdcMem, hbmScreen);
    BitBlt(hdcMem, 0, 0, SAMPLING_SIZE, SAMPLING_SIZE, hdcScreen, x - SAMPLING_SIZE / 2, y - SAMPLING_SIZE / 2, SRCCOPY);

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = SAMPLING_SIZE;
    bmi.bmiHeader.biHeight = -SAMPLING_SIZE;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    std::vector<COLORREF> pixels(SAMPLING_SIZE * SAMPLING_SIZE);
    GetDIBits(hdcMem, hbmScreen, 0, SAMPLING_SIZE, pixels.data(), &bmi, DIB_RGB_COLORS);

    DeleteObject(hbmScreen);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);

    // Calculate the average color from the sampled area
    int r = 0, g = 0, b = 0;
    for (const auto& pixel : pixels) {
        r += GetRValue(pixel);
        g += GetGValue(pixel);
        b += GetBValue(pixel);
    }
    r /= pixels.size();
    g /= pixels.size();
    b /= pixels.size();

    return RGB(r, g, b);
}

// Compares colors with tolerance
bool colors_differ(COLORREF c1, COLORREF c2) {
    int r1 = GetRValue(c1), g1 = GetGValue(c1), b1 = GetBValue(c1);
    int r2 = GetRValue(c2), g2 = GetGValue(c2), b2 = GetBValue(c2);
    return (abs(r1 - r2) + abs(g1 - g2) + abs(b1 - b2)) > COLOR_TOLERANCE;
}

// Sends a command to Arduino
void send_to_arduino(HANDLE hSerial) {
    const char* message = "loli\n";
    DWORD bytes_written;
    WriteFile(hSerial, message, strlen(message), &bytes_written, NULL);
}

// Lists available COM ports
std::list<std::string> list_com_ports() {
    std::list<std::string> ports;
    for (int i = 1; i <= 255; ++i) {
        std::string port = "COM" + std::to_string(i);
        HANDLE hSerial = CreateFileA(port.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
        if (hSerial != INVALID_HANDLE_VALUE) {
            ports.push_back(port);
            CloseHandle(hSerial);
        }
    }
    return ports;
}

// Initializes connection to Arduino
HANDLE connect_to_arduino(const std::string& port) {
    std::wstring wide_port(port.begin(), port.end());

    HANDLE hSerial = CreateFileW(wide_port.c_str(), GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cerr << "❌ Arduino not found on port " << port << "!" << std::endl;
        return nullptr;
    }
    std::cout << "✅ Connected to Arduino on port " << port << "!" << std::endl;
    return hSerial;
}

// Main triggerbot function
void triggerbot() {
    // List and select COM port
    std::list<std::string> available_ports = list_com_ports();
    if (available_ports.empty()) {
        std::cerr << "❌ No COM ports found!" << std::endl;
        return;
    }

    std::cout << "Available COM ports:\n";
    int index = 1;
    for (const auto& port : available_ports) {
        std::cout << index++ << ". " << port << std::endl;
    }

    int choice;
    std::cout << "Select the COM port for Arduino: ";
    std::cin >> choice;
    if (choice < 1 || choice > available_ports.size()) {
        std::cerr << "❌ Invalid choice!" << std::endl;
        return;
    }

    auto selected_port = std::next(available_ports.begin(), choice - 1);
    HANDLE arduino = connect_to_arduino(*selected_port);
    if (!arduino) return;

    std::cout << "🎯 Press SHIFT to activate the triggerbot!" << std::endl;

    while (true) {
        if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
            POINT cursor;
            GetCursorPos(&cursor);
            COLORREF initial_color = get_pixel_color(cursor.x, cursor.y);
            while (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                COLORREF current_color = get_pixel_color(cursor.x, cursor.y);
                if (colors_differ(initial_color, current_color)) {
                    send_to_arduino(arduino);
                    random_delay();
                    initial_color = current_color;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    CloseHandle(arduino);
}

int main() {
    std::cout << "💻 Starting Lolikuza Triggerbot..." << std::endl;
    triggerbot();
    return 0;
}
