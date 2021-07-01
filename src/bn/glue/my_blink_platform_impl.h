// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "base/compiler_specific.h"
#include "base/single_thread_task_runner.h"
#include "base/timer/timer.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
//#include "components/webcrypto/webcrypto_impl.h"
#include "content/common/content_export.h"
#include "third_party/blink/public/common/input/web_gesture_device.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_url_error.h"
#include "third_party/blink/public/public_buildflags.h"
#include "ui/base/layout.h"

#include "app_base/Window.h"

#if defined(OS_MACOSX)
#import <CoreText/CoreText.h>
#include "third_party/blink/public/platform/mac/web_sandbox_support.h"
#elif defined(OS_WIN)
// Nothing here
#else
#include "third_party/blink/public/platform/linux/web_sandbox_support.h"
#endif

// class WebCryptoImpl;

namespace beacon::glue {

#if defined(OS_MACOSX)
class MyWebSandboxSupport : public blink::WebSandboxSupport {
 private:
  app_base::Window* window;

 public:
  MyWebSandboxSupport(app_base::Window* window) : window(window) {}
  virtual ~MyWebSandboxSupport() override {}
  // Given an input font - |srcFont| [which can't be loaded due to sandbox
  // restrictions]. Return a font belonging to an equivalent font file
  // that can be used to access the font and a unique identifier corresponding
  // to the on-disk font file.
  //
  // If this function succeeds, the caller assumes ownership of the |out|
  // parameter and must call CGFontRelease() to unload it when done.
  //
  // Returns: true on success, false on error.
  bool LoadFont(CTFontRef src_font,
                CGFontRef* out,
                uint32_t* font_id) override {
    return false;
  }

  // Returns the system's preferred value for a named color.
  SkColor GetSystemColor(blink::MacSystemColorID colorId) override {
    app_base::PlatformColors pc = window->GetPlatformColors();
    switch (colorId) {
      case blink::MacSystemColorID::kSelectedText:
        return pc.selectionTextColorActive;
      case blink::MacSystemColorID::kSelectedTextBackground:
        return pc.selectionBackgroundColorActive;
      case blink::MacSystemColorID::kSecondarySelectedControl:
        return pc.selectionBackgroundColorInactive;

      default:
        return SkColorSetRGB(128, 0, 0);
    }
  }
};
#elif defined(OS_WIN)
// Nothing here
#else  // This is for linux
// TODO Implement this class for Linux
#endif

class BlinkPlatformImplBase : public blink::Platform {
 public:
  explicit BlinkPlatformImplBase(
      scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> io_thread_task_runner);
  ~BlinkPlatformImplBase() override;

  void TakeExampleWindow(app_base::Window* window);

  // Platform methods (partial implementation):
  blink::WebThemeEngine* ThemeEngine() override;
  // bool IsURLSupportedForAppCache(const blink::WebURL& url) override;

  size_t MaxDecodedImageBytes() override;
  bool IsLowEndDevice() override;
  void RecordAction(const blink::UserMetricsAction&) override;

  blink::WebData GetDataResource(int resource_id,
                                 ui::ScaleFactor scale_factor) override;
  blink::WebData UncompressDataResource(int resource_id) override;
  blink::WebString QueryLocalizedString(int resource_id) override;
  blink::WebString QueryLocalizedString(int resource_id,
                                        const blink::WebString& value) override;
  blink::WebString QueryLocalizedString(
      int resource_id,
      const blink::WebString& value1,
      const blink::WebString& value2) override;
  void SuddenTerminationChanged(bool enabled) override {}
  bool AllowScriptExtensionForServiceWorker(
      const blink::WebSecurityOrigin& script_origin) override;
  blink::WebCrypto* Crypto() override;
  /*blink::ThreadSafeBrowserInterfaceBrokerProxy* GetBrowserInterfaceBroker()
      override;*/

  scoped_refptr<base::SingleThreadTaskRunner> GetIOTaskRunner() const override;
  std::unique_ptr<NestedMessageLoopRunner> CreateNestedMessageLoopRunner()
      const override;

  blink::WebSandboxSupport* GetSandboxSupport() override {
#if defined(OS_MACOSX)
    return myWebSandboxSupport.get();
#elif defined(OS_WIN)
    // Nothing here
    return nullptr;
#else  // This is for linux
    // TODO Implement this class for Linux
    return nullptr;
#endif
  }

 private:
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> io_thread_task_runner_;
  // const scoped_refptr<blink::ThreadSafeBrowserInterfaceBrokerProxy>
  // browser_interface_broker_proxy_;
  std::unique_ptr<blink::WebThemeEngine> native_theme_engine_;
  //  webcrypto::WebCryptoImpl web_crypto_;
  base::string16 GetLocalizedString(int message_id) { return base::string16(); }

#if defined(OS_MACOSX)
  std::unique_ptr<blink::WebSandboxSupport> myWebSandboxSupport;
#elif WIN32
  // Nothing here
#else  // This is for linux
  // TODO Implement this class for Linux
  std::unique_ptr<blink::WebSandboxSupport> myWebSandboxSupport;
#endif
};

}  // namespace beacon::glue
