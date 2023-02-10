#include "third_party/blink/public/web/win/web_font_rendering.h"
#include "third_party/blink/renderer/platform/fonts/font_cache.h"

#include "third_party/blink/renderer/core/exported/web_render_theme.cc"

#include "base/strings/utf_string_conversions.h"

#include "app_base/Window.h"

#include "third_party/skia/include/core/SkCanvas.h"

#include "BNLayer.h"

#include <chrono>
#include <thread>
#include <iostream>

namespace beacon::impl {

void BNLayer::updateTitle() {
  if (!isWindowConnected() /*|| fWindow->sampleCount() <= 1*/) {
    return;
  }

  std::string title = getTitle();

  SkString skTitle(title.c_str());
  skTitle.append(" [");
  skTitle.append(app_base::Window::kRaster_BackendType == getBackendType()
                     ? "Raster"
                     : "OpenGL");
  skTitle.append("]");

  setWindowTitle(skTitle.c_str());
}

void BNLayer::UpdatePlatformFontsAndColors() {
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
  app_base::PlatformColors pc = this->fWindow->GetPlatformColors();
  blink::SetFocusRingColor(pc.GetFocusRingColor(this->fWindow->IsActive()));
  blink::SetSelectionColors(
      pc.GetSelectionBackgroundColor(true), pc.GetSelectionTextColor(true),
      pc.GetSelectionBackgroundColor(false), pc.GetSelectionTextColor(false));
  blink::ColorSchemeChanged();
}

//void BNLayer::UpdateBackend(bool forceFallback) {
//  bool fallback = false;//forceFallback;
//
//  if (fWindow != nullptr) {
//
//#ifdef __APPLE__
//    // On Apple the "raster" is the same OpenGl renderer
//    // but switching contexts takes time and makes a black flicker
//    // so avoiding it
//    fallback = true;
//#endif
//
//    /* if (fWindow->width() * fWindow->height() <= 2560 * 1440 && resizing) {
//      // Checking if we need a fallback to Raster renderer.
//      // Fallback is effective for small screens and weak videochips
//
//      // Also, OpenGL context slows down the resizing process.
//      // So we are changing the backend to software raster during resizing
//      fallback = true;
//    }*/
//
//    auto newBackendType =  fallback ? app_base::Window::kRaster_BackendType
//                                    : app_base::Window::kNativeGL_BackendType;
//
//    std::chrono::steady_clock::time_point curTime =
//        std::chrono::steady_clock::now();
//
//    if (forceFallback) {
//      // If we are forcing the fallback, we don't
//      // let the context to upgrade immediately
//      lastBackendInitFailedAttempt = curTime;
//    }
//
//    if (fBackendType != app_base::Window::kRaster_BackendType &&
//      fWindow->getGrContext() == nullptr) {
//      // If we attempted to initialize GL before, but failed,
//      // then falling back to raster
//      newBackendType = app_base::Window::kRaster_BackendType;
//    }
//
//    bool enoughTimePassed = std::chrono::duration_cast<std::chrono::milliseconds>(
//                                curTime - lastBackendInitFailedAttempt)
//                                .count() > 1000;
//
//    bool enoughTimePassedSinceSizeChange =
//        std::chrono::duration_cast<std::chrono::milliseconds>(curTime -
//                                                              lastSizeChange)
//            .count() > 1000;
//
//    if (enoughTimePassedSinceSizeChange) {
//      // If long time passed since the last size shange,
//      // we are assuming that the resizing has finished
//      lastSizeChange = curTime;
//      resizing = false;
//    }
//
//    if (fBackendType != newBackendType) {
//      if (newBackendType == app_base::Window::kRaster_BackendType ||
//          enoughTimePassed) {
//        std::cout << "BNApp::UpdateBackend: updating backend" << std::endl;
//        fBackendType = newBackendType;
//
//        fWindow->detach();
//
//        // If we are switching to the raster fallback mode
//        // or enough time has passed since the previous context
//        // switching failure, let's try to switch the context
//
//        if (!fWindow->attach(fBackendType)) {
//          // Oops, we've failed...
//          // Let's record the last failure time
//          lastBackendInitFailedAttempt = curTime;
//        }
//        updateTitle();
//      }
//    }
//  }
//}

void BNLayer::onPaint(SkSurface* surface) {
  auto* canvas = surface->getCanvas();

  canvas->save();

  Paint(canvas);

  canvas->restore();
}

void BNLayer::ShowWindow() {
  // TODO Make these calls at proper places
  updateTitle();

  // fWindow->inval();
  fWindow->show();
}

void BNLayer::DoFrame() {

  // Just re-paint continuously
  //int FPS = this->isWindowActive() ? 60 : 30;

  bool needsRepaint = UpdateViewIfNeededAndBeginFrame();

  if (needsRepaint /* || just_updated*/) {
    if (fWindow != nullptr) {
      fWindow->inval();
    }
  } else {
    //std::this_thread::sleep_for(std::chrono::milliseconds(1000 / FPS));
  }

}

}  // namespace beacon::impl
