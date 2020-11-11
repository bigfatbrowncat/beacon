#ifndef LgBlinkPlatformImpl_DEFINED
#define LgBlinkPlatformImpl_DEFINED

#include "hell/my_blink_platform_impl.h"

#include "third_party/blink/public/platform/web_string.h"

class LgBlinkPlatformImpl : public content::BlinkPlatformImpl {
  blink::WebString DefaultLocale() override { return blink::WebString("en-US"); }
};

#endif /* LgBlinkPlatformImpl_DEFINED */
