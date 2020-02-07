/*
 * Copyright 2017 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "HelloWorld.h"

#include <fstream>
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
//#include "third_party/blink/renderer/core/testing/core_unit_test_helper.h"

#include "base/message_loop/message_pump.h"
#include "base/run_loop.h"
#include "base/time/default_tick_clock.h"
#include "mojo/core/embedder/embedder.h"
#include "third_party/blink/public/platform/scheduler/web_thread_scheduler.h"
#include "third_party/blink/public/platform/web_font_render_style.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/renderer/core/frame/visual_viewport.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_record_builder.h"
#include "third_party/blink/renderer/platform/heap/heap.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/main_thread_scheduler_impl.h"
#include "third_party/blink/renderer/platform/scheduler/public/dummy_schedulers.h"

//#include
//"third_party/blink/renderer/platform/testing/paint_test_configurations.h"

#include "base/task/post_task.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/thread_pool/thread_pool_impl.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "my_frame_test_helpers.h"
#include "third_party/blink/renderer/platform/wtf/text/text_stream.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/skia/include/core/SkCanvas.h"

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

// A simple test RunLoop::Delegate to exercise Runloop logic independent of any
// other base constructs. BindToCurrentThread() must be called before this
// TestBoundDelegate is operational.
class TestBoundDelegate final : public InjectableTestDelegate {
 public:
  TestBoundDelegate() = default;

  // Makes this TestBoundDelegate become the RunLoop::Delegate and
  // ThreadTaskRunnerHandle for this thread.
  void BindToCurrentThread() {
    thread_task_runner_handle_ =
        std::make_unique<base::ThreadTaskRunnerHandle>(simple_task_runner_);
    base::RunLoop::RegisterDelegateForCurrentThread(this);
  }

 private:
  void Run(bool application_tasks_allowed, base::TimeDelta timeout) override {
    if (nested_run_allowing_tasks_incoming_) {
      // EXPECT_TRUE(base::RunLoop::IsNestedOnCurrentThread());
      // EXPECT_TRUE(application_tasks_allowed);
    } else if (base::RunLoop::IsNestedOnCurrentThread()) {
      // EXPECT_FALSE(application_tasks_allowed);
    }
    nested_run_allowing_tasks_incoming_ = false;

    while (!should_quit_) {
      if (application_tasks_allowed && simple_task_runner_->ProcessSingleTask())
        continue;

      if (ShouldQuitWhenIdle())
        break;

      if (RunInjectedClosure())
        continue;

      base::PlatformThread::YieldCurrentThread();
    }
    should_quit_ = false;
  }

  void Quit() override { should_quit_ = true; }

  void EnsureWorkScheduled() override {
    nested_run_allowing_tasks_incoming_ = true;
  }

  // True if the next invocation of Run() is expected to be from a
  // True if the next invocation of Run() is expected to be from a
  // kNestableTasksAllowed RunLoop.
  bool nested_run_allowing_tasks_incoming_ = false;

  scoped_refptr<SimpleSingleThreadTaskRunner> simple_task_runner_ =
      base::MakeRefCounted<SimpleSingleThreadTaskRunner>();

  std::unique_ptr<base::ThreadTaskRunnerHandle> thread_task_runner_handle_;

  bool should_quit_ = false;
};

Application* Application::Create(int argc, char** argv, void* platformData) {
  //     base::RunLoop run_loop;
  return new HelloWorld(argc, argv, platformData);
}

class MyPlatform : public blink::Platform {
  WebData UncompressDataResource(int resource_id) override {
    return WebData("");
  }

  WebString DefaultLocale() override { return WebString("en-US"); }
};

HelloWorld::HelloWorld(int argc, char** argv, void* platformData)
    : fBackendType(Window::kRaster_BackendType),
      fRotationAngle(0),
      platform(std::make_unique<MyPlatform>()) {
  // WTF::Partitions::Initialize();
  // WTF::Initialize(nullptr /*TODO: make here call on main thread*/);
  ////WTF::Threading::Initialize();
  // blink::GCInfoTable::CreateGlobalTable();
  // blink::ThreadState::AttachMainThread();
  ////blink::ThreadState::AttachCurrentThread();

  // base::SingleThreadTaskExecutor main_task_executor;
  // mojo::core::Init();

  // if (argc == 2) {
  //    htmlFilename = std::string(argv[1]);
  //}

  base::CommandLine::Init(argc, argv);
  base::CommandLine::StringVector parsedArgs =
      base::CommandLine::ForCurrentProcess()->GetArgs();

  if (parsedArgs.size() > 0) {
    htmlFilename = parsedArgs[0];
  }

  // blink::Platform::CreateMainThreadAndInitialize(platform.get());
  // blink::Platform::Initialize(platform.get(), my_web_thread_sched.get());

  mojo::core::Init();

  mainTaskRunner = scoped_refptr<base::SingleThreadTaskRunner>(
      new SimpleSingleThreadTaskRunner());
  composeTaskRunner = nullptr;
  /*scoped_refptr<base::SingleThreadTaskRunner>(
      new SimpleSingleThreadTaskRunner());*/

  /*base::SequencedTaskRunnerHandle* strh =*/
  // new base::SequencedTaskRunnerHandle(mainTaskRunner);
  // new base::ThreadTaskRunnerHandle(mainTaskRunner);

  //  new base::internal::ScopedSetSequenceLocalStorageMapForCurrentThread(
  //      new base::internal::SequenceLocalStorageMap());

  my_web_thread_sched =
      blink::scheduler::WebThreadScheduler::CreateMainThreadScheduler(
          base::MessagePump::Create(base::MessagePumpType::DEFAULT),
          base::Optional<base::Time>());

  run_loop = std::make_shared<base::RunLoop>();

  binder_map = std::make_unique<mojo::BinderMap>();
  blink::Initialize(platform.get(), /*mojo::BinderMap**/ binder_map.get(),
                    /*scheduler::WebThreadScheduler * main_thread_scheduler*/
                    my_web_thread_sched.get());

  blink::InitializePlatformLanguage();

  /*
  blink::WebFontRenderStyle::SetAutoHint(true);
  blink::WebFontRenderStyle::SetAntiAlias(true);
  blink::WebFontRenderStyle::SetSubpixelRendering(true);
  blink::WebFontRenderStyle::SetSubpixelPositioning(true);
  */

  webViewHelper =
      std::make_shared<blink::my_frame_test_helpers::WebViewHelper>();
  wfc = std::make_shared<blink::my_frame_test_helpers::TestWebFrameClient>();
  wvc = std::make_shared<blink::my_frame_test_helpers::TestWebViewClient>();
  wwc = std::make_shared<blink::my_frame_test_helpers::TestWebWidgetClient>(
      new my_frame_test_helpers::StubLayerTreeViewDelegate(), mainTaskRunner,
      composeTaskRunner, my_web_thread_sched.get());

  webView = webViewHelper->Initialize(wfc.get(), wvc.get(), wwc.get());

  // base::ThreadPoolInstance::Set(std::make_unique<::base::internal::ThreadPoolImpl>("MainThreadPool"));
  base::ThreadPoolInstance::CreateAndStartWithDefaultParams("MainThreadPool");

  UpdateContents();

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

  // webView->Close();
  webViewHelper->Reset();
  webViewHelper = nullptr;
  // dynamic_cast<blink::scheduler::MainThreadSchedulerImpl*>(
  my_web_thread_sched  //)
      ->Shutdown();
  my_web_thread_sched = nullptr;
}

