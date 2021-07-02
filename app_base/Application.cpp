#include "Application.h"

#ifdef WIN32
#include <windows.h>
#endif

namespace app_base {

#ifdef WIN32
    PlatformData::PlatformData(HINSTANCE hInstance, std::shared_ptr<MSG> msg)
        : hInstance(hInstance), msg(msg) {}
#elif (__linux__)
    PlatformData::PlatformData(Display* display)
        : display(display) {}
#endif

}   // namespace app_base
