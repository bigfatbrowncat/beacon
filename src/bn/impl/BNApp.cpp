#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkGraphics.h"
#include "include/core/SkSurface.h"
#include "include/effects/SkGradientShader.h"

#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/core/animation/document_animations.h"
#include "third_party/blink/renderer/core/animation/document_timeline.h"
#include "third_party/blink/renderer/core/events/before_print_event.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/visual_viewport.h"
#include "third_party/blink/renderer/core/frame/web_frame_widget_base.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/html/html_collection.h"
#include "third_party/blink/renderer/core/html/html_element.h"
#include "third_party/blink/renderer/core/html/html_head_element.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/page/focus_controller.h"
#include "third_party/blink/renderer/core/page/print_context.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/paint/paint_layer_painter.h"
#include "third_party/blink/renderer/core/scroll/scrollbar_theme.h"
#include "third_party/blink/renderer/platform/animation/compositor_animation_timeline.h"
#include "third_party/blink/renderer/platform/fonts/web_font_typeface_factory.h"
#include "third_party/blink/renderer/platform/geometry/int_point.h"
#include "third_party/blink/renderer/platform/graphics/compositing/paint_artifact_compositor.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_record_builder.h"
#include "third_party/blink/renderer/platform/heap/heap.h"
#include "third_party/blink/renderer/platform/language.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/main_thread_scheduler_impl.h"
#include "third_party/blink/renderer/platform/scheduler/public/dummy_schedulers.h"
#include "third_party/blink/renderer/platform/wtf/text/text_stream.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

#include "third_party/blink/public/platform/scheduler/web_thread_scheduler.h"
#include "third_party/blink/public/platform/web_coalesced_input_event.h"
#include "third_party/blink/public/platform/web_font_render_style.h"
#include "third_party/blink/public/resources/grit/blink_resources.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_render_theme.h"

#include "base/message_loop/message_pump.h"
#include "base/run_loop.h"
#include "base/time/default_tick_clock.h"

#include "base/files/file_path.h"
#include "base/memory/discardable_memory_allocator.h"
#include "base/task/post_task.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/thread_pool/thread_pool_impl.h"
#include "base/task/thread_pool/thread_pool_instance.h"

#include "cc/paint/skia_paint_canvas.h"
#include "cc/animation/animation_host.h"

#include "mojo/core/embedder/embedder.h"

#include "ui/events/blink/blink_event_util.h"
#ifdef __linux__
// FIXME: maybe switch to proper X11 handling from events/platform/x11
#include "ui/events/devices/x11/device_data_manager_x11.h"
#endif
#include "ui/base/resource/resource_bundle.h"

#if defined(OS_WIN)
#include "base/strings/utf_string_conversions.h"
#include "third_party/blink/public/web/win/web_font_rendering.h"
#include "ui/events/blink/web_input_event_builders_win.h"
#endif

#include <bn/impl/BNApp.h>
#include <bn/impl/BNBlinkPlatformImpl.h>

#include <bn/glue/SkLoadICU.h>
#include <bn/glue/my_blink_platform_impl.h>
#include <bn/glue/my_frame_test_helpers.h>

using namespace app_base;
using namespace blink;

extern "C" uint8_t blink_resources_pak[];     /* binary data         */
extern "C" uint32_t blink_resources_pak_size; /* size of binary data */

