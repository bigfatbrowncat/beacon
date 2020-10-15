/*
 * Copyright 2017 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "HelloWorld.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkGraphics.h"
#include "include/core/SkSurface.h"
#include "include/effects/SkGradientShader.h"
#include "third_party/blink/renderer/core/events/before_print_event.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/html/html_element.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/page/print_context.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/paint/paint_layer_painter.h"
#include "third_party/blink/renderer/core/scroll/scrollbar_theme.h"
#include "third_party/blink/renderer/platform/language.h"

#include "base/message_loop/message_pump.h"
#include "base/run_loop.h"
#include "base/time/default_tick_clock.h"
#include "mojo/core/embedder/embedder.h"
#include "third_party/blink/public/platform/scheduler/web_thread_scheduler.h"
#include "third_party/blink/public/platform/web_font_render_style.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/renderer/core/frame/visual_viewport.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"
#include "third_party/blink/renderer/core/html/html_head_element.h"
#include "third_party/blink/renderer/core/html/html_collection.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_record_builder.h"
#include "third_party/blink/renderer/platform/heap/heap.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/main_thread_scheduler_impl.h"
#include "third_party/blink/renderer/platform/scheduler/public/dummy_schedulers.h"

#include "base/memory/discardable_memory_allocator.h"
#include "base/task/post_task.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/thread_pool/thread_pool_impl.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "cc/paint/skia_paint_canvas.h"
#include "my_frame_test_helpers.h"
#include "third_party/blink/renderer/platform/geometry/int_point.h"
#include "third_party/blink/renderer/platform/wtf/text/text_stream.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/third_party/icu/SkLoadICU.h"

#include "ui/events/blink/blink_event_util.h"


using namespace sk_app;
using namespace blink;

// A simple SingleThreadTaskRunner that just queues undelayed tasks (and ignores
// delayed tasks). Tasks can then be processed one by one by ProcessTask() which
// will return true if it processed a task and false otherwise.
class SimpleSingleThreadTaskRunner : public base::SingleThreadTaskRunner {
 public:
  SimpleSingleThreadTaskRunner() = default;

  bool PostDelayedTask(const base::Location& from_here,
                       base::OnceClosure task,
                       base::TimeDelta delay) override {
    if (delay > base::TimeDelta())
      return false;
    base::AutoLock auto_lock(tasks_lock_);
    pending_tasks_.push(std::move(task));
    return true;
  }

  bool PostNonNestableDelayedTask(const base::Location& from_here,
                                  base::OnceClosure task,
                                  base::TimeDelta delay) override {
    return PostDelayedTask(from_here, std::move(task), delay);
  }

  bool RunsTasksInCurrentSequence() const override {
    return origin_thread_checker_.CalledOnValidThread();
  }

  bool ProcessSingleTask() {
    base::OnceClosure task;
    {
      base::AutoLock auto_lock(tasks_lock_);
      if (pending_tasks_.empty())
        return false;
      task = std::move(pending_tasks_.front());
      pending_tasks_.pop();
    }
    // It's important to Run() after pop() and outside the lock as |task| may
    // run a nested loop which will re-enter ProcessSingleTask().
    std::move(task).Run();
    return true;
  }

 private:
  ~SimpleSingleThreadTaskRunner() override = default;

  base::Lock tasks_lock_;
  base::queue<base::OnceClosure> pending_tasks_;

  // RunLoop relies on RunsTasksInCurrentSequence() signal. Use a
  // ThreadCheckerImpl to be able to reliably provide that signal even in
  // non-dcheck builds.
  base::ThreadCheckerImpl origin_thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(SimpleSingleThreadTaskRunner);
};

// The basis of all TestDelegates, allows safely injecting a OnceClosure to be
// run in the next idle phase of this delegate's Run() implementation. This can
// be used to have code run on a thread that is otherwise livelocked in an idle
// phase (sometimes a simple PostTask() won't do it -- e.g. when processing
// application tasks is disallowed).
class InjectableTestDelegate : public base::RunLoop::Delegate {
 public:
  void InjectClosureOnDelegate(base::OnceClosure closure) {
    base::AutoLock auto_lock(closure_lock_);
    closure_ = std::move(closure);
  }

  bool RunInjectedClosure() {
    base::AutoLock auto_lock(closure_lock_);
    if (closure_.is_null())
      return false;
    std::move(closure_).Run();
    return true;
  }

 private:
  base::Lock closure_lock_;
  base::OnceClosure closure_;
};

Application* Application::Create(
    int argc,
    char** argv,
    const std::shared_ptr<PlatformData>& platformData) {
  return new HelloWorld(argc, argv, platformData);
}

class MyPlatform : public blink::Platform {
  WebData UncompressDataResource(int resource_id) override {
    return WebData("");
  }

  WebString DefaultLocale() override { return WebString("en-US"); }
};

HelloWorld::HelloWorld(int argc,
                       char** argv,
                       const std::shared_ptr<PlatformData>& platformData)
    : fBackendType(Window::kRaster_BackendType),
      platformData(platformData),
      platform(std::make_unique<MyPlatform>()) {
  base::CommandLine::Init(argc, argv);
  base::CommandLine::StringVector parsedArgs =
      base::CommandLine::ForCurrentProcess()->GetArgs();

  if (parsedArgs.size() > 0) {
    htmlFilename = parsedArgs[0];
  }

  SkLoadICU();

  mojo::core::Init();

  // Creating a thread pool
  base::ThreadPoolInstance::CreateAndStartWithDefaultParams("MainThreadPool");

  // Creating a thread for composer (this thread will issue CSS animation tasks
  // for instance)
  base::TaskTraits default_traits = {base::ThreadPool()};
  composeTaskRunner = base::CreateSingleThreadTaskRunner(default_traits);

  my_web_thread_sched =
      blink::scheduler::WebThreadScheduler::CreateMainThreadScheduler(
          base::MessagePump::Create(base::MessagePumpType::DEFAULT),
          base::Optional<base::Time>());


  exit_manager = std::make_shared<base::AtExitManager>();

  binder_map = std::make_unique<mojo::BinderMap>();

  // Creating a discardable memory allocator
  discardableSharedMemoryManager =
      std::make_shared<discardable_memory::DiscardableSharedMemoryManager>();
  base::DiscardableMemoryAllocator::SetInstance(discardableSharedMemoryManager.get());

  blink::Initialize(platform.get(), binder_map.get(),
                    /*scheduler::WebThreadScheduler * main_thread_scheduler*/
                    my_web_thread_sched.get());

  blink::InitializePlatformLanguage();

  /*
  blink::WebFontRenderStyle::SetAutoHint(true);
  blink::WebFontRenderStyle::SetAntiAlias(true);
  blink::WebFontRenderStyle::SetSubpixelRendering(true);
  blink::WebFontRenderStyle::SetSubpixelPositioning(true);
  */

  backend = std::make_shared<SDK::Backend>();

  webViewHelper =
      std::make_shared<blink::my_frame_test_helpers::WebViewHelper>();
  wfc = std::make_shared<blink::my_frame_test_helpers::TestWebFrameClient>(
      backend);
  wfc->SetScheduler(my_web_thread_sched);

  wvc = std::make_shared<blink::my_frame_test_helpers::TestWebViewClient>();
  wwc = std::make_shared<blink::my_frame_test_helpers::TestWebWidgetClient>(
      new my_frame_test_helpers::StubLayerTreeViewDelegate(),
      my_web_thread_sched->DefaultTaskRunner(),  // mainTaskRunner,
      composeTaskRunner,
      my_web_thread_sched.get());

  webView = webViewHelper->InitializeAndLoad("mem://index.html", wfc.get(), wvc.get(), wwc.get());

  SkGraphics::Init();

  fWindow = Window::CreateNativeWindow(platformData);
  fWindow->setRequestedDisplayParams(DisplayParams());

  // register callbacks
  fWindow->pushLayer(this);
  fWindow->attach(fBackendType);
}

