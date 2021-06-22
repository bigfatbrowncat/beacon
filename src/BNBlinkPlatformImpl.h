#pragma once

#include "hell/my_blink_platform_impl.h"

#include "third_party/blink/public/platform/web_string.h"

class BNBlinkPlatformImpl : public BlinkPlatformImplBase {
public:
  BNBlinkPlatformImpl(
                             scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner,
                    scoped_refptr<base::SingleThreadTaskRunner> io_thread_task_runner, app_base::Window* window) :
  BlinkPlatformImplBase(main_thread_task_runner, io_thread_task_runner, window) { }

  
  blink::WebString DefaultLocale() override { return blink::WebString("en-US"); }
};