namespace beacon::impl {

// Extracts a C string from a V8 Utf8Value.
const char* ToCString(const v8::String::Utf8Value& value) {
  return *value ? *value : "<string conversion failed>";
}

// The callback that is invoked by v8 whenever the JavaScript 'print'
// function is called.  Prints its arguments on stdout separated by
// spaces and ending with a newline.
void Print(const v8::FunctionCallbackInfo<v8::Value>& args) {
  bool first = true;
  for (int i = 0; i < args.Length(); i++) {
    v8::HandleScope handle_scope(args.GetIsolate());
    if (first) {
      first = false;
    } else {
      std::cout << " ";
    }
    v8::String::Utf8Value str(args.GetIsolate(), args[i]);
    const char* cstr = ToCString(str);
    std::cout << cstr;
  }
  std::cout << std::endl;
}

BNViewLayerWindow::BNViewLayerWindow(BNApp& app) :
#ifndef __APPLE__
      BNLayer(app_base::Window::kRaster_BackendType),
#else
      BNLayer(app_base::Window::kNativeGL_BackendType),
#endif
      app(app) {

  auto fWindow = app_base::Window::CreateNativeWindow(app.getPlatformData());
  fWindow->setRequestedDisplayParams(DisplayParams());

  getApplication().getPlatform()->TakeExampleWindow(fWindow);

  // Tuin
  /*blink::WebFontRenderStyle::SetAutoHint(true);
  blink::WebFontRenderStyle::SetAntiAlias(true);
  blink::WebFontRenderStyle::SetSubpixelRendering(true);
  blink::WebFontRenderStyle::SetSubpixelPositioning(true);*/

  // Creating a thread for composer
  // (this thread will issue CSS animation tasks, for instance)
  base::TaskTraits default_traits = {base::ThreadPool()};
  composeTaskRunner = base::CreateSingleThreadTaskRunner(default_traits);

  backend = std::make_shared<beacon::sdk::Backend>();

  webViewHelper = std::make_shared<beacon::glue::WebViewHelper>();
  wfc = std::make_shared<beacon::glue::TestWebFrameClient>(backend, static_cast<beacon::impl::BNViewLayerWindow *>(this));
  wfc->SetScheduler(getApplication().getThreadScheduler());

  wvc = std::make_shared<beacon::glue::TestWebViewClient>(webViewHelper);
  wwc = std::make_shared<beacon::glue::TestWebWidgetClient>(
      new beacon::glue::StubLayerTreeViewDelegate(),
      getApplication().getThreadScheduler()->DefaultTaskRunner(),  // mainTaskRunner,
      composeTaskRunner, getApplication().getThreadScheduler().get());

#ifdef __linux__
  // needed for proper XEvent handling like mouse scrolling
  ui::DeviceDataManagerX11::CreateInstance();
#endif

  webView = webViewHelper->InitializeAndLoad("mem://index.html", wfc.get(),
                                             wvc.get(), wwc.get());

  // register callbacks
  connectWindow(fWindow);

  // Setting the WebView scaling factor according to the screen DPI
  float scaleFactor = fWindow->getScale();
  webView->SetZoomFactorOverride(scaleFactor);

  // Setting the main view initially focused
  webViewHelper->SetFocused();

  // Setting caret visible
  blink::WebLocalFrameImpl& frm = *webView->MainFrameImpl();
  frm.SetCaretVisible(true);

  // Setting up the v8 global object

  v8::Isolate* isolate = ToIsolate(frm.GetFrame());
  {
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Context> v8context = frm.MainWorldScriptContext();
    v8::Local<v8::Object> v8proxyProto = frm.GlobalProxy();  //->GetPrototype();

    v8::Local<v8::Value> printKey =
        v8::String::NewFromUtf8(isolate, "print", v8::NewStringType::kNormal)
            .ToLocalChecked();
    v8::Local<v8::Value> printValue = v8::FunctionTemplate::New(isolate, Print)
                                          ->GetFunction(v8context)
                                          .ToLocalChecked();
    v8proxyProto->Set(v8context, printKey, printValue).Check();
  }

  // Setting the initial size
  IntSize page_size(fWindow->width(), fWindow->height());
  webView->Resize(WebSize(page_size));
}


BNApp::BNApp(int argc,
             char** argv,
             const std::shared_ptr<PlatformData>& platformData)
    : platformData(platformData) {
  
  exit_manager = std::make_shared<base::AtExitManager>();

  base::CommandLine::Init(argc, argv);
  base::CommandLine::StringVector parsedArgs =
      base::CommandLine::ForCurrentProcess()->GetArgs();

  if (parsedArgs.size() > 0) {
    // Processing the command line
  }

  if (!beacon::glue::SkLoadICU()) {
    std::cerr << "Can't load ICU4C data" << std::endl;
  }

  mojo::core::Init();

  if (!ui::ResourceBundle::HasSharedInstance()) {
    ui::ResourceBundle::InitSharedInstanceWithLocale(
        "en-US", nullptr, ui::ResourceBundle::DO_NOT_LOAD_COMMON_RESOURCES);

    // Adding the blink_resources.pak embeded into the binary
    base::StringPiece blink_pak_memory((char*)blink_resources_pak,
                                       blink_resources_pak_size);

    ui::ResourceBundle::GetSharedInstance().AddDataPackFromBuffer(
        blink_pak_memory, ui::SCALE_FACTOR_100P);
  }

  // Creating a thread pool
  base::ThreadPoolInstance::CreateAndStartWithDefaultParams("MainThreadPool");

  
  binder_map = std::make_unique<mojo::BinderMap>();

  SkGraphics::Init();

  my_web_thread_sched =
      blink::scheduler::WebThreadScheduler::CreateMainThreadScheduler(
          base::MessagePump::Create(base::MessagePumpType::DEFAULT),
          base::Optional<base::Time>());

  platform = std::make_unique<BNBlinkPlatformImpl>(
      getThreadScheduler()->DefaultTaskRunner(),
      getThreadScheduler()->DefaultTaskRunner());

  blink::Initialize(platform.get(), binder_map.get(),
                    /*scheduler::WebThreadScheduler * main_thread_scheduler*/
                    my_web_thread_sched.get());

  blink::InitializePlatformLanguage();

  // Creating a discardable memory allocator
  discardableSharedMemoryManager =
      std::make_shared<discardable_memory::DiscardableSharedMemoryManager>();
  base::DiscardableMemoryAllocator::SetInstance(
      discardableSharedMemoryManager.get());

  viewLayerWindow = std::make_shared<BNViewLayerWindow>(*this);
}

BNViewLayerWindow::~BNViewLayerWindow() {
  // Resetting the WebViewHelper
  webViewHelper->Reset();
  webViewHelper = nullptr;
}

BNApp::~BNApp() {
  viewLayerWindow = nullptr;

  // Shutting down the thread scheduler
  my_web_thread_sched->Shutdown();
  my_web_thread_sched = nullptr;
}

std::string BNViewLayerWindow::getTitle() {
  WTF::String title = "Blink window";
  if (GetDocument() != nullptr && GetDocument().head() != nullptr) {
    // Search for <title> tags in the <head>
    HTMLCollection* titleEls =
        GetDocument().head()->getElementsByTagName("title");
    if (titleEls != nullptr && titleEls->length() > 0) {
      // Taking the first one (assuming it is only one)
      title = titleEls->item(0)->innerText();
    }
  }
  return std::string(title.Utf8().c_str());
}


Document& BNViewLayerWindow::GetDocument() {
  return *((blink::Document*)webViewHelper->GetWebView()
               ->MainFrameImpl()
               ->GetDocument());
}

template <typename Function>
static void ForAllGraphicsLayers(GraphicsLayer& layer,
                                 const Function& function) {
  function(layer);
  for (auto* child : layer.Children())
    ForAllGraphicsLayers(*child, function);
}



void BNViewLayerWindow::onResize(int width, int height) {
  int kPageWidth = width;
  int kPageHeight = height;
  IntSize page_size(kPageWidth, kPageHeight);

  webView->Resize(WebSize(page_size));

  BNLayer::onResize(width, height);
}


bool BNViewLayerWindow::UpdateViewIfNeededAndBeginFrame() {
  GetDocument().GetPage()->GetFocusController().SetActive(isWindowActive());

  bool anything_changed = false;
  // std::cout << "anything_changed: ";

  // if (collectedInputEvents.size() > 0) {
  //  anything_changed = true;
  // std::cout << "events ";
  //}

  // Handling events
  webViewHelper->GetWebWidgetClient()->HandleScrollEvents(
      webView->MainFrameWidget());

  for (auto& p : collectedInputEvents) {
    WebInputEvent& theEvent = *p;
    auto mtp = theEvent.GetType();

    auto frame_widget =
        ((WebFrameWidgetBase*)webView->MainFrameImpl()->FrameWidget());
    if (mtp == WebInputEvent::Type::kMouseUp) {
      // Ending drag on mouse up event. That prevents input disabling
      frame_widget->DragSourceSystemDragEnded();
    }

    if (mtp != WebInputEvent::Type::kMouseMove ||
        !frame_widget->DoingDragAndDrop()) {
      // Any event except mouse moving without an active
      // drag-drop operation is considered a changing operation
      anything_changed = true;
    }

    webView->MainFrameWidget()->HandleInputEvent(
        WebCoalescedInputEvent(theEvent));
  }
  collectedInputEvents.clear();

  // webView->MainFrameWidget()->BeginFrame(base::TimeTicks::Now(), false);

  // Updating the state machine

  LocalFrameView* frame_view =
      webViewHelper->GetWebView()->MainFrameImpl()->GetFrameView();

  {
    blink::DisableCompositingQueryAsserts disabler;

    // auto root_graphics_layer =
    //    GetDocument().GetLayoutView()->Compositor()->PaintRootGraphicsLayer();

    /*ForAllGraphicsLayers(*root_graphics_layer, [&](GraphicsLayer& layer)
     * {});*/

    auto anims = GetDocument().GetDocumentAnimations().getAnimations();
    int playing_anims = 0;
    for (WTF::wtf_size_t ii = 0; ii < anims.size(); ii++) {
      if (anims[ii]->Playing())
        playing_anims++;
    }

    if (playing_anims > 0) {
      anything_changed = true;
    }

    webView->MainFrameWidget()->UpdateLifecycle(
        WebWidget::LifecycleUpdate::kPrePaint,
        WebWidget::LifecycleUpdateReason::kBeginMainFrame);

    if (frame_view->GetPaintArtifactCompositor() != nullptr &&
        frame_view->GetPaintArtifactCompositor()->NeedsUpdate()) {
      // If the compositor thinks we need to update -- we need to update indeed
      anything_changed = true;
    }

    const LocalFrameView::ScrollableAreaSet* anim_scrolls = frame_view->AnimatingScrollableAreas();
    if (anim_scrolls != nullptr && anim_scrolls->size() > 0) {
      // If there is an animating scrollbar, we need to update
      anything_changed = true;
    }

    /* cc::PropertyTrees* pts = frame_view->GetPaintArtifactCompositor()
                                 ->GetPropertyTreesForDirectUpdate();
    if (pts->changed) {
      anything_changed = true;
      std::cout << "changed" << std::endl;
    }*/

    /*frame_view->GetFrame().LocalFrameRoot().View()->UpdateLifecyclePhases(
        DocumentLifecycle::kPrePaintClean, reason);*/

    // PropertyTreeState property_tree_state =
    //    frame_view->GetLayoutView()->FirstFragment().LocalBorderBoxProperties();

    // if (frame_view->GetPaintArtifactCompositor()->NeedsUpdate()) {
    // auto layout_view = GetDocument().GetLayoutView();
    // auto compositor = layout_view->Compositor();
    // compositor->UpdateIfNeededRecursive(DocumentLifecycle::LifecycleState::kInPrePaint);

    // PaintArtifactCompositor handles all the changes except animations

    /* ForAllGraphicsLayers(*root_graphics_layer, [&](GraphicsLayer& layer) {
      if (layer.PaintsContentOrHitTest()) {
        anything_changed = true;
        std::cout << "layer";
      }
    });*/

    // std::cout << std::endl;

    return anything_changed;
  }
}

void BNViewLayerWindow::Paint(SkCanvas* canvas) {
  // std::cout << "BNApp::Paint()" << std::endl;

  // GetDocument().GetPage()->GetFocusController().SetActive(
  //    this->fWindow->IsActive());

  // Handling events

  /*webViewHelper->GetWebWidgetClient()->HandleScrollEvents(
      webView->MainFrameWidget());

  for (auto& p : collectedInputEvents) {
    WebInputEvent& theEvent = *p;
    auto mtp = theEvent.GetType();

    if (mtp == WebInputEvent::Type::kMouseUp) {
      // Ending drag on mouse up event. That prevents input disabling
      ((WebFrameWidgetBase*)webView->MainFrameImpl()->FrameWidget())
          ->DragSourceSystemDragEnded();
    }

    webView->MainFrameWidget()->HandleInputEvent(
        WebCoalescedInputEvent(theEvent));
  }
  collectedInputEvents.clear();*/

  webView->MainFrameWidget()->BeginFrame(base::TimeTicks::Now(), false);

  std::shared_ptr<cc::SkiaPaintCanvas> spc =
      std::make_shared<cc::SkiaPaintCanvas>(canvas);

  // PaintRecordBuilder builder(nullptr, nullptr, nullptr,
  //                           spc->GetPaintPreviewTracker());

  // Updating the state machine
  /* webView->MainFrameWidget()->UpdateAllLifecyclePhases(
      WebWidget::LifecycleUpdateReason::kBeginMainFrame);*/

  LocalFrameView* frame_view =
      webViewHelper->GetWebView()->MainFrameImpl()->GetFrameView();

  PropertyTreeState property_tree_state =
      frame_view->GetLayoutView()->FirstFragment().LocalBorderBoxProperties();
  {
    blink::DisableCompositingQueryAsserts disabler;
    auto root_graphics_layer =
        GetDocument().GetLayoutView()->Compositor()->PaintRootGraphicsLayer();

    // if (frame_view->GetPaintArtifactCompositor()->NeedsUpdate()) {
    // auto layout_view = GetDocument().GetLayoutView();
    // auto compositor = layout_view->Compositor();
    // compositor->UpdateIfNeededRecursive(DocumentLifecycle::LifecycleState::kInPaint);

    frame_view->UpdateAllLifecyclePhases(
        DocumentLifecycle::LifecycleUpdateReason::kBeginMainFrame);

    // just_updated = false;
    ForAllGraphicsLayers(*root_graphics_layer, [&](GraphicsLayer& layer) {
      if (layer.PaintsContentOrHitTest()) {
        auto& artifact = layer.GetPaintController().GetPaintArtifact();
        artifact.Replay(*spc, property_tree_state, IntPoint(0, 0));
        // just_updated = true;
      }
    });
  }
}

bool BNViewLayerWindow::onEvent(const ui::PlatformEvent& platformEvent) {

  std::unique_ptr<ui::Event> evt = ui::EventFromNative(platformEvent);
  if (evt == nullptr)
    return false;

  int modifiers = 0;
  if (evt->IsShiftDown())
    modifiers |= WebInputEvent::kShiftKey;
  if (evt->IsControlDown())
    modifiers |= WebInputEvent::kControlKey;
  if (evt->IsAltDown() || evt->IsAltGrDown())
    modifiers |= WebInputEvent::kAltKey;
  if (evt->IsCommandDown())
    modifiers |= WebInputEvent::kMetaKey;

  std::shared_ptr<blink::WebInputEvent> bEvent = nullptr;
  if (evt->IsMouseWheelEvent() || evt->IsScrollEvent()) {
    std::shared_ptr<blink::WebGestureEvent> bGstEvent;

    int x_offset, y_offset;
    gfx::PointF location_f;

    switch (evt->type()) {
      case ui::ET_MOUSEWHEEL:  // This one is generated for mouse wheel on
                               // Windows
      {
        ui::MouseWheelEvent* scrlEvt = evt->AsMouseWheelEvent();
        x_offset = scrlEvt->x_offset();
        y_offset = scrlEvt->y_offset();
        location_f = scrlEvt->location_f();
      } break;
      case ui::ET_SCROLL:  // This one is generated for mouse wheel on macOS
      {
        ui::ScrollEvent* scrlEvt = evt->AsScrollEvent();
        x_offset = scrlEvt->x_offset();
        y_offset = scrlEvt->y_offset();
        location_f = scrlEvt->location_f();
      } break;

      default:
        return false;
    }

    // Setting up begin+update+end sequence
    {
      // Begin
      bGstEvent = std::make_shared<blink::WebGestureEvent>(
          WebInputEvent::Type::kGestureScrollBegin, modifiers,
          base::TimeTicks());

      bGstEvent->SetPositionInWidget(location_f);
      bGstEvent->SetSourceDevice(WebGestureDevice::kSyntheticAutoscroll);
      bGstEvent->SetFrameScale(1.0);
      bGstEvent->data.scroll_begin.scrollable_area_element_id = 0;
      bGstEvent->data.scroll_begin.delta_y_hint = 0.0;
      collectedInputEvents.push_back(bGstEvent);
    }
    {
      // Update
      bGstEvent = std::make_shared<blink::WebGestureEvent>(
          WebInputEvent::Type::kGestureScrollUpdate, modifiers,
          base::TimeTicks());

      bGstEvent->SetPositionInWidget(location_f);

      bGstEvent->SetSourceDevice(WebGestureDevice::kSyntheticAutoscroll);
      bGstEvent->SetFrameScale(1.0);
      bGstEvent->data.scroll_update.inertial_phase =
          WebGestureEvent::InertialPhaseState::kMomentum;
      bGstEvent->data.scroll_update.delta_x = x_offset;
      bGstEvent->data.scroll_update.delta_y = y_offset;
      collectedInputEvents.push_back(bGstEvent);
    }
    {
      // End
      bGstEvent = std::make_shared<blink::WebGestureEvent>(
          WebInputEvent::Type::kGestureScrollEnd, modifiers, base::TimeTicks());
      bGstEvent->SetPositionInWidget(location_f);
      bGstEvent->SetSourceDevice(WebGestureDevice::kSyntheticAutoscroll);
      bGstEvent->SetFrameScale(1.0);
      collectedInputEvents.push_back(bGstEvent);
    }

    return true;

  } else if (evt->IsMouseEvent()) {     // Any mouse event except the wheel
    ui::MouseEvent* mseEvt = evt->AsMouseEvent();
    std::shared_ptr<blink::WebMouseEvent> bMseEvent;
    WebInputEvent::Type mtp;

    switch (mseEvt->type()) {
      case ui::ET_MOUSE_PRESSED:
        mtp = WebInputEvent::Type::kMouseDown;
        break;
      case ui::ET_MOUSE_RELEASED:
        mtp = WebInputEvent::Type::kMouseUp;
        break;
      case ui::ET_MOUSE_MOVED:
      case ui::ET_MOUSE_DRAGGED:
        mtp = WebInputEvent::Type::kMouseMove;
        break;
      case ui::ET_MOUSE_ENTERED:
        mtp = WebInputEvent::Type::kMouseEnter;
        break;
      case ui::ET_MOUSE_EXITED:
        mtp = WebInputEvent::Type::kMouseLeave;
        break;

      default:
        return false;
    }

    WebPointerProperties::Button button;
    switch (mseEvt->button_flags()) {
      case ui::EF_LEFT_MOUSE_BUTTON:
        button = WebPointerProperties::Button::kLeft;
        break;
      case ui::EF_MIDDLE_MOUSE_BUTTON:
        button = WebPointerProperties::Button::kMiddle;
        break;
      case ui::EF_RIGHT_MOUSE_BUTTON:
        button = WebPointerProperties::Button::kRight;
        break;
      default:
        button = WebPointerProperties::Button::kNoButton;
    }

    int click_count_param = mseEvt->GetClickCount();
    WebGestureEvent ge(WebInputEvent::Type::kUndefined, 0, base::TimeTicks());
    bMseEvent = std::make_shared<blink::WebMouseEvent>(
        mtp, std::move(ge), button, click_count_param, modifiers,
        base::TimeTicks());
    bMseEvent->SetPositionInWidget(mseEvt->location_f());

    bEvent = bMseEvent;
    collectedInputEvents.push_back(bEvent);

  } else if (evt->IsKeyEvent()) {
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

    bKbdEvent = std::make_shared<blink::WebKeyboardEvent>(mtp, modifiers,
                                                          base::TimeTicks());
    bKbdEvent->text[0] = keyEvt->GetText();
    bKbdEvent->windows_key_code = keyEvt->key_code();
    bKbdEvent->dom_key = keyEvt->GetDomKey();  // GetCharacter();

    bEvent = bKbdEvent;
    collectedInputEvents.push_back(bEvent);
    return true;
  }

  return false;
}

void BNApp::onIdle() {

  if (viewLayerWindow != nullptr) {
    viewLayerWindow->DoFrame();

    // Checking if the window is closed
    if (viewLayerWindow->isClosePending()) {
      // ... and closing it
      viewLayerWindow = nullptr;

      // The next behaviour differs in dependency to the OS tradition
      // When the last window is closed, Windows and Linux close the
      // application while macOS can leave an icon on the Dock...
      //
      // But as soon as our application doesn't support a possibility 
      // to open a window after it was closed, it is useless even on macOS
//#ifndef __APPLE__
      Quit();
//#endif

    }
  }
  
  // Processing the pending commands
  base::RunLoop run_loop;
  run_loop.RunUntilIdle();

}

void BNApp::onUserQuit() {
  bool keep_any_window = false;
  if (viewLayerWindow != nullptr) {
    if (viewLayerWindow->onUserCloseKeepWindow()) {
      keep_any_window = true;
    }
  }
  
  if (!keep_any_window) {
    // If no window could be kept, quitting the app
    Quit();
  }
}

void BNViewLayerWindow::onAttach(app_base::Window* window) {}

}  // namespace beacon::impl