HelloWorld::~HelloWorld() {
  fWindow->detach();
  delete fWindow;

  // Resetting the WebViewHelper
  webViewHelper->Reset();
  webViewHelper = nullptr;

  // Shutting down the thread scheduler
  my_web_thread_sched->Shutdown();
  my_web_thread_sched = nullptr;
}

void HelloWorld::updateTitle() {
  if (!fWindow /*|| fWindow->sampleCount() <= 1*/) {
    return;
  }

  WTF::String title = "Blink window";
  if (GetDocument() != nullptr && GetDocument().head() != nullptr) {
      // Search for <title> tags in the <head>
      HTMLCollection* titleEls = GetDocument().head()->getElementsByTagName("title");
      if (titleEls != nullptr && titleEls->length() > 0) {
        // Taking the first one (assuming it is only one)
        title = titleEls->item(0)->innerText();
      }
  }
  SkString skTitle(title.Utf8().c_str());
  skTitle.append(" [");
  skTitle.append(Window::kRaster_BackendType == fBackendType ? "Raster"
                                                             : "OpenGL");
  skTitle.append("]");
  fWindow->setTitle(skTitle.c_str());
}

void HelloWorld::onBackendCreated() {
  std::cout << "HelloWorld::onBackendCreated" << std::endl;
  this->updateTitle();
  fWindow->show();
  fWindow->inval();
}

