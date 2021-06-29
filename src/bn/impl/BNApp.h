#pragma once

#include <bn/glue/my_frame_test_helpers.h>
#include <bn/sdk/BNBackendController.h>

#include "base/command_line.h"
#include "base/at_exit.h"
#include "base/task/sequence_manager/thread_controller_with_message_pump_impl.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/scheduler/web_thread_scheduler.h"
#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/platform/heap/member.h"
#include "third_party/blink/renderer/platform/wtf/cross_thread_functional.h"
#include "mojo/public/cpp/bindings/binder_map.h"
#include "components/discardable_memory/service/discardable_shared_memory_manager.h"

#include "app_base/Application.h"

#include "BNLayer.h"



class SkCanvas;
namespace blink {
class Element;
}  // namespace blink

namespace beacon::impl {

class BNApp : public app_base::Application, BNLayer {
 public:
  BNApp(int argc,
        char** argv,
        const std::shared_ptr<app_base::PlatformData>& platformData);
  virtual ~BNApp() override;

  void onIdle() override;
  void onAttach(app_base::Window* window) override;

  void onResize(int width, int height) override;

  bool onEvent(const ui::PlatformEvent& platformEvent) override;


  std::string getTitle() override;
  void Paint(SkCanvas* canvas) override;


  blink::Document& GetDocument();
  
 private:
  bool UpdateViewIfNeededAndBeginFrame() override;


  std::shared_ptr<app_base::PlatformData> platformData;
  std::shared_ptr<discardable_memory::DiscardableSharedMemoryManager>
      discardableSharedMemoryManager;

  std::shared_ptr<blink::Platform> platform;

  std::shared_ptr<beacon::sdk::Backend> backend;
  std::shared_ptr<beacon::glue::WebViewHelper> webViewHelper;
  std::shared_ptr<beacon::glue::TestWebFrameClient> wfc;
  std::shared_ptr<beacon::glue::TestWebViewClient> wvc;
  std::shared_ptr<beacon::glue::TestWebWidgetClient> wwc;

  std::shared_ptr<blink::scheduler::WebThreadScheduler> my_web_thread_sched;
  scoped_refptr<base::SingleThreadTaskRunner> mainTaskRunner, composeTaskRunner;

  std::shared_ptr<
      base::sequence_manager::internal::ThreadControllerWithMessagePumpImpl>
      threadController;

  blink::WebViewImpl* webView = nullptr;
  // blink::GraphicsLayer* root_graphics_layer = nullptr;

  WTF::Vector<std::shared_ptr<blink::WebInputEvent>> collectedInputEvents;

  blink::HeapHashMap<String, blink::Member<blink::Element>>
      linked_destinations_;

  std::shared_ptr<base::AtExitManager> exit_manager;
  std::shared_ptr<mojo::BinderMap> binder_map;

  std::chrono::time_point<std::chrono::high_resolution_clock> paintTime;
};

}  // namespace beacon::impl
