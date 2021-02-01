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
#include "third_party/blink/renderer/core/frame/visual_viewport.h"
#include "third_party/blink/renderer/core/frame/web_frame_widget_base.h"
#include "third_party/blink/renderer/core/page/focus_controller.h"
#include "third_party/blink/renderer/core/html/html_head_element.h"
#include "third_party/blink/renderer/core/html/html_collection.h"
#include "third_party/blink/renderer/platform/language.h"
#include "third_party/blink/renderer/platform/fonts/web_font_typeface_factory.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_record_builder.h"
#include "third_party/blink/renderer/platform/heap/heap.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/main_thread_scheduler_impl.h"
#include "third_party/blink/renderer/platform/scheduler/public/dummy_schedulers.h"
#include "third_party/blink/renderer/platform/geometry/int_point.h"
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


#include "base/memory/discardable_memory_allocator.h"
#include "base/task/post_task.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/thread_pool/thread_pool_impl.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/files/file_path.h"

#include "mojo/core/embedder/embedder.h"
#include "cc/paint/skia_paint_canvas.h"

#include "ui/events/blink/blink_event_util.h"
#include "ui/base/resource/resource_bundle.h"

#if defined(OS_WIN)
#include "ui/events/blink/web_input_event_builders_win.h"
#include "base/strings/utf_string_conversions.h"
#include "third_party/blink/public/web/win/web_font_rendering.h"
#endif

#include "LgApp.h"
#include "LgBlinkPlatformImpl.h"

#include "hell/SkLoadICU.h"
#include "hell/my_blink_platform_impl.h"
#include "hell/my_frame_test_helpers.h"


using namespace sk_app;
using namespace blink;

extern "C" uint8_t blink_resources_pak[]; /* binary data         */
extern "C" uint32_t blink_resources_pak_size; /* size of binary data */

LgApp::LgApp(int argc, char** argv,
                       const std::shared_ptr<PlatformData>& platformData)
    : fBackendType(Window::kRaster_BackendType),
      platformData(platformData),
      paintTime(std::chrono::high_resolution_clock::now()) {

  exit_manager = std::make_shared<base::AtExitManager>();

  base::CommandLine::Init(argc, argv);
  base::CommandLine::StringVector parsedArgs =
      base::CommandLine::ForCurrentProcess()->GetArgs();

  if (parsedArgs.size() > 0) {
      // Processing toe command line
  }

  if (!SkLoadICU()) {
  	std::cerr << "Can't load ICU4C data" << std::endl;
  }

  mojo::core::Init();

  if (!ui::ResourceBundle::HasSharedInstance()) {
    ui::ResourceBundle::InitSharedInstanceWithLocale(
        "en-US", nullptr, ui::ResourceBundle::DO_NOT_LOAD_COMMON_RESOURCES);

    // Adding the blink_resources.pak embeded into the binary
    base::StringPiece blink_pak_memory((char*) blink_resources_pak,
                                       blink_resources_pak_size);

    ui::ResourceBundle::GetSharedInstance().AddDataPackFromBuffer(
        blink_pak_memory, ui::SCALE_FACTOR_100P);     
  }

       
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



  binder_map = std::make_unique<mojo::BinderMap>();

  // Creating a discardable memory allocator
  discardableSharedMemoryManager =
      std::make_shared<discardable_memory::DiscardableSharedMemoryManager>();
  base::DiscardableMemoryAllocator::SetInstance(discardableSharedMemoryManager.get());

  SkGraphics::Init();
  
  fWindow = Window::CreateNativeWindow(platformData);
  fWindow->setRequestedDisplayParams(DisplayParams());

  platform = std::make_unique<LgBlinkPlatformImpl>(my_web_thread_sched->DefaultTaskRunner(),
                                                   my_web_thread_sched->DefaultTaskRunner(),
                                                   fWindow);
        
  blink::Initialize(platform.get(), binder_map.get(),
                    /*scheduler::WebThreadScheduler * main_thread_scheduler*/
                    my_web_thread_sched.get());

  blink::InitializePlatformLanguage();

  // Tuin
  /*blink::WebFontRenderStyle::SetAutoHint(true);
  blink::WebFontRenderStyle::SetAntiAlias(true);
  blink::WebFontRenderStyle::SetSubpixelRendering(true);
  blink::WebFontRenderStyle::SetSubpixelPositioning(true);*/
 
  backend = std::make_shared<SDK::Backend>();

  webViewHelper =
      std::make_shared<blink::my_frame_test_helpers::WebViewHelper>();
  wfc = std::make_shared<blink::my_frame_test_helpers::TestWebFrameClient>(
      backend);
  wfc->SetScheduler(my_web_thread_sched);

  wvc = std::make_shared<blink::my_frame_test_helpers::TestWebViewClient>(webViewHelper);
  wwc = std::make_shared<blink::my_frame_test_helpers::TestWebWidgetClient>(
      new my_frame_test_helpers::StubLayerTreeViewDelegate(),
      my_web_thread_sched->DefaultTaskRunner(),  // mainTaskRunner,
      composeTaskRunner,
      my_web_thread_sched.get());

  webView = webViewHelper->InitializeAndLoad("mem://index.html", wfc.get(), wvc.get(), wwc.get());


  // register callbacks
  fWindow->pushLayer(this);
  fWindow->attach(fBackendType);

  // Setting the WebView scaling factor according to the screen DPI
  float scaleFactor = fWindow->getScale();
  webView->SetZoomFactorOverride(scaleFactor);



  // Setting the main view initially focused
  webViewHelper->SetFocused();
  
  // Setting caret visible
  blink::WebLocalFrameImpl& frm = *webView->MainFrameImpl();
  frm.SetCaretVisible(true);
        
  // Setting the initial size
  IntSize page_size(fWindow->width(), fWindow->height());
  webView->Resize(WebSize(page_size));

}

