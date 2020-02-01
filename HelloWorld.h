/*
 * Copyright 2017 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef HelloWorld_DEFINED
#define HelloWorld_DEFINED

#include "base/run_loop.h"
#include "mojo/public/cpp/bindings/binder_map.h"
#include "my_frame_test_helpers.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/scheduler/web_thread_scheduler.h"
#include "third_party/blink/renderer/platform/heap/member.h"
#include "third_party/blink/renderer/core/dom/document.h"
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

  void onBackendCreated() override;
  void onPaint(SkSurface*) override;
  bool onChar(SkUnichar c, skui::ModifierKey modifiers) override;

  blink::Document& GetDocument();
  void PrintSinglePage(SkCanvas* canvas);
  void SetBodyInnerHTML(String body_content);

  void OutputLinkedDestinations(blink::GraphicsContext& context,
                                            const blink::IntRect& page_rect);
  void CollectLinkedDestinations(blink::Node* node);
  bool Capture(/*cc::PaintCanvas* */ SkCanvas* canvas, blink::FloatSize size);

 private:
  void updateTitle();

  sk_app::Window* fWindow;
  sk_app::Window::BackendType fBackendType;

  SkScalar fRotationAngle;

  std::shared_ptr<blink::Platform> platform;

  std::shared_ptr<blink::my_frame_test_helpers::WebViewHelper> webViewHelper;
  std::shared_ptr<blink::my_frame_test_helpers::TestWebFrameClient> wfc;
  std::shared_ptr<blink::my_frame_test_helpers::TestWebViewClient> wvc;
  std::shared_ptr<blink::my_frame_test_helpers::TestWebWidgetClient> wwc;

  
  std::shared_ptr<blink::scheduler::WebThreadScheduler> my_web_thread_sched;
  scoped_refptr<base::SingleThreadTaskRunner> mainTaskRunner;

  blink::WebViewImpl* webView;

  blink::HeapHashMap<String, blink::Member<blink::Element>> linked_destinations_;

  std::shared_ptr<base::RunLoop> run_loop;
  std::shared_ptr<mojo::BinderMap> binder_map;
  //    std::shared_ptr<blink::scheduler::WebThreadScheduler>
  //    my_web_thread_sched;
};

#endif
