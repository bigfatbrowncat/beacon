#pragma once

#include "hell/my_blink_platform_impl.h"

#include "third_party/blink/public/platform/web_string.h"

class LgBlinkPlatformImpl : public content::BlinkPlatformImpl {
public:
  LgBlinkPlatformImpl(
                             scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner,
                    scoped_refptr<base::SingleThreadTaskRunner> io_thread_task_runner, sk_app::Window* window) :
  BlinkPlatformImpl(main_thread_task_runner, io_thread_task_runner, window) { }

  
  blink::WebString DefaultLocale() override { return blink::WebString("en-US"); }
};