Document& HelloWorld::GetDocument() {
  return *((blink::Document*)webViewHelper->GetWebView()
               ->MainFrameImpl()
               ->GetDocument());
}

void HelloWorld::CollectLinkedDestinations(Node* node) {
  for (Node* i = node->firstChild(); i; i = i->nextSibling())
    CollectLinkedDestinations(i);

  auto* element = DynamicTo<Element>(node);
  if (!node->IsLink() || !element)
    return;
  const AtomicString& href = element->getAttribute(html_names::kHrefAttr);
  if (href.IsNull())
    return;
  KURL url = node->GetDocument().CompleteURL(href);
  if (!url.IsValid())
    return;

  if (url.HasFragmentIdentifier() &&
      EqualIgnoringFragmentIdentifier(url, node->GetDocument().BaseURL())) {
    String name = url.FragmentIdentifier();
    if (Element* element = node->GetDocument().FindAnchor(name))
      linked_destinations_.Set(name, element);
  }
}

void HelloWorld::OutputLinkedDestinations(GraphicsContext& context,
                                          const IntRect& page_rect) {
  // if (!linked_destinations_valid_) {
  // Collect anchors in the top-level frame only because our PrintContext
  // supports only one namespace for the anchors.
  CollectLinkedDestinations(&GetDocument());
  // linked_destinations_valid_ = true;
  //}

  for (const auto& entry : linked_destinations_) {
    LayoutObject* layout_object = entry.value->GetLayoutObject();
    if (!layout_object || !layout_object->GetFrameView())
      continue;
    IntPoint anchor_point = layout_object->AbsoluteBoundingBoxRect().Location();
    if (page_rect.Contains(anchor_point))
      context.SetURLDestinationLocation(entry.key, anchor_point);
  }
}

template <typename Function>
static void ForAllGraphicsLayers(GraphicsLayer& layer,
                                 const Function& function) {
  function(layer);
  for (auto* child : layer.Children())
    ForAllGraphicsLayers(*child, function);
}

