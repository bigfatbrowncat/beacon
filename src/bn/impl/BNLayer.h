#pragma once

#include "ui/events/platform_event.h"

#include "third_party/blink/renderer/core/dom/document.h"

#include "third_party/skia/include/core/SkSurface.h"

#include "app_base/Window.h"

namespace beacon::impl {

class BNLayer : private app_base::Window::Layer {
 public:
  BNLayer(app_base::Window::BackendType BackendType) : fBackendType(BackendType)
  { }

  virtual ~BNLayer() {
    assert(fWindow != nullptr);
    fWindow->detach();
    delete fWindow;
  }

  void onAttach(app_base::Window* window) override { 
    fWindow = window;
  }

  bool onMouse(const ui::PlatformEvent& platformEvent,
               int x, int y,
               skui::InputState, skui::ModifierKey) override {
    return onEvent(platformEvent);
  }
  bool onMouseWheel(const ui::PlatformEvent& platformEvent,
      float delta, skui::ModifierKey) override {
    return onEvent(platformEvent);
  }
  bool onKey(const ui::PlatformEvent& platformEvent,
      uint64_t, skui::InputState, skui::ModifierKey) override {
    return onEvent(platformEvent);
  }
  bool onChar(const ui::PlatformEvent& platformEvent,
      SkUnichar c, skui::ModifierKey modifiers) override {
    return onEvent(platformEvent);
  }

  void onBackendCreated() override {
    this->updateTitle();
    fWindow->show();
    fWindow->inval();
  }

  void onResize(int width, int height) override {
    if (width != oldWidth || height != oldHeight) {
      resizing = true;
      oldWidth = width; oldHeight = height;

      std::chrono::steady_clock::time_point curTime =
          std::chrono::steady_clock::now();
      lastSizeChange = curTime;
    }
    fWindow->inval();
  }

  void onBeginResizing() override {
    // Careful! This method is currently only called on Windows
    resizing = true;
    UpdateBackend(true);
  }

  void onEndResizing() override {
    // Careful! This method is currently only called on Windows
    resizing = false;
    UpdateBackend(true);
  }

  bool onUserCloseKeepWindow() override { 
    // Doing nothing, just requesting the closure
    //fWindow->Close();
    return false;
  }

  // To be defined in the implementation
  virtual std::string getTitle() = 0;
  virtual bool UpdateViewIfNeededAndBeginFrame() = 0;
  virtual bool onEvent(const ui::PlatformEvent& platformEvent) = 0;

  bool isClosePending() { return fWindow->isClosePending(); }

  void DoFrame();

 protected:
  virtual void Paint(SkCanvas* canvas) = 0;

  void connectWindow(app_base::Window* window) {
    fWindow = window;
    window->pushLayer(this);
    window->attach(fBackendType);
  }
  app_base::Window::BackendType getBackendType() {
    return fBackendType;
  }
  void setWindowTitle(const std::string& title) {
    fWindow->setTitle(title.c_str());
  }
  bool isWindowConnected() {
    return fWindow != nullptr;
  }
  bool isWindowActive() { 
    assert(isWindowConnected());
    return fWindow->IsActive();
  }

  void onPaint(SkSurface* surface) override;

 private:
  int oldWidth, oldHeight;  

  void updateTitle();
  void UpdateBackend(bool forceFallback);
  void UpdatePlatformFontsAndColors();

  app_base::Window* fWindow = nullptr;
  app_base::Window::BackendType fBackendType;

  bool resizing = false;

  app_base::PlatformFont defaultUIFont;
  std::chrono::steady_clock::time_point lastBackendInitFailedAttempt =
      std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point lastSizeChange =
      std::chrono::steady_clock::now();
};

}
