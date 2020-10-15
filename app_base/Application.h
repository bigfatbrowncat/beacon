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

#ifdef WIN32
struct PlatformData {
    HINSTANCE hInstance;
    std::shared_ptr<MSG> msg;

    PlatformData(HINSTANCE hInstance, std::shared_ptr<MSG> msg);
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
