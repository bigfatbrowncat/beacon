#include "BNBlinkPlatformImpl.h"

namespace beacon::impl {

blink::WebString BNBlinkPlatformImpl::DefaultLocale() {
  return blink::WebString("en-US");
}

}  // namespace beacon::impl