LgApp::~LgApp() {
  fWindow->detach();
  delete fWindow;

  // Resetting the WebViewHelper
  webViewHelper->Reset();
  webViewHelper = nullptr;

  // Shutting down the thread scheduler
  my_web_thread_sched->Shutdown();
  my_web_thread_sched = nullptr;
}

void LgApp::updateTitle() {
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

void LgApp::onBackendCreated() {
  std::cout << "LgApp::onBackendCreated" << std::endl;
  this->updateTitle();
  fWindow->show();
  fWindow->inval();
}

Document& LgApp::GetDocument() {
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

void LgApp::UpdateBackend() {
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
  } else
  {
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
  }

  if (fBackendType != newBackendType) {
    std::cout << "LgApp::UpdateBackend: updating backend" << std::endl;
    fBackendType = newBackendType;
    fWindow->detach();
    fWindow->attach(fBackendType);
    updateTitle();
  }
}

void LgApp::onResize(int width, int height) {
  UpdateBackend();

  int kPageWidth = width;
  int kPageHeight = height;
  IntSize page_size(kPageWidth, kPageHeight);

  webView->Resize(WebSize(page_size));
  fWindow->inval();
}

void LgApp::onBeginResizing() {
  std::cout << "LgApp::onBeginResizing" << std::endl;
  resizing = true;
  UpdateBackend();
}

void LgApp::onEndResizing() {
  std::cout << "LgApp::onEndResizing" << std::endl;
  resizing = false;
  UpdateBackend();
}

void LgApp::onPaint(SkSurface* surface) {
  auto* canvas = surface->getCanvas();

  canvas->save();

  updateTitle();

  // Updating fonts and colors.
  // TODO Don't run this code on every frame. Put it to a system update event
  // instead
  UpdatePlatformFontsAndColors();

  Paint(canvas);

  canvas->restore();
}

void LgApp::UpdatePlatformFontsAndColors() {
  // Updating fonts
#if defined(OS_WIN)
  blink::FontCache::SetAntialiasedTextEnabled(true);

  if (!this->fWindow->GetDefaultUIFont(defaultUIFont)) {
    // Fallback. I don't know which disaster should happen to Windows
    // that makes it fail to determine the system metrics, but we are prepared
    // here.
    defaultUIFont.typeface = "Arial";
    defaultUIFont.heightPt = 10;
  }

  // On Windows blink determines the UI font as the default Menu font (no idea
  // why not Message font). So, if we want "system-ui" typeface to work
  // properly, we need to set it here
  std::wstring wsTypeface;
  base::UTF8ToWide(defaultUIFont.typeface.c_str(),
                   defaultUIFont.typeface.size(), &wsTypeface);
  blink::WebFontRendering::SetMenuFontMetrics(wsTypeface.c_str(),
                                              defaultUIFont.heightPt);
#else
  // On other platforms we just load the default UI font
  this->fWindow->GetDefaultUIFont(defaultUIFont);
#endif

  // Updating colors
  PlatformColors pc = this->fWindow->GetPlatformColors();
  blink::SetFocusRingColor(pc.GetFocusRingColor(this->fWindow->IsActive()));
  blink::SetSelectionColors(
      pc.GetSelectionBackgroundColor(true), pc.GetSelectionTextColor(true),
      pc.GetSelectionBackgroundColor(false), pc.GetSelectionTextColor(false));
  blink::ColorSchemeChanged();
}

void LgApp::Paint(SkCanvas* canvas) {
  
  GetDocument().GetPage()->GetFocusController().SetActive(
      this->fWindow->IsActive());

  // Handling events
  LocalFrameView* frame_view =
      webViewHelper->GetWebView()->MainFrameImpl()->GetFrameView();

  webViewHelper->GetWebWidgetClient()->HandleScrollEvents(
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
  collectedInputEvents.clear();

  // Updating the state machine
  webView->MainFrameWidget()->UpdateAllLifecyclePhases(
      WebWidget::LifecycleUpdateReason::kBeginMainFrame);

  webView->MainFrameWidget()->BeginFrame(base::TimeTicks::Now(), false);

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

bool LgApp::onMouse(const ui::PlatformEvent& platformEvent,
                         int, int, skui::InputState, skui::ModifierKey) {
  std::unique_ptr<ui::Event> evt = ui::EventFromNative(platformEvent);
  if (evt == nullptr)
    return false;

  std::shared_ptr<blink::WebInputEvent> bEvent = nullptr;
  if (evt->IsMouseEvent()) {
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

    int modifiers = 0;
    if (evt->IsShiftDown())
      modifiers |= WebInputEvent::kShiftKey;
    if (evt->IsControlDown())
      modifiers |= WebInputEvent::kControlKey;
    if (evt->IsAltDown() || evt->IsAltGrDown())
      modifiers |= WebInputEvent::kAltKey;
    if (evt->IsCommandDown())
      modifiers |= WebInputEvent::kMetaKey;

    int click_count_param = mseEvt->GetClickCount();
    WebGestureEvent ge(WebInputEvent::Type::kUndefined, 0, base::TimeTicks());
    bMseEvent = std::make_shared<blink::WebMouseEvent>(
        mtp, std::move(ge), button, click_count_param, modifiers, base::TimeTicks());
    bMseEvent->SetPositionInWidget(mseEvt->location_f());

    bEvent = bMseEvent;
  }

  if (evt->IsMouseEvent()) {
    collectedInputEvents.push_back(bEvent);
    return true;
  }
  return false;
}

bool LgApp::onMouseWheel(const ui::PlatformEvent& platformEvent,
    float delta,
    skui::ModifierKey modKey) {

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


    if (evt->IsMouseWheelEvent() || evt->IsScrollEvent()) {
      std::shared_ptr<blink::WebGestureEvent> bGstEvent;

      int x_offset, y_offset;
      gfx::PointF location_f;

      switch (evt->type()) {
        case ui::ET_MOUSEWHEEL:     // This one is generated for mouse wheel on Windows
        {
          ui::MouseWheelEvent* scrlEvt = evt->AsMouseWheelEvent();
          x_offset = scrlEvt->x_offset();
          y_offset = scrlEvt->y_offset();
          location_f = scrlEvt->location_f();
        }
        break;
        case ui::ET_SCROLL:         // This one is generated for mouse wheel on macOS
        {
          ui::ScrollEvent* scrlEvt = evt->AsScrollEvent();
          x_offset = scrlEvt->x_offset();
          y_offset = scrlEvt->y_offset();
          location_f = scrlEvt->location_f();
        }
        break;

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
            WebInputEvent::Type::kGestureScrollEnd, modifiers,
            base::TimeTicks());
        bGstEvent->SetPositionInWidget(location_f);
        bGstEvent->SetSourceDevice(WebGestureDevice::kSyntheticAutoscroll);
        bGstEvent->SetFrameScale(1.0);
        collectedInputEvents.push_back(bGstEvent);
      }

      return true;
    }

    return false;

}

bool LgApp::onKey(const ui::PlatformEvent& platformEvent,
                       uint64_t key,
                       skui::InputState inState,
                       skui::ModifierKey modKey) {
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
        std::make_shared<blink::WebKeyboardEvent>(mtp, modifiers, base::TimeTicks());
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

bool LgApp::onChar(const ui::PlatformEvent& platformEvent,
                        SkUnichar c,
                        skui::ModifierKey modifiers) {
  return false;
}

void LgApp::onIdle() {
  UpdateBackend();

  // Processing the pending commands
  base::RunLoop run_loop;
  run_loop.RunUntilIdle();

  // Just re-paint continously
  int FPS = this->fWindow->IsActive() ? 60 : 30;

  auto now = std::chrono::high_resolution_clock::now();
  auto span =
      std::chrono::duration<double, std::ratio<1>>(now - paintTime).count();
  if (span > 1.0 / FPS) {
    //std::cout << "Span: " << span << std::endl;
    fWindow->inval();
    paintTime = now;
  }
}

void LgApp::onAttach(sk_app::Window* window) {}
