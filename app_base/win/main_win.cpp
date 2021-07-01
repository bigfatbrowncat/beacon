/*
* Copyright 2016 Google Inc.
*
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*/

#include <windows.h>
#include <tchar.h>

#include "include/core/SkTypes.h"
#include "app_base/Application.h"
#include "app_base/win/Window_win.h"
#include "tools/timer/Timer.h"

using app_base::Application;

static char* tchar_to_utf8(const TCHAR* str) {
#ifdef _UNICODE
    int size = WideCharToMultiByte(CP_UTF8, 0, str, wcslen(str), NULL, 0, NULL, NULL);
    char* str8 = (char*)sk_malloc_throw(size + 1);
    WideCharToMultiByte(CP_UTF8, 0, str, wcslen(str), str8, size, NULL, NULL);
    str8[size] = '\0';
    return str8;
#else
    return _strdup(str);
#endif
}

// This file can work with GUI or CONSOLE subsystem types since we define _tWinMain and main().

static int main_common(HINSTANCE hInstance, int show, int argc, char**argv);

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine,
                       int nCmdShow) {

    // convert from lpCmdLine to argc, argv.
    char* argv[4096];
    int argc = 0;
    TCHAR exename[1024], *next;
    int exenameLen = GetModuleFileName(NULL, exename, SK_ARRAY_COUNT(exename));
    // we're ignoring the possibility that the exe name exceeds the exename buffer
    (void)exenameLen;
    argv[argc++] = tchar_to_utf8(exename);
    TCHAR* arg = _tcstok_s(lpCmdLine, _T(" "), &next);
    while (arg != NULL) {
        argv[argc++] = tchar_to_utf8(arg);
        arg = _tcstok_s(NULL, _T(" "), &next);
    }
    int result = main_common(hInstance, nCmdShow, argc, argv);
    for (int i = 0; i < argc; ++i) {
        sk_free(argv[i]);
    }
    return result;
}

int main(int argc, char**argv) {
    return main_common(GetModuleHandle(NULL), SW_SHOW, argc, argv);
}

static int main_common(HINSTANCE hInstance, int show, int argc, char**argv) {
    SetProcessDPIAware();

    std::shared_ptr<MSG> msg = std::make_shared<MSG>();
    memset(msg.get(), 0, sizeof(msg));

    std::shared_ptr<app_base::PlatformData> platformData =
            std::make_shared<app_base::PlatformData>(hInstance, msg);

    Application* app = Application::Create(argc, argv, platformData);

    bool idled = false;

    // Main message loop
    while (!app->isQuitPending()) {
        if (PeekMessage(msg.get(), nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(msg.get());
          if (WM_QUIT == msg->message) {
              app->onUserQuit();
          } else

            if (WM_PAINT == msg->message) {
                // Ensure that call onIdle at least once per WM_PAINT, or mouse events can
                // overwhelm the message processing loop, and prevent animation from running.
                if (!idled) {
                    app->onIdle();
                }
                idled = false;
            }
            DispatchMessage(msg.get());
        } else {
            app->onIdle();
            idled = true;
        }
    }

    delete app;

    return (int)msg->wParam;
}
