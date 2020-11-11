/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "include/gpu/vk/GrVkVulkan.h"

#include "app_base/win/Window_win.h"

#include <tchar.h>
#include <windows.h>
#include <windowsx.h>
#include <WinUser.h>

#include "src/core/SkUtils.h"
#include "app_base/WindowContext.h"
#include "app_base/win/WindowContextFactory_win.h"
#include "tools/skui/ModifierKey.h"

#ifdef SK_VULKAN
#include "app_base/VulkanWindowContext.h"
#endif

namespace sk_app {

static int gWindowX = CW_USEDEFAULT;
static int gWindowY = 0;
static int gWindowWidth = CW_USEDEFAULT;
static int gWindowHeight = 0;

Window* Window::CreateNativeWindow(const std::shared_ptr<PlatformData>& platformData) {
    HINSTANCE hInstance = platformData->hInstance;

    Window_win* window = new Window_win(platformData);
    if (!window->init()) {
        delete window;
        return nullptr;
    }

    return window;
}

void Window_win::closeWindow() {
    RECT r;
    if (GetWindowRect(fHWnd, &r)) {
        gWindowX = r.left;
        gWindowY = r.top;
        gWindowWidth = r.right - r.left;
        gWindowHeight = r.bottom - r.top;
    }
    DestroyWindow(fHWnd);
}

Window_win::~Window_win() {
    this->closeWindow();
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

bool Window_win::init() {
    HINSTANCE fHInstance = platformData->hInstance ? platformData->hInstance : GetModuleHandle(nullptr);

    // The main window class name
    static const TCHAR gSZWindowClass[] = _T("SkiaApp");

    static WNDCLASSEX wcex;
    static bool wcexInit = false;
    if (!wcexInit) {
        wcex.cbSize = sizeof(WNDCLASSEX);

        wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
        wcex.lpfnWndProc = WndProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = fHInstance;
        wcex.hIcon = LoadIcon(fHInstance, (LPCTSTR)IDI_WINLOGO);
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = NULL;
        wcex.lpszMenuName = nullptr;
        wcex.lpszClassName = gSZWindowClass;
        wcex.hIconSm = LoadIcon(fHInstance, (LPCTSTR)IDI_WINLOGO);

        if (!RegisterClassEx(&wcex)) {
            return false;
        }
        wcexInit = true;
    }

    /*
     if (fullscreen)
     {
         DEVMODE dmScreenSettings;
         // If full screen set the screen to maximum size of the users desktop and 32bit.
         memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
         dmScreenSettings.dmSize = sizeof(dmScreenSettings);
         dmScreenSettings.dmPelsWidth = (unsigned long)width;
         dmScreenSettings.dmPelsHeight = (unsigned long)height;
         dmScreenSettings.dmBitsPerPel = 32;
         dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

         // Change the display settings to full screen.
         ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);

         // Set the position of the window to the top left corner.
         posX = posY = 0;
     }
     */
    //   gIsFullscreen = fullscreen;

    fHWnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW | WS_EX_COMPOSITED, gSZWindowClass, nullptr,
        WS_OVERLAPPEDWINDOW, gWindowX, gWindowY, gWindowWidth,
        gWindowHeight,
                           nullptr, nullptr, fHInstance, nullptr);
    if (!fHWnd) {
        return false;
    }

    SetWindowLongPtr(fHWnd, GWLP_USERDATA, (LONG_PTR)this);
    RegisterTouchWindow(fHWnd, 0);

    return true;
}

static skui::ModifierKey get_modifiers(UINT message, WPARAM wParam, LPARAM lParam) {
    skui::ModifierKey modifiers = skui::ModifierKey::kNone;

    switch (message) {
        case WM_UNICHAR:
        case WM_CHAR:
            if (0 == (lParam & (1 << 30))) {
                modifiers |= skui::ModifierKey::kFirstPress;
            }
            if (lParam & (1 << 29)) {
                modifiers |= skui::ModifierKey::kOption;
            }
            break;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if (0 == (lParam & (1 << 30))) {
                modifiers |= skui::ModifierKey::kFirstPress;
            }
            if (lParam & (1 << 29)) {
                modifiers |= skui::ModifierKey::kOption;
            }
            break;

        case WM_KEYUP:
        case WM_SYSKEYUP:
            if (lParam & (1 << 29)) {
                modifiers |= skui::ModifierKey::kOption;
            }
            break;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MOUSEMOVE:
        case WM_MOUSEWHEEL:
        case WM_LBUTTONDBLCLK:
            if (wParam & MK_CONTROL) {
                modifiers |= skui::ModifierKey::kControl;
            }
            if (wParam & MK_SHIFT) {
                modifiers |= skui::ModifierKey::kShift;
            }
            break;
    }

    return modifiers;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    PAINTSTRUCT ps;
    HDC hdc;

    Window_win* window = (Window_win*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (window == nullptr) {
        // The window is not prepared yet
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    MSG& msg = *(window->getPlatformData().msg);

    bool eventHandled = false;

    switch (message) {
        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            window->onPaint();
            EndPaint(hWnd, &ps);
            eventHandled = true;
            break;

        case WM_CLOSE:
            PostQuitMessage(0);
            eventHandled = true;
            break;

        case WM_ACTIVATE:
            // disable/enable rendering here, depending on wParam != WA_INACTIVE
            break;

        case WM_ENTERSIZEMOVE:
            window->onBeginResizing();
            eventHandled = true;
            break;

        case WM_EXITSIZEMOVE:
            window->onEndResizing();
            eventHandled = true;
            break;

        case WM_SIZE:
            window->onResize(LOWORD(lParam), HIWORD(lParam));
            eventHandled = true;
            break;

        case WM_UNICHAR:
            eventHandled = window->onChar(msg, (SkUnichar)wParam,
                                            get_modifiers(message, wParam, lParam));
            break;

        case WM_CHAR: {
            const uint16_t* c = reinterpret_cast<uint16_t*>(&wParam);
            eventHandled = window->onChar(msg, SkUTF16_NextUnichar(&c),
                                            get_modifiers(message, wParam, lParam));
        } break;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            eventHandled = window->onKey(msg, (uint64_t)wParam, 
                                            skui::InputState::kDown,
                                            get_modifiers(message, wParam, lParam));
            break;

        case WM_KEYUP:
        case WM_SYSKEYUP:
            eventHandled = window->onKey(msg, (uint64_t)wParam, 
                                            skui::InputState::kUp,
                                            get_modifiers(message, wParam, lParam));
            break;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK: {
          SetCapture(hWnd);

          int xPos = GET_X_LPARAM(lParam);
          int yPos = GET_Y_LPARAM(lParam);

          skui::InputState istate = ((wParam & MK_LBUTTON) != 0)
                                        ? skui::InputState::kDown
                                        : skui::InputState::kUp;

          eventHandled = window->onMouse(
              msg, xPos, yPos, istate, get_modifiers(message, wParam, lParam));
        } break;

        case WM_LBUTTONUP: {
          ReleaseCapture();

            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);

            skui::InputState istate = ((wParam & MK_LBUTTON) != 0) ? skui::InputState::kDown
                                                                    : skui::InputState::kUp;
            eventHandled = window->onMouse(msg, xPos, yPos, istate,
                                            get_modifiers(message, wParam, lParam));
        } break;

        case WM_MOUSEMOVE: {
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);

            //if (!gIsFullscreen)
            //{
            //    RECT rc = { 0, 0, 640, 480 };
            //    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
            //    xPos -= rc.left;
            //    yPos -= rc.top;
            //}

            eventHandled =
                    window->onMouse(msg, xPos, yPos, skui::InputState::kMove,
                                            get_modifiers(message, wParam, lParam));
        } break;

        case WM_MOUSEWHEEL:
            eventHandled = window->onMouseWheel(
                    msg, GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f,
                                            get_modifiers(message, wParam, lParam));
            break;

        case WM_TOUCH: {
            uint16_t numInputs = LOWORD(wParam);
            std::unique_ptr<TOUCHINPUT[]> inputs(new TOUCHINPUT[numInputs]);
            if (GetTouchInputInfo((HTOUCHINPUT)lParam, numInputs, inputs.get(),
                                  sizeof(TOUCHINPUT))) {
                POINT topLeft = {0, 0};
                ClientToScreen(hWnd, &topLeft);
                for (uint16_t i = 0; i < numInputs; ++i) {
                    TOUCHINPUT ti = inputs[i];
                    skui::InputState state;
                    if (ti.dwFlags & TOUCHEVENTF_DOWN) {
                        state = skui::InputState::kDown;
                    } else if (ti.dwFlags & TOUCHEVENTF_MOVE) {
                        state = skui::InputState::kMove;
                    } else if (ti.dwFlags & TOUCHEVENTF_UP) {
                        state = skui::InputState::kUp;
                    } else {
                        continue;
                    }
                    // TOUCHINPUT coordinates are in 100ths of pixels
                    // Adjust for that, and make them window relative
                    LONG tx = (ti.x / 100) - topLeft.x;
                    LONG ty = (ti.y / 100) - topLeft.y;
                    eventHandled =
                            window->onTouch(msg, ti.dwID, state, tx, ty) ||
                            eventHandled;
                }
            }
        } break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return eventHandled ? 0 : 1;
}

void Window_win::setTitle(const char* title) {
    SetWindowTextA(fHWnd, title);
}

void Window_win::show() {
    ShowWindow(fHWnd, SW_SHOW);
}


bool Window_win::attach(BackendType attachType) {
    fBackend = attachType;

    switch (attachType) {
        case kNativeGL_BackendType:
            fWindowContext = window_context_factory::MakeGLForWin(fHWnd, fRequestedDisplayParams);
            break;
#if SK_ANGLE
        case kANGLE_BackendType:
            fWindowContext =
                    window_context_factory::MakeANGLEForWin(fHWnd, fRequestedDisplayParams);
            break;
#endif
#ifdef SK_DAWN
        case kDawn_BackendType:
            fWindowContext =
                    window_context_factory::MakeDawnD3D12ForWin(fHWnd, fRequestedDisplayParams);
            break;
#endif
        case kRaster_BackendType:
            fWindowContext =
                    window_context_factory::MakeRasterForWin(fHWnd, fRequestedDisplayParams);
            break;
#ifdef SK_VULKAN
        case kVulkan_BackendType:
            fWindowContext =
                    window_context_factory::MakeVulkanForWin(fHWnd, fRequestedDisplayParams);
            break;
#endif
    }
    this->onBackendCreated();

    return (SkToBool(fWindowContext));
}

void Window_win::onInval() {
    InvalidateRect(fHWnd, nullptr, false);
}

void Window_win::setRequestedDisplayParams(const DisplayParams& params, bool allowReattach) {
    // GL on Windows doesn't let us change MSAA after the window is created
    if (params.fMSAASampleCount != this->getRequestedDisplayParams().fMSAASampleCount
            && allowReattach) {
        // Need to change these early, so attach() creates the window context correctly
        fRequestedDisplayParams = params;

        fWindowContext = nullptr;
        this->closeWindow();
        this->init();
        this->attach(fBackend);
    }

    INHERITED::setRequestedDisplayParams(params, allowReattach);
}

float Window_win::getScale() {
	// Windows UI should be scaled proportionally based on the screen DPI
    return (float)::GetDpiForWindow(this->fHWnd) / 96;
}

static std::string utf8_encode(const std::wstring& wstr) {
  if (wstr.empty())
    return std::string();
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(),
                                        NULL, 0, NULL, NULL);
  std::string strTo(size_needed, 0);
  WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0],
                      size_needed, NULL, NULL);
  return strTo;
}

