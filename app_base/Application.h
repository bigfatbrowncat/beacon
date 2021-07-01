/*
* Copyright 2016 Google Inc.
*
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*/

#ifndef Application_DEFINED
#define Application_DEFINED

#include <memory>

#ifdef WIN32
#include <windows.h>
#elif defined(__linux__)
//#include <X11/Xlib.h>
typedef struct _XDisplay Display;
#endif

namespace app_base {

#if defined(WIN32)

struct PlatformData {
    HINSTANCE hInstance;
    std::shared_ptr<MSG> msg;

    PlatformData(HINSTANCE hInstance, std::shared_ptr<MSG> msg);
};

#elif defined(__APPLE__)

struct PlatformData {
};

#elif defined(__linux__)
struct PlatformData {
    Display* display;

    PlatformData(Display* display);
};
#endif



class Application {
 private:
  bool quitPending = false;   
 public:
    static Application* Create(int argc, char** argv, const std::shared_ptr<PlatformData>& platformData);

    virtual ~Application() {}

    virtual void onIdle() = 0;

    // When the user has selected the app in the Taskbar 
    // (or in Dock on Mac) and has pressed "close". 
    // 
    // Here the implementation should check for unsaved
    // files and ask the user questions
    virtual void onUserQuit() = 0;

    // This command sets a graceful quit flag signalling 
    // that the application is ready to stop.
    // The main application message loop ends by this flag
    // and the application object is destroyed.
    // 
    // This operation is irreversable and should be called after
    // the user accepted all the "Are you sure?" questions
    virtual void Quit() { quitPending = true; }
    virtual bool isQuitPending() { return quitPending; }

    virtual std::shared_ptr<app_base::PlatformData> getPlatformData() = 0;
};

}   // namespace app_base

#endif
