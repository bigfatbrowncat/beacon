#include "Application.h"

#ifdef WIN32
#include <windows.h>
#endif

namespace sk_app {

#ifdef WIN32
    PlatformData::PlatformData(HINSTANCE hInstance, std::shared_ptr<MSG> msg)
        : hInstance(hInstance), msg(msg) {}
#endif

}   // namespace sk_app