void HelloWorld::PrintSinglePage(SkCanvas* canvas, int width, int height) {
  int kPageWidth = width;
  int kPageHeight = height;
  IntRect page_rect(0, 0, kPageWidth, kPageHeight);
  IntSize page_size(kPageWidth, kPageHeight);

  webView->Resize(WebSize(page_size));

  // Handling events

  for (auto& p : collectedInputEvents) {

    WebInputEvent& theEvent = *p;
    auto mtp = theEvent.GetType();
    switch (mtp) {
    case WebInputEvent::Type::kMouseDown:
        ((PageWidgetEventHandler*)webView)
            ->HandleMouseDown(*webView->MainFrameImpl()->GetFrame(),
                            (WebMouseEvent&)theEvent);
        break;

    case WebInputEvent::Type::kMouseUp:
        ((PageWidgetEventHandler*)webView)
            ->HandleMouseUp(*webView->MainFrameImpl()->GetFrame(),
                            (WebMouseEvent&)theEvent);
        break;

    case WebInputEvent::Type::kMouseMove:
        ((PageWidgetEventHandler*)webView)
            ->HandleMouseMove(*webView->MainFrameImpl()->GetFrame(),
                            (WebMouseEvent&)theEvent,
                            WebVector<const WebInputEvent*>(),
                            WebVector<const WebInputEvent*>());
        break;

    case WebInputEvent::Type::kMouseWheel:
        ((PageWidgetEventHandler*)webView)
            ->HandleMouseWheel(*webView->MainFrameImpl()->GetFrame(),
                                (WebMouseWheelEvent&)theEvent);
        break;

    case WebInputEvent::Type::kKeyDown:
    case WebInputEvent::Type::kKeyUp:
        ((PageWidgetEventHandler*)webView)
            ->HandleKeyEvent((WebKeyboardEvent&)theEvent);
        break;

    case WebInputEvent::Type::kChar:
        ((PageWidgetEventHandler*)webView)
            ->HandleCharEvent((WebKeyboardEvent&)theEvent);
        break;

    default:
        break;
    }
  }
  collectedInputEvents.clear();

  LocalFrameView* frame_view =
      webViewHelper->GetWebView()->MainFrameImpl()->GetFrameView();

  webView->MainFrameWidget()->UpdateAllLifecyclePhases(
      WebWidget::LifecycleUpdateReason::kBeginMainFrame);

  /*if (!resizing)*/ {
    // We don't update CSS animations during the window resizing because it
    // is significantly reducing the resizing process
    webView->MainFrameWidget()->BeginFrame(base::TimeTicks::Now(), false);
  } /*else {
  }*/

  PropertyTreeState property_tree_state =
      frame_view->GetLayoutView()->FirstFragment().LocalBorderBoxProperties();

  std::shared_ptr<cc::SkiaPaintCanvas> spc =
      std::make_shared<cc::SkiaPaintCanvas>(canvas);

  {
    blink::DisableCompositingQueryAsserts disabler;
    root_graphics_layer =
        GetDocument().GetLayoutView()->Compositor()->PaintRootGraphicsLayer();

    ForAllGraphicsLayers(*root_graphics_layer, [&](GraphicsLayer& layer) {
      if (layer.PaintsContentOrHitTest()) {
        layer.GetPaintController().GetPaintArtifact().Replay(
            *spc, property_tree_state, IntPoint(0, 0));
      }
    });
  }
}

void HelloWorld::SetBodyInnerHTML(String body_content) {
  GetDocument().body()->setAttribute(html_names::kStyleAttr, "margin: 0");
  GetDocument().body()->SetInnerHTMLFromString(body_content);
}

void HelloWorld::UpdateBackend() {
  // Checking if we need a fallback to Raster renderer.
  // Fallback is effective for small screens and weak videochips
  bool fallback = false;
  if (fWindow->width() * fWindow->height() <= 2560 * 1440 && resizing) {
    fallback = true;
  }

  // OpenGL context slows down the resizing process.
  // So we are changing the backend to software raster during resizing
  auto newBackendType =
      fallback ? Window::kRaster_BackendType : Window::kNativeGL_BackendType;

  // If there is no GL context allocated then falling back to raster
  if (fWindow->getGrContext() == nullptr) {
    newBackendType = Window::kRaster_BackendType;
  }

  // If too much time passed after the last attempt to init GL
  // and we aren't resizing, try again
  std::chrono::steady_clock::time_point curTime =
      std::chrono::steady_clock::now();
  if (std::chrono::duration_cast<std::chrono::milliseconds>(curTime -
                                                            lastGLInitAttempt)
              .count() > 1000 &&
      !fallback) {
    // Good luck to us!
    newBackendType = Window::kNativeGL_BackendType;
    lastGLInitAttempt = curTime;
  }

  if (fBackendType != newBackendType) {
    std::cout << "HelloWorld::UpdateBackend: updating backend" << std::endl;
    fBackendType = newBackendType;
    fWindow->detach();
    fWindow->attach(fBackendType);
    updateTitle();
  }
}

void HelloWorld::onResize(int width, int height) {
  UpdateBackend();
  fWindow->inval();
}

void HelloWorld::onBeginResizing() {
  std::cout << "HelloWorld::onBeginResizing" << std::endl;
  resizing = true;
  UpdateBackend();
}

void HelloWorld::onEndResizing() {
  std::cout << "HelloWorld::onEndResizing" << std::endl;
  resizing = false;
  UpdateBackend();
}