void HelloWorld::updateTitle() {
  if (!fWindow || fWindow->sampleCount() <= 1) {
    return;
  }

  SkString title("Hello World");
  title.append(Window::kRaster_BackendType == fBackendType ? "Raster"
                                                           : "OpenGL");
  fWindow->setTitle(title.c_str());
}

void HelloWorld::onBackendCreated() {
  this->updateTitle();
  fWindow->show();
  fWindow->inval();
}

Document& HelloWorld::GetDocument() {
  return *((blink::Document*)webViewHelper->GetWebView()
               ->MainFrameImpl()
               ->GetDocument());
}

// static String AbsoluteBlockHtmlForLink(int x,
//                                       int y,
//                                       int width,
//                                       int height,
//                                       const char* url,
//                                       const char* children = nullptr) {
//  WTF::TextStream ts;
//  ts << WTF::String("<a>children_go_here</a>"); /*
//  << x << "px; top: " << y
//     << "px; width: " << width << "px; height: " << height << "px' href='"
//     << url << "'>" << (children ? children : url) << "</a>";*/
//  return ts.Release();
//}

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

bool HelloWorld::Capture(/*cc::PaintCanvas**/ SkCanvas* canvas,
                         FloatSize size) {
  // This code is based on ChromePrintContext::SpoolSinglePage()/SpoolPage().
  // It differs in that it:
  //   1. Uses a different set of flags for painting and the graphics context.
  //   2. Paints a single page of |size| rather than a specific page in a
  //      reformatted document.
  //   3. Does no scaling.
  //  if (!GetDocument() || !GetDocument()->GetLayoutView())
  //    return false;
  //GetDocument().View()->UpdateLifecyclePhasesForPrinting();
  //  if (!GetDocument() || !GetDocument()->GetLayoutView())
  //    return false;
  FloatRect bounds(0, 0, size.Width(), size.Height());
  PaintRecordBuilder builder(nullptr, nullptr, nullptr/*,
                             canvas->GetPaintPreviewTracker()*/); // < --MAY BE NECESSARY

  builder.Context().GetPaintController().SetDisplayItemConstructionIsDisabled(
      true);
  builder.Context().BeginRecording(FloatRect());

  builder.Context().SetIsPaintingPreview(true);

  LocalFrameView* frame_view =
      webViewHelper->GetWebView()->MainFrameImpl()->GetFrameView();
  DCHECK(frame_view);
  PropertyTreeState property_tree_state =
      frame_view->GetLayoutView()->FirstFragment().LocalBorderBoxProperties();

  // This calls BeginRecording on |builder| with dimensions specified by the
  // CullRect.
  frame_view->PaintContentsOutsideOfLifecycle(
      builder.Context(),
      kGlobalPaintNormalPhase | kGlobalPaintFlattenCompositingLayers |
          kGlobalPaintAddUrlMetadata,
      CullRect(RoundedIntRect(bounds)));
  {
    // Add anchors.
    // ScopedPaintChunkProperties scoped_paint_chunk_properties(
    //    builder.Context().GetPaintController(), property_tree_state, builder,
    //    DisplayItem::kPrintedContentDestinationLocations); <-- MAY BE
    //    NECESSARY
    DrawingRecorder line_boundary_recorder(
        builder.Context(), builder,
        DisplayItem::kPrintedContentDestinationLocations
        /*DisplayItem::kDocumentBackground*/);

    builder.Context().GetPaintController().SetDisplayItemConstructionIsDisabled(
        false);

    linked_destinations_.clear();
    OutputLinkedDestinations(builder.Context(), RoundedIntRect(bounds));
  }
  // canvas->drawPicture(builder.EndRecording(property_tree_state));         <--
  // MAY BE NECESSARY

  sk_sp<PaintRecord> playlist = builder.EndRecording(property_tree_state);
  playlist->Playback(canvas);

  return true;
}

