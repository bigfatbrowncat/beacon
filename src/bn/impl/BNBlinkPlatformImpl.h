#pragma once

#include <bn/glue/my_blink_platform_impl.h>

#include "base/single_thread_task_runner.h"
#include "third_party/blink/public/platform/web_string.h"

namespace beacon::impl {

class BNBlinkPlatformImpl : public beacon::glue::BlinkPlatformImplBase {
 public:
  BNBlinkPlatformImpl(
      scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> io_thread_task_runner)
      : beacon::glue::BlinkPlatformImplBase(main_thread_task_runner,
                              io_thread_task_runner) {}

  blink::WebString DefaultLocale() override;
};

}  // namespace beacon::impl