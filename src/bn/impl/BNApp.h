#pragma once

#include <bn/glue/my_frame_test_helpers.h>
#include <bn/impl/BNBlinkPlatformImpl.h>
#include <bn/sdk/BNBackendController.h>

#include "base/command_line.h"
#include "base/at_exit.h"
#include "base/task/sequence_manager/thread_controller_with_message_pump_impl.h"
#include "ui/events/platform_event.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/scheduler/web_thread_scheduler.h"
#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/platform/heap/member.h"
#include "third_party/blink/renderer/platform/wtf/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"
#include "mojo/public/cpp/bindings/binder_map.h"
#include "components/discardable_memory/service/discardable_shared_memory_manager.h"

#include "app_base/Application.h"

#include "BNLayer.h"

#include <chrono>
#include <list>

class SkCanvas;

namespace blink {
class Element;
}  // namespace blink

namespace beacon {
namespace impl {

class BNViewLayerWindow;

class BNApp : public app_base::Application {
 public:
  BNApp(int argc,
        char** argv,
        const std::shared_ptr<app_base::PlatformData>& platformData);
  ~BNApp() override;

  void onIdle() override;
  void onUserQuit() override;

  std::shared_ptr<app_base::PlatformData> getPlatformData() override {
    return platformData;
  }
  std::shared_ptr<blink::scheduler::WebThreadScheduler> getThreadScheduler() {
    return my_web_thread_sched;
  }
  std::shared_ptr<mojo::BinderMap> getBinderMap() {
    return binder_map;
  }
  std::shared_ptr<beacon::impl::BNBlinkPlatformImpl> getPlatform() {
    return platform;
  }
  /* std::shared_ptr<BNViewLayerWindow> getViewLayerWindow() {
    return viewLayerWindow;
  }*/
  /* const app_base::PlatformData& getPlatformData() override {
    return *platformData.get();
  } */

 private:
  std::shared_ptr<app_base::PlatformData> platformData;
  std::shared_ptr<discardable_memory::DiscardableSharedMemoryManager>
      discardableSharedMemoryManager;

  std::shared_ptr<base::AtExitManager> exit_manager;
  std::shared_ptr<mojo::BinderMap> binder_map;
  std::shared_ptr<blink::scheduler::WebThreadScheduler> my_web_thread_sched;

  std::shared_ptr<
      base::sequence_manager::internal::ThreadControllerWithMessagePumpImpl>
      threadController;
  std::shared_ptr<beacon::impl::BNBlinkPlatformImpl> platform;

  std::list<std::shared_ptr<BNViewLayerWindow>> viewLayerWindows;
  std::chrono::steady_clock::time_point lastDoFrame =
      std::chrono::steady_clock::now();
};

class BNViewLayerWindow : public BNLayer {
 public:
  BNViewLayerWindow(BNApp& app);

  ~BNViewLayerWindow() override;

  void onAttach(app_base::Window* window) override;

  void onResize(int width, int height) override;

  bool onEvent(const ui::PlatformEvent& platformEvent) override;

  std::string getTitle() override;
  void Paint(SkCanvas* canvas) override;

  blink::Document& GetDocument();
  BNApp& getApplication() { return app; }
  
  bool onUserCloseKeepWindow() override {
    std::cout << "BNViewLayerWindow::onUserCloseKeepWindow()" << std::endl;
    // Don't keep the windows by default
    return false;
  }
  
 private:
  bool UpdateViewIfNeededAndBeginFrame() override;

  BNApp& app;

  std::shared_ptr<beacon::sdk::Backend> backend;
  std::shared_ptr<beacon::glue::WebViewHelper> webViewHelper;
  std::shared_ptr<beacon::glue::TestWebFrameClient> wfc;
  std::shared_ptr<beacon::glue::TestWebViewClient> wvc;
  std::shared_ptr<beacon::glue::TestWebWidgetClient> wwc;

  scoped_refptr<base::SingleThreadTaskRunner> composeTaskRunner;

  blink::WebViewImpl* webView = nullptr;
  // blink::GraphicsLayer* root_graphics_layer = nullptr;

  WTF::Vector<std::shared_ptr<blink::WebInputEvent>> collectedInputEvents;

  blink::HeapHashMap<String, blink::Member<blink::Element>>
      linked_destinations_;
};

}  // namespace impl
}  // namespace beacon
