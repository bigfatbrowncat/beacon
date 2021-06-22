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
#include "app_base/Window.h"

#include <chrono>

class SkCanvas;
namespace blink {
class Element;
}  // namespace blink

namespace beacon::impl {

class BNApp : public app_base::Application, app_base::Window::Layer {
 public:
  BNApp(int argc,
        char** argv,
        const std::shared_ptr<app_base::PlatformData>& platformData);
  ~BNApp() override;

  void onIdle() override;
  void onAttach(app_base::Window* window) override;

  void onBackendCreated() override;
  void onResize(int width, int height) override;
  void onBeginResizing() override;
  void onEndResizing() override;

  bool UpdateViewIfNeededAndBeginFrame();
  void onPaint(SkSurface*) override;

  bool onMouse(const ui::PlatformEvent& platformEvent,
               int x,
               int y,
               skui::InputState,
               skui::ModifierKey) override;
  bool onMouseWheel(const ui::PlatformEvent& platformEvent,
                    float delta,
                    skui::ModifierKey) override;
  bool onKey(const ui::PlatformEvent& platformEvent,
             uint64_t,
             skui::InputState,
             skui::ModifierKey) override;
  bool onChar(const ui::PlatformEvent& platformEvent,
              SkUnichar c,
              skui::ModifierKey modifiers) override;

  blink::Document& GetDocument();
  void UpdateBackend(bool forceFallback);

 private:
  void updateTitle();
  void Paint(SkCanvas* canvas);
  void UpdatePlatformFontsAndColors();

  app_base::PlatformFont defaultUIFont;

  app_base::Window* fWindow;
  app_base::Window::BackendType fBackendType;
  std::shared_ptr<app_base::PlatformData> platformData;
  bool resizing = false;
  std::shared_ptr<discardable_memory::DiscardableSharedMemoryManager>
      discardableSharedMemoryManager;

  std::chrono::steady_clock::time_point lastBackendInitFailedAttempt =
      std::chrono::steady_clock::now();

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