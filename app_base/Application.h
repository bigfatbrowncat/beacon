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
#endif

namespace sk_app {

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
};
#endif



class Application {
public:
    static Application* Create(int argc, char** argv, const std::shared_ptr<PlatformData>& platformData);

    virtual ~Application() {}

    virtual void onIdle() = 0;
};

}   // namespace sk_app

#endif
