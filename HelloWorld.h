/*
 * Copyright 2017 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef HelloWorld_DEFINED
#define HelloWorld_DEFINED

#include <chrono>

#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/task/sequence_manager/thread_controller_with_message_pump_impl.h"

#include "mojo/public/cpp/bindings/binder_map.h"
#include "my_frame_test_helpers.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/scheduler/web_thread_scheduler.h"
#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/platform/heap/member.h"
#include "tools/sk_app/Application.h"
#include "tools/sk_app/Window.h"



class SkCanvas;
namespace blink {
class Element;
}  // namespace blink

class HelloWorld : public sk_app::Application, sk_app::Window::Layer {
 public:
  HelloWorld(int argc, char** argv, void* platformData);
  ~HelloWorld() override;

  void onIdle() override;
  void onAttach(sk_app::Window* window) override;

  void onBackendCreated() override;
  void onPaint(SkSurface*) override;
  bool onChar(SkUnichar c, skui::ModifierKey modifiers) override;

  blink::Document& GetDocument();
  void PrintSinglePage(SkCanvas* canvas, int width, int height);
  void SetBodyInnerHTML(String body_content);
  void UpdateContents();

  void OutputLinkedDestinations(blink::GraphicsContext& context,
                                const blink::IntRect& page_rect);
  void CollectLinkedDestinations(blink::Node* node);
  bool Capture(/*cc::PaintCanvas* */ SkCanvas* canvas, blink::FloatSize size);

 private:
  void updateTitle();

  sk_app::Window* fWindow;
  sk_app::Window::BackendType fBackendType;

  base::CommandLine::StringType htmlFilename;
  std::string htmlContents;
  std::chrono::steady_clock::time_point htmlContentsUpdateTime =
      std::chrono::steady_clock::now();

  SkScalar fRotationAngle;

  std::shared_ptr<blink::Platform> platform;

  std::shared_ptr<blink::my_frame_test_helpers::WebViewHelper> webViewHelper;
  std::shared_ptr<blink::my_frame_test_helpers::TestWebFrameClient> wfc;
  std::shared_ptr<blink::my_frame_test_helpers::TestWebViewClient> wvc;
  std::shared_ptr<blink::my_frame_test_helpers::TestWebWidgetClient> wwc;

  std::shared_ptr<blink::scheduler::WebThreadScheduler> my_web_thread_sched;
  scoped_refptr<base::SingleThreadTaskRunner> mainTaskRunner, composeTaskRunner;

  std::shared_ptr<
      base::sequence_manager::internal::ThreadControllerWithMessagePumpImpl>
      threadController;

  blink::WebViewImpl* webView;

  blink::HeapHashMap<String, blink::Member<blink::Element>>
      linked_destinations_;

  std::shared_ptr<base::RunLoop> run_loop;
  std::shared_ptr<mojo::BinderMap> binder_map;
  //    std::shared_ptr<blink::scheduler::WebThreadScheduler>
  //    my_web_thread_sched;
};

#endif