bool Window_win::GetDefaultUIFont(PlatformFont& result) {
    // Getting the default UI font
    NONCLIENTMETRICS ncm;
    ncm.cbSize = sizeof(NONCLIENTMETRICS);
    bool res = SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 
                                    sizeof(NONCLIENTMETRICS), &ncm, 0);
    
    if (res) {
      std::wstring wsFaceName(ncm.lfMenuFont.lfFaceName);
      result.typeface = utf8_encode(wsFaceName);
      result.heightPt = GetFontSize(ncm.lfMenuFont);
    }

    return res;
}

SkColor Window_win::GetFocusRingColor() const {
  DWORD color = ::GetSysColor(COLOR_MENUHILIGHT /*COLOR_HIGHLIGHT*/);
    uint8_t r = GetRValue(color);
    uint8_t g = GetRValue(color);
    uint8_t b = GetBValue(color);
    return SkColorSetRGB(r, g, b);
}


int Window_win::GetFontSize(const LOGFONT& font) {
  int nFontSize = 0;

  HDC hDC = ::GetWindowDC(this->fHWnd);

  if (font.lfHeight < 0) {
    nFontSize = -::MulDiv(font.lfHeight, 72, ::GetDeviceCaps(hDC, LOGPIXELSY));
  } else {
    TEXTMETRIC tm;
    ::ZeroMemory(&tm, sizeof(TEXTMETRIC));
    ::GetTextMetrics(hDC, &tm);

    nFontSize = ::MulDiv(font.lfHeight - tm.tmInternalLeading, 72,
                         ::GetDeviceCaps(hDC, LOGPIXELSY));
  }

  ::ReleaseDC(this->fHWnd, hDC);

  return nFontSize;
}

}   // namespace sk_app
