#ifndef LgApp_DEFINED
#define LgApp_DEFINED

#include <chrono>

#include "base/command_line.h"
#include "base/at_exit.h"
#include "base/task/sequence_manager/thread_controller_with_message_pump_impl.h"

#include "mojo/public/cpp/bindings/binder_map.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/scheduler/web_thread_scheduler.h"
#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/platform/heap/member.h"
#include "components/discardable_memory/service/discardable_shared_memory_manager.h"
#include "third_party/blink/renderer/platform/wtf/cross_thread_functional.h"

#include "app_base/Application.h"
#include "app_base/Window.h"
#include "hell/my_frame_test_helpers.h"

class SkCanvas;
namespace blink {
class Element;
}  // namespace blink

class LgApp : public sk_app::Application, sk_app::Window::Layer {
 public:
  LgApp(int argc, char** argv,
        const std::shared_ptr<sk_app::PlatformData>& platformData);
  ~LgApp() override;

  void onIdle() override;
  void onAttach(sk_app::Window* window) override;

  void onBackendCreated() override;
  void onResize(int width, int height) override;
  void onBeginResizing() override;
  void onEndResizing() override;
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
  void PrintSinglePage(SkCanvas* canvas, int width, int height);
  void UpdateBackend();

 private:
  void updateTitle();

  sk_app::PlatformFont defaultUIFont;

  sk_app::Window* fWindow;
  sk_app::Window::BackendType fBackendType;
  std::shared_ptr<sk_app::PlatformData> platformData;
  bool resizing = false;
  std::shared_ptr<discardable_memory::DiscardableSharedMemoryManager>
      discardableSharedMemoryManager;    

  base::CommandLine::StringType htmlFilename;
  std::string htmlContents;
  std::chrono::steady_clock::time_point htmlContentsUpdateTime =
      std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point lastGLInitAttempt =
      std::chrono::steady_clock::now();

  std::shared_ptr<blink::Platform> platform;

  std::shared_ptr<SDK::Backend> backend;
  std::shared_ptr<blink::my_frame_test_helpers::WebViewHelper> webViewHelper;
  std::shared_ptr<blink::my_frame_test_helpers::TestWebFrameClient> wfc;
  std::shared_ptr<blink::my_frame_test_helpers::TestWebViewClient> wvc;
  std::shared_ptr<blink::my_frame_test_helpers::TestWebWidgetClient> wwc;

  std::shared_ptr<blink::scheduler::WebThreadScheduler> my_web_thread_sched;
  scoped_refptr<base::SingleThreadTaskRunner> mainTaskRunner, composeTaskRunner;

  std::shared_ptr<
      base::sequence_manager::internal::ThreadControllerWithMessagePumpImpl>
      threadController;

  blink::WebViewImpl* webView = nullptr;
  blink::GraphicsLayer* root_graphics_layer = nullptr;

  WTF::Vector<std::shared_ptr<blink::WebInputEvent>> collectedInputEvents;

  blink::HeapHashMap<String, blink::Member<blink::Element>>
      linked_destinations_;

  std::shared_ptr<base::AtExitManager> exit_manager;
  std::shared_ptr<mojo::BinderMap> binder_map;
};

#endif  /* LgApp_DEFINED */
