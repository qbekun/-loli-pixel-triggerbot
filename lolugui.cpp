#include <windows.h>
#include <wininet.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include <random>
#include <list>
#include <commctrl.h>
#include <memory>
#include <thread>
#include <chrono>
#include "serial/serial.h"  // wjwwood/serial library

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")

// Configuration constants
const int COLOR_TOLERANCE = 30;
const int SAMPLING_SIZE = 5;
const int ACTIVATION_KEY = VK_XBUTTON1;

// GUI Control IDs
#define IDC_START 1001
#define IDC_PORT 1002
#define IDC_STATUS 1003
#define IDC_PORTLIST 1004
#define IDC_TOLERANCE 1005
#define IDC_TOLERANCE_LABEL 1006

// App class forward declaration
class TriggerBotApp;

// Global variables
HINSTANCE hInst;
std::unique_ptr<TriggerBotApp> g_app;
HBITMAP hBackground = nullptr;
ULONG_PTR gdiplusToken;
HFONT hFont = nullptr;
bool isButtonHovered = false;

// Random delay generator
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<double> delay_dist(0.05, 0.15);

// TriggerBot main application class
class TriggerBotApp {
private:
    bool running;
    std::unique_ptr<serial::Serial> serialPort;
    std::thread processingThread;
    int colorTolerance;

public:
    TriggerBotApp() : running(false), colorTolerance(COLOR_TOLERANCE) {}

    ~TriggerBotApp() {
        Stop();
    }

    bool Start(const std::string& portName) {
        try {
            if (running) {
                return true;
            }

            // Configure and open the serial port
            serialPort = std::make_unique<serial::Serial>(
                portName,
                9600,
                serial::Timeout::simpleTimeout(1000),
                serial::eightbits,
                serial::parity_none,
                serial::stopbits_one,
                serial::flowcontrol_none
            );

            if (!serialPort->isOpen()) {
                MessageBoxW(NULL, L"Failed to open serial port", L"Error", MB_OK | MB_ICONERROR);
                return false;
            }

            running = true;

            // Start processing thread
            processingThread = std::thread(&TriggerBotApp::ProcessLoop, this);

            return true;
        }
        catch (const serial::IOException& e) {
            std::string errorMsg = "Serial Error: " + std::string(e.what());
            std::wstring wErrorMsg(errorMsg.begin(), errorMsg.end());
            MessageBoxW(NULL, wErrorMsg.c_str(), L"Error", MB_OK | MB_ICONERROR);
            return false;
        }
        catch (const std::exception& e) {
            std::string errorMsg = "Error: " + std::string(e.what());
            std::wstring wErrorMsg(errorMsg.begin(), errorMsg.end());
            MessageBoxW(NULL, wErrorMsg.c_str(), L"Error", MB_OK | MB_ICONERROR);
            return false;
        }
    }

    void Stop() {
        running = false;

        if (processingThread.joinable()) {
            processingThread.join();
        }

        if (serialPort && serialPort->isOpen()) {
            serialPort->close();
        }
    }

    void SetColorTolerance(int tolerance) {
        colorTolerance = tolerance;
    }

    int GetColorTolerance() const {
        return colorTolerance;
    }

    bool IsRunning() const {
        return running;
    }

private:
    void ProcessLoop() {
        while (running) {
            if (GetAsyncKeyState(ACTIVATION_KEY) & 0x8000) {
                POINT cursor;
                GetCursorPos(&cursor);
                COLORREF initialColor = GetPixelColor(cursor.x, cursor.y);

                while ((GetAsyncKeyState(ACTIVATION_KEY) & 0x8000) && running) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    COLORREF currentColor = GetPixelColor(cursor.x, cursor.y);

                    if (ColorsDiffer(initialColor, currentColor)) {
                        SendTriggerSignal();
                        int delayMs = static_cast<int>(delay_dist(gen) * 1000);
                        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
                        initialColor = currentColor;
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    COLORREF GetPixelColor(int x, int y) {
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

    bool ColorsDiffer(COLORREF c1, COLORREF c2) {
        int r1 = GetRValue(c1), g1 = GetGValue(c1), b1 = GetBValue(c1);
        int r2 = GetRValue(c2), g2 = GetGValue(c2), b2 = GetBValue(c2);
        return (abs(r1 - r2) + abs(g1 - g2) + abs(b1 - b2)) > colorTolerance;
    }

    void SendTriggerSignal() {
        if (serialPort && serialPort->isOpen()) {
            try {
                serialPort->write("loli\n");
            }
            catch (const std::exception& e) {
                // Handle possible errors during write
                std::string errorMsg = "Serial write error: " + std::string(e.what());
                std::wstring wErrorMsg(errorMsg.begin(), errorMsg.end());
                MessageBoxW(NULL, wErrorMsg.c_str(), L"Error", MB_OK | MB_ICONERROR);
            }
        }
    }
};

// Function prototypes
HBITMAP DownloadAndConvertImage(const wchar_t* url);
std::vector<std::string> ListSerialPorts();

// Custom button procedure for hover effect
LRESULT CALLBACK ButtonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    switch (msg) {
    case WM_MOUSEMOVE: {
        if (!isButtonHovered) {
            isButtonHovered = true;
            InvalidateRect(hwnd, NULL, TRUE);
            TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
            TrackMouseEvent(&tme);
        }
        return 0;
    }
    case WM_MOUSELEAVE: {
        isButtonHovered = false;
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rect;
        GetClientRect(hwnd, &rect);

        // Background
        HBRUSH hBrush = CreateSolidBrush(isButtonHovered ? RGB(96, 0, 214) : RGB(214, 0, 171));
        FillRect(hdc, &rect, hBrush);
        DeleteObject(hBrush);

        // Border
        HBRUSH hBorderBrush = CreateSolidBrush(RGB(255, 255, 255));
        FrameRect(hdc, &rect, hBorderBrush);
        DeleteObject(hBorderBrush);

        // Text
        wchar_t text[20];
        GetWindowTextW(hwnd, text, 20);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));
        SelectObject(hdc, hFont);
        DrawTextW(hdc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        EndPaint(hwnd, &ps);
        return 0;
    }
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

// Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hStartButton, hPortCombo, hStatus, hTolerance;
    static HWND hToleranceLabel;

    switch (msg) {
    case WM_CREATE: {
        // Initialize GDI+
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != Gdiplus::Ok) {
            MessageBoxW(hwnd, L"Error initializing GDI+", L"Error", MB_OK | MB_ICONERROR);
        }

        // Download and convert image
        hBackground = DownloadAndConvertImage(L"https://i.imgur.com/DZoQJML.jpeg");
        if (!hBackground) {
            MessageBoxW(hwnd, L"Failed to download or convert image", L"Error", MB_OK | MB_ICONERROR);
        }

        // Create custom font
        hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");

        // Initialize TriggerBot app
        g_app = std::make_unique<TriggerBotApp>();

        // Create controls with improved layout
        // Start button
        hStartButton = CreateWindowW(L"BUTTON", L"Start",
            WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            150, 20, 100, 40, hwnd, (HMENU)IDC_START, hInst, NULL);
        SetWindowSubclass(hStartButton, ButtonProc, 0, 0);
        SendMessageW(hStartButton, WM_SETFONT, (WPARAM)hFont, TRUE);

        // COM port label
        HWND hPortLabel = CreateWindowW(L"STATIC", L"COM Port:",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            50, 70, 80, 20, hwnd, NULL, hInst, NULL);
        SendMessageW(hPortLabel, WM_SETFONT, (WPARAM)hFont, TRUE);

        // COM port combobox
        hPortCombo = CreateWindowW(L"COMBOBOX", NULL,
            WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
            150, 70, 100, 200, hwnd, (HMENU)IDC_PORT, hInst, NULL);
        SendMessageW(hPortCombo, WM_SETFONT, (WPARAM)hFont, TRUE);

        // Tolerance label
        hToleranceLabel = CreateWindowW(L"STATIC", L"Color Tolerance:",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            50, 110, 100, 20, hwnd, (HMENU)IDC_TOLERANCE_LABEL, hInst, NULL);
        SendMessageW(hToleranceLabel, WM_SETFONT, (WPARAM)hFont, TRUE);

        // Tolerance edit box
        hTolerance = CreateWindowW(L"EDIT", NULL,
            WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
            200, 110, 40, 20, hwnd, (HMENU)IDC_TOLERANCE, hInst, NULL);
        SendMessageW(hTolerance, WM_SETFONT, (WPARAM)hFont, TRUE);
        std::wstring tolValue = std::to_wstring(g_app->GetColorTolerance());
        SetWindowTextW(hTolerance, tolValue.c_str());

        // Status label
        hStatus = CreateWindowW(L"STATIC", L"Status: Stopped",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            50, 150, 95, 20, hwnd, (HMENU)IDC_STATUS, hInst, NULL);
        SendMessageW(hStatus, WM_SETFONT, (WPARAM)hFont, TRUE);

        // Populate COM ports
        std::vector<std::string> ports = ListSerialPorts();
        for (const auto& port : ports) {
            std::wstring wport(port.begin(), port.end());
            SendMessageW(hPortCombo, CB_ADDSTRING, 0, (LPARAM)wport.c_str());
        }
        if (!ports.empty()) {
            SendMessageW(hPortCombo, CB_SETCURSEL, 0, 0);
        }

        InvalidateRect(hwnd, NULL, TRUE); // Force redraw
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        if (hBackground) {
            HDC hdcMem = CreateCompatibleDC(hdc);
            SelectObject(hdcMem, hBackground);

            RECT rect;
            GetClientRect(hwnd, &rect);

            BITMAP bm;
            GetObject(hBackground, sizeof(BITMAP), &bm);
            StretchBlt(hdc, 0, 0, rect.right, rect.bottom,
                hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

            DeleteDC(hdcMem);
        }
        else {
            FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1)); // Fallback
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_START) {
            if (!g_app->IsRunning()) {
                // Get selected COM port
                int portIndex = SendMessageW(hPortCombo, CB_GETCURSEL, 0, 0);
                if (portIndex == CB_ERR) {
                    MessageBoxW(hwnd, L"Please select a COM port.", L"Error", MB_OK | MB_ICONWARNING);
                    return 0;
                }

                wchar_t portBuffer[20];
                SendMessageW(hPortCombo, CB_GETLBTEXT, portIndex, (LPARAM)portBuffer);
                std::wstring wPort(portBuffer);
                std::string portStr(wPort.begin(), wPort.end());

                // Get color tolerance
                int tolerance = GetDlgItemInt(hwnd, IDC_TOLERANCE, NULL, FALSE);
                g_app->SetColorTolerance(tolerance);

                // Start the application
                if (g_app->Start(portStr)) {
                    SetWindowTextW(hStartButton, L"Stop");
                    SetWindowTextW(hStatus, L"Status: Running");
                }
            }
            else {
                g_app->Stop();
                SetWindowTextW(hStartButton, L"Start");
                SetWindowTextW(hStatus, L"Status: Stopped");
            }
        }
        return 0;

    case WM_HSCROLL:
        if ((HWND)lParam == hTolerance) {
            int tolerance = SendMessageW(hTolerance, TBM_GETPOS, 0, 0);
            g_app->SetColorTolerance(tolerance);
        }
        return 0;

    case WM_DESTROY:
        g_app->Stop();
        g_app.reset();
        if (hBackground) DeleteObject(hBackground);
        if (hFont) DeleteObject(hFont);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    hInst = hInstance;

    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"lolikuza";
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(L"lolikuza", L"lolikuza.ovh",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 350, 220,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}

HBITMAP DownloadAndConvertImage(const wchar_t* url) {
    HBITMAP hBitmap = nullptr;
    HINTERNET hInternet = InternetOpenW(L"TriggerBot", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        MessageBoxW(NULL, L"Cannot open internet connection", L"Error", MB_OK | MB_ICONERROR);
        return nullptr;
    }

    HINTERNET hUrl = InternetOpenUrlW(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hUrl) {
        MessageBoxW(NULL, L"Cannot open URL", L"Error", MB_OK | MB_ICONERROR);
        InternetCloseHandle(hInternet);
        return nullptr;
    }

    std::vector<BYTE> buffer;
    BYTE temp[1024];
    DWORD bytesRead;
    while (InternetReadFile(hUrl, temp, sizeof(temp), &bytesRead) && bytesRead > 0) {
        buffer.insert(buffer.end(), temp, temp + bytesRead);
    }
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);

    if (buffer.empty()) {
        MessageBoxW(NULL, L"Downloaded empty file", L"Error", MB_OK | MB_ICONERROR);
        return nullptr;
    }

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, buffer.size());
    if (hMem) {
        void* pMem = GlobalLock(hMem);
        memcpy(pMem, buffer.data(), buffer.size());
        GlobalUnlock(hMem);

        IStream* pStream = nullptr;
        if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) == S_OK) {
            Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromStream(pStream);
            if (bitmap) {
                if (bitmap->GetLastStatus() == Gdiplus::Ok) {
                    bitmap->GetHBITMAP(Gdiplus::Color(255, 255, 255), &hBitmap);
                }
                else {
                    MessageBoxW(NULL, L"GDI+ bitmap loading error", L"Error", MB_OK | MB_ICONERROR);
                }
                delete bitmap;
            }
            else {
                MessageBoxW(NULL, L"Cannot create bitmap", L"Error", MB_OK | MB_ICONERROR);
            }
            pStream->Release();
        }
        else {
            MessageBoxW(NULL, L"Cannot create stream", L"Error", MB_OK | MB_ICONERROR);
        }
        GlobalFree(hMem);
    }

    return hBitmap;
}

std::vector<std::string> ListSerialPorts() {
    std::vector<std::string> ports;

    try {
        // Use serial::list_ports from wjwwood/serial to get available ports
        std::vector<serial::PortInfo> portInfoList = serial::list_ports();

        for (const auto& portInfo : portInfoList) {
            ports.push_back(portInfo.port);
        }
    }
    catch (const std::exception& e) {
        std::string errorMsg = "Error listing ports: " + std::string(e.what());
        std::wstring wErrorMsg(errorMsg.begin(), errorMsg.end());
        MessageBoxW(NULL, wErrorMsg.c_str(), L"Error", MB_OK | MB_ICONERROR);

        // Fallback to traditional COM port search if serial::list_ports fails
        for (int i = 1; i <= 20; ++i) {
            std::string port = "COM" + std::to_string(i);
            try {
                serial::Serial testSerial(port, 9600, serial::Timeout::simpleTimeout(50));
                if (testSerial.isOpen()) {
                    ports.push_back(port);
                    testSerial.close();
                }
            }
            catch (...) {
                // Port not available, continue to next
            }
        }
    }

    return ports;
}