void HelloWorld::onPaint(SkSurface* surface) {
  auto* canvas = surface->getCanvas();

  int width = fWindow->width();
  int height = fWindow->height();

  canvas->save();

  updateTitle();
  PrintSinglePage(canvas, width, height);

  canvas->restore();
}

bool HelloWorld::onMouse(const ui::PlatformEvent& platformEvent,
                         int x,
                         int y,
                         skui::InputState inState,
                         skui::ModifierKey modKey) {
  WebInputEvent::Type mtp;
  switch (inState) {
    case skui::InputState::kDown:
      mtp = WebInputEvent::Type::kMouseDown;
      std::cout << "HelloWorld::onMouse [down]" << std::endl;
      break;
    case skui::InputState::kUp:
      mtp = WebInputEvent::Type::kMouseUp;
      std::cout << "HelloWorld::onMouse [up]" << std::endl;
      break;
    case skui::InputState::kMove:
      mtp = WebInputEvent::Type::kMouseMove;
      break;
    default:
      return false;
  }

  IntPoint pos(x, y);

  int mods = 0;

  auto mouseEvent = my_frame_test_helpers::CreateMouseEvent(
      mtp, WebMouseEvent::Button::kLeft, pos, mods);

  collectedInputEvents.push_back(
          std::make_shared<WebMouseEvent>(mouseEvent));

  return true;
}

bool HelloWorld::onMouseWheel(const ui::PlatformEvent& platformEvent,
    float delta,
    skui::ModifierKey modKey) {

    std::cout << "HelloWorld::onMouseWheel [delta = " << delta << "]"
            << std::endl;
    int mods = 0;

    auto mouseEvent = my_frame_test_helpers::CreateMouseWheelEvent(0, delta * 10, mods);
    mouseEvent.SetFrameTranslate(gfx::Vector2dF(0, 10 * delta));
    mouseEvent.event_action =
        blink::WebMouseWheelEvent::EventAction::kScrollVertical;

    collectedInputEvents.push_back(
        std::make_shared<WebMouseWheelEvent>(mouseEvent));

    return true;
}

bool HelloWorld::onKey(const ui::PlatformEvent& platformEvent,
                       uint64_t key,
                       skui::InputState inState,
                       skui::ModifierKey modKey) {
  std::unique_ptr<ui::Event> evt = ui::EventFromNative(platformEvent);
  if (evt == nullptr)
    return false;

  std::shared_ptr<blink::WebInputEvent> bEvent = nullptr;
  if (evt->IsKeyEvent()) {
    auto* keyEvt = evt->AsKeyEvent();
    std::shared_ptr<blink::WebKeyboardEvent> bKbdEvent;
    WebInputEvent::Type mtp;

    switch (keyEvt->type()) {
      case ui::ET_KEY_PRESSED:
        mtp = WebInputEvent::Type::kKeyDown;
        break;
      case ui::ET_KEY_RELEASED:
        mtp = WebInputEvent::Type::kKeyUp;
        break;
      default:
        return false;
    }

    bKbdEvent =
        std::make_shared<blink::WebKeyboardEvent>(mtp, 0, base::TimeTicks());
    bKbdEvent->text[0] = keyEvt->GetText();
    bKbdEvent->windows_key_code = keyEvt->key_code();
    bKbdEvent->dom_key = keyEvt->GetDomKey();  // GetCharacter();

    bEvent = bKbdEvent;
  }

  if (evt->IsKeyEvent() /* || evt.IsMouseEvent() || evt.IsTouchEvent()*/) {
    collectedInputEvents.push_back(bEvent);
    return true;
  }
  return false;
}

bool HelloWorld::onChar(const ui::PlatformEvent& platformEvent,
                        SkUnichar c,
                        skui::ModifierKey modifiers) {
  return false;
}

void HelloWorld::onIdle() {
  UpdateBackend();

  // Update contents if necessary
  std::chrono::steady_clock::time_point curTime =
      std::chrono::steady_clock::now();
  if (std::chrono::duration_cast<std::chrono::milliseconds>(
          curTime - htmlContentsUpdateTime)
          .count() > 500) {
    htmlContentsUpdateTime = curTime;
  }

  // Processing the pending commands
  base::RunLoop run_loop;
  run_loop.RunUntilIdle();

  // Just re-paint continously
  fWindow->inval();
}

void HelloWorld::onAttach(sk_app::Window* window) {}