void HelloWorld::PrintSinglePage(SkCanvas* canvas, int width, int height) {
  int kPageWidth = width;
  int kPageHeight = height;
  IntRect page_rect(0, 0, kPageWidth, kPageHeight);
  IntSize page_size(kPageWidth, kPageHeight);
  FloatSize page_float_size((float)kPageWidth, (float)kPageHeight);

  /*GetDocument().SetPrinting(Document::kBeforePrinting);
  Event* event = MakeGarbageCollected<BeforePrintEvent>();
  GetPrintContext().GetFrame()->DomWindow()->DispatchEvent(*event);
  GetPrintContext().BeginPrintMode(page_rect.Width(), page_rect.Height());
  */

  // UpdateAllLifecyclePhasesForTest();
  // PaintRecordBuilder builder;
  // GraphicsContext& context = builder.Context();
  // context.SetPrinting(true);

  // SetBodyInnerHTML("Hello, losers!blah blah blah blah blah blah blah");

  VisualViewport& visual_viewport = webViewHelper->GetWebView()
                                        ->MainFrameImpl()
                                        ->GetFrame()
                                        ->GetPage()
                                        ->GetVisualViewport();

  webViewHelper->GetWebView()
      ->MainFrameImpl()
      ->GetFrameView()
      ->SetInitialViewportSize(page_size);
  webViewHelper->GetWebView()->MainFrameImpl()->GetFrameView()->SetLayoutSize(
      page_size);

  visual_viewport.SetSize(page_size);

  // GetPrintContext().OutputLinkedDestinations(context, page_rect);
  GetDocument().View()->UpdateLayout();
  GetDocument().View()->UpdateAllLifecyclePhasesExceptPaint();
  //    GetDocument().View()->PaintContentsOutsideOfLifecycle(
  //        context,
  //        kGlobalPaintNormalPhase |
  //            /*kGlobalPaintPrinting | */ kGlobalPaintFlattenCompositingLayers
  //            | kGlobalPaintAddUrlMetadata,
  //        CullRect(page_rect));
  ////  {
  ////    DrawingRecorder recorder(
  ////        context, *GetDocument().GetLayoutView(),
  ////        DisplayItem::kDocumentBackground
  ////        /*DisplayItem::kPrintedContentDestinationLocations*/);
  ////  }
  //
  //
  //  sk_sp<PaintRecord> playlist = builder.EndRecording();
  //  playlist->Playback(canvas);
  //

  Capture(canvas, page_float_size);
}

void HelloWorld::SetBodyInnerHTML(String body_content) {
  GetDocument().body()->setAttribute(html_names::kStyleAttr, "margin: 0");
  GetDocument().body()->SetInnerHTMLFromString(body_content);
}
void HelloWorld::onPaint(SkSurface* surface) {
  auto* canvas = surface->getCanvas();

  int width = fWindow->width();
  int height = fWindow->height();

  // Clear background
  SkColor windowBgColor = SkColorSetARGB(255, 128, 128, 255);  // SK_ColorWHITE;
#ifdef WIN32
//  CDC* pDc = GetDC();
//  COLORREF win32WindowBG = pDc->GetBkColor();
//  BYTE r = GetRValue(windowBG);
//  BYTE g = GetRValue(windowBG);
//  BYTE b = GetRValue(windowBG);
//  windowBgColor = (SkColor)win32WindowBG;
#endif

  canvas->clear(windowBgColor);

  // SkPaint paint;
  // paint.setColor(SK_ColorRED);

  // Draw a rectangle with red paint
  // SkRect rect = SkRect::MakeXYWH(10, 10, 128, 128);
  // canvas->drawRect(rect, paint);

  // Set up a linear gradient and draw a circle
  /*{
    SkPoint linearPoints[] = {{0, 0}, {300, 300}};
    SkColor linearColors[] = {SK_ColorGREEN, SK_ColorBLACK};
    paint.setShader(SkGradientShader::MakeLinear(
        linearPoints, linearColors, nullptr, 2, SkTileMode::kMirror));
    paint.setAntiAlias(true);

    canvas->drawCircle(200, 200, 64, paint);

    // Detach shader
    paint.setShader(nullptr);
  }*/

  // Draw a message with a nice black paint
  /*SkFont font;
  font.setSubpixel(true);
  font.setSize(20);
  paint.setColor(SK_ColorBLACK);*/

  canvas->save();
  // static const char message[] = "Hello World";

  // Translate and rotate
  fRotationAngle += 0.2f;
  /*canvas->translate(300, 300);
  if (fRotationAngle > 360) {
    fRotationAngle -= 360;
  }
  canvas->rotate(fRotationAngle);

  // Draw the text
  canvas->drawSimpleText(message, strlen(message), SkTextEncoding::kUTF8, 0, 0,
                         font, paint);

  SetBodyInnerHTML(
      AbsoluteBlockHtmlForLink(50, 60, 70, 80, "http://www.google.com") +
      AbsoluteBlockHtmlForLink(150, 160, 170, 180,
                               "http://www.google.com#fragment"));*/
  PrintSinglePage(canvas, width, height);

  canvas->restore();
}

void HelloWorld::UpdateContents() {
  std::ifstream htmlFile(htmlFilename);
  if (!htmlFile.good()) {
    GetDocument().SetContent(
        "<html style=\"background-color: blue\">"
        "<head>"
        "<script>document.write(\"boo! \" + (3+2));</script>"
        "</head>"
        "<body style=\""
        "display: block; "
        "margin: 0px; "
        "width: 100%; "
        "height: 100%; "
        "font-size: 10px; "
        "font-family: Arial, Helvetica, sans-serif; "
        "color: #cccccc "

        "\">"
        "Hello, losers! <div style=\"background-color: lightblue; "
        "display: "
        "block\">blah blah blah blah blah</div> blah blah"
        "</body>"
        "</html>");
  } else {
    std::stringstream ss;
    ss << htmlFile.rdbuf();

    std::string newHTMLContents = ss.str();

    if (htmlContents != newHTMLContents) {
      htmlContents = newHTMLContents;
      GetDocument().SetContent(WTF::String::FromUTF8(htmlContents.c_str()));
    }
  }
}

void HelloWorld::onIdle() {
  // Update contents if necessary

  std::chrono::steady_clock::time_point curTime =
      std::chrono::steady_clock::now();
  if (std::chrono::duration_cast<std::chrono::milliseconds>(
          curTime - htmlContentsUpdateTime)
          .count() > 500) {
    htmlContentsUpdateTime = curTime;
    UpdateContents();
  }

  // Platform::Current()->AdvanceClockSeconds(0.01);
  run_loop->RunUntilIdle();

  // Just re-paint continously
  fWindow->inval();
}

void HelloWorld::onAttach(sk_app::Window* window) {}

bool HelloWorld::onChar(SkUnichar c, skui::ModifierKey modifiers) {
  if (' ' == c) {
    fBackendType = Window::kRaster_BackendType == fBackendType
                       ? Window::kNativeGL_BackendType
                       : Window::kRaster_BackendType;
    fWindow->detach();
    fWindow->attach(fBackendType);
  }
  return true;
}
