/*
 * Copyright 2019 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/core/SkUtils.h"
#include "app_base/Application.h"
#include "app_base/mac/WindowContextFactory_mac.h"
#include "app_base/mac/Window_mac.h"
#include "tools/skui/ModifierKey.h"

@interface WindowDelegate : NSObject<NSWindowDelegate>

- (WindowDelegate*)initWithWindow:(app_base::Window_mac*)initWindow;
- (void)windowDidResize:(NSNotification *)notification;

@end

@interface MainView : NSView

- (MainView*)initWithWindow:(app_base::Window_mac*)initWindow;

@end

///////////////////////////////////////////////////////////////////////////////

using app_base::Window;

namespace app_base {
  
  SkTDynamicHash<Window_mac, NSInteger> Window_mac::gWindowMap;
  
  Window* Window::CreateNativeWindow(const std::shared_ptr<PlatformData>& platformData) {
    Window_mac* window = new Window_mac();
    if (!window->initWindow()) {
      delete window;
      return nullptr;
    }
    
    return window;
  }
  
  bool Window_mac::initWindow() {
    // we already have a window
    if (fWindow) {
      return true;
    }
    
    // Create a delegate to track certain events
    WindowDelegate* delegate = [[WindowDelegate alloc] initWithWindow:this];
    if (nil == delegate) {
      return false;
    }
    
    // Create Cocoa window
    constexpr int initialWidth = 1280;
    constexpr int initialHeight = 960;
    NSRect windowRect = NSMakeRect(100, 100, initialWidth, initialHeight);
    
    NSUInteger windowStyle = (NSTitledWindowMask | NSClosableWindowMask | NSResizableWindowMask |
                              NSMiniaturizableWindowMask);
    
    fWindow = [[NSWindow alloc] initWithContentRect:windowRect styleMask:windowStyle
                                            backing:NSBackingStoreBuffered defer:NO];
    if (nil == fWindow) {
      [delegate release];
      return false;
    }
    
    // create view
    MainView* view = [[MainView alloc] initWithWindow:this] ;
    if (nil == view) {
      [fWindow release];
      [delegate release];
      return false;
    }
    
    [fWindow setContentView:view];
    [fWindow makeFirstResponder:view];
    [fWindow setDelegate:delegate];
    [fWindow setAcceptsMouseMovedEvents:YES];
    [fWindow setRestorable:NO];
    
    // Should be retained by window now
    [view release];
    
    fWindowNumber = fWindow.windowNumber;
    gWindowMap.add(this);
    
    return true;
  }
  
  void Window_mac::closeWindow() {
    if (nil != fWindow) {
      gWindowMap.remove(fWindowNumber);
      if (app_base::Window_mac::gWindowMap.count() < 1) {
        [NSApp terminate:fWindow];
      }
      [fWindow close];
      fWindow = nil;
    }
  }
  
  void Window_mac::setTitle(const char* title) {
    NSString *titleString = [NSString stringWithCString:title encoding:NSUTF8StringEncoding];
    [fWindow setTitle:titleString];
  }
  
  void Window_mac::show() {
    onResize(width(), height());
    inval();
    
    [fWindow orderFront:nil];
    
    [NSApp activateIgnoringOtherApps:YES];
    [fWindow makeKeyAndOrderFront:NSApp];
  }
  
  bool Window_mac::attach(BackendType attachType) {
    this->initWindow();
    
    window_context_factory::MacWindowInfo info;
    info.fMainView = [fWindow contentView];
    switch (attachType) {
      case kRaster_BackendType:
        fWindowContext = MakeRasterForMac(info, fRequestedDisplayParams);
        break;
#ifdef SK_DAWN
      case kDawn_BackendType:
        fWindowContext = MakeDawnMTLForMac(info, fRequestedDisplayParams);
        break;
#endif
#ifdef SK_VULKAN
      case kVulkan_BackendType:
        fWindowContext = MakeVulkanForMac(info, fRequestedDisplayParams);
        break;
#endif
#ifdef SK_METAL
      case kMetal_BackendType:
        fWindowContext = MakeMetalForMac(info, fRequestedDisplayParams);
        break;
#endif
      case kNativeGL_BackendType:
      default:
        fWindowContext = MakeGLForMac(info, fRequestedDisplayParams);
        break;
    }
    this->onBackendCreated();
    
    return (SkToBool(fWindowContext));
  }
  
  void Window_mac::PaintWindows() {
    SkTDynamicHash<Window_mac, NSInteger>::Iter iter(&gWindowMap);
    while (!iter.done()) {
      if ((*iter).fIsContentInvalidated) {
        (*iter).onPaint();
      }
      ++iter;
    }
  }
  
  bool Window_mac::GetDefaultUIFont(PlatformFont& result) {
    NSFont* defaultFont = [NSFont systemFontOfSize: 0];
    result.typeface = std::string([defaultFont.familyName UTF8String]);
    result.heightPt = [NSFont smallSystemFontSize];  //int(defaultFont.pointSize);
    //CGFloat scaleFactor = [[NSScreen mainScreen] backingScaleFactor];
    return true;
  }
  
  static SkColor NSColorToSkColor(NSColor* acc) {
    const CGFloat* rgb = CGColorGetComponents(acc.CGColor);
    return SkColorSetRGB((uint8_t)(rgb[0] * 255), (uint8_t)(rgb[1] * 255), (uint8_t)(rgb[2] * 255));
  }
  
  PlatformColors Window_mac::GetPlatformColors() const {
    PlatformColors pc;
    
    pc.selectionBackgroundColorActive = NSColorToSkColor(NSColor.selectedTextBackgroundColor);
    
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunguarded-availability"
    
    if ([NSColor respondsToSelector:@selector(unemphasizedSelectedTextBackgroundColor)]) {
      pc.selectionBackgroundColorInactive = NSColorToSkColor(NSColor.unemphasizedSelectedTextBackgroundColor);  // 10.14 or newer
    } else {
      pc.selectionBackgroundColorInactive = NSColorToSkColor(NSColor.windowBackgroundColor);
    }
    
    pc.selectionTextColorActive = NSColorToSkColor(NSColor.selectedTextColor);
    
    if ([NSColor respondsToSelector:@selector(unemphasizedSelectedTextColor)]) {
      pc.selectionTextColorInactive = NSColorToSkColor(NSColor.unemphasizedSelectedTextColor);  // 10.14 or newer
    } else {
      pc.selectionTextColorInactive = NSColorToSkColor(NSColor.selectedTextColor);
    }
#pragma clang diagnostic pop
    
    pc.focusRingColorActive = NSColorToSkColor(NSColor.keyboardFocusIndicatorColor);
    pc.focusRingColorInactive = NSColorToSkColor(NSColor.controlColor);
    return pc;
  }
  
  bool Window_mac::IsActive() const {
    return [fWindow isKeyWindow];
  }
  
}   // namespace app_base

///////////////////////////////////////////////////////////////////////////////

@implementation WindowDelegate {
  app_base::Window_mac* fWindow;
}

- (WindowDelegate*)initWithWindow:(app_base::Window_mac *)initWindow {
  fWindow = initWindow;
  
  // Setting the window size initially (
  //[self windowDidResize:nullptr];
  
  return self;
}

- (void)windowDidResize:(NSNotification *)notification {
  const NSRect mainRect = [fWindow->window().contentView bounds];
  
  fWindow->onResize(mainRect.size.width, mainRect.size.height);
  fWindow->inval();
}

- (void)windowWillStartLiveResize:(NSNotification *)notification {
  fWindow->onBeginResizing();
}

- (void)windowDidEndLiveResize:(NSNotification *)notification {
  fWindow->onEndResizing();
}

- (BOOL)windowShouldClose:(NSWindow*)sender {
  if (!fWindow->onUserCloseKeepWindow()) {
    fWindow->Close();
  }
  
  //fWindow->closeWindow();
  
  return FALSE;
}

@end

///////////////////////////////////////////////////////////////////////////////

static skui::Key get_key(unsigned short vk) {
  // This will work with an ANSI QWERTY keyboard.
  // Something more robust would be needed to support alternate keyboards.
  static const struct {
    unsigned short fVK;
    skui::Key    fKey;
  } gPair[] = {
    { 0x33, skui::Key::kBack },
    { 0x24, skui::Key::kOK },
    { 0x7E, skui::Key::kUp },
    { 0x7D, skui::Key::kDown },
    { 0x7B, skui::Key::kLeft },
    { 0x7C, skui::Key::kRight },
    { 0x30, skui::Key::kTab },
    { 0x74, skui::Key::kPageUp },
    { 0x79, skui::Key::kPageDown },
    { 0x73, skui::Key::kHome },
    { 0x77, skui::Key::kEnd },
    { 0x75, skui::Key::kDelete },
    { 0x35, skui::Key::kEscape },
    { 0x38, skui::Key::kShift },
    { 0x3C, skui::Key::kShift },
    { 0x3B, skui::Key::kCtrl },
    { 0x3E, skui::Key::kCtrl },
    { 0x3A, skui::Key::kOption },
    { 0x3D, skui::Key::kOption },
    { 0x00, skui::Key::kA },
    { 0x08, skui::Key::kC },
    { 0x09, skui::Key::kV },
    { 0x07, skui::Key::kX },
    { 0x10, skui::Key::kY },
    { 0x06, skui::Key::kZ },
  };
  for (size_t i = 0; i < SK_ARRAY_COUNT(gPair); i++) {
    if (gPair[i].fVK == vk) {
      return gPair[i].fKey;
    }
  }
  
  return skui::Key::kNONE;
}

static skui::ModifierKey get_modifiers(const NSEvent* event) {
  NSUInteger modifierFlags = [event modifierFlags];
  skui::ModifierKey modifiers = skui::ModifierKey::kNone;
  
  if (modifierFlags & NSEventModifierFlagCommand) {
    modifiers |= skui::ModifierKey::kCommand;
  }
  if (modifierFlags & NSEventModifierFlagShift) {
    modifiers |= skui::ModifierKey::kShift;
  }
  if (modifierFlags & NSEventModifierFlagControl) {
    modifiers |= skui::ModifierKey::kControl;
  }
  if (modifierFlags & NSEventModifierFlagOption) {
    modifiers |= skui::ModifierKey::kOption;
  }
  
  if ((NSKeyDown == [event type] || NSKeyUp == [event type]) &&
      NO == [event isARepeat]) {
    modifiers |= skui::ModifierKey::kFirstPress;
  }
  
  return modifiers;
}

@implementation MainView {
  app_base::Window_mac* fWindow;
  // A TrackingArea prevents us from capturing events outside the view
  NSTrackingArea* fTrackingArea;
}

- (MainView*)initWithWindow:(app_base::Window_mac *)initWindow {
  self = [super init];
  
  fWindow = initWindow;
  fTrackingArea = nil;
  
  [self updateTrackingAreas];
  
  return self;
}

- (void)dealloc
{
  [fTrackingArea release];
  [super dealloc];
}

- (BOOL)isOpaque {
  return YES;
}

- (BOOL)canBecomeKeyView {
  return YES;
}

- (BOOL)acceptsFirstResponder {
  return YES;
}

- (void)updateTrackingAreas {
  if (fTrackingArea != nil) {
    [self removeTrackingArea:fTrackingArea];
    [fTrackingArea release];
  }
  
  const NSTrackingAreaOptions options = NSTrackingMouseEnteredAndExited |
  NSTrackingActiveInKeyWindow |
  NSTrackingEnabledDuringMouseDrag |
  NSTrackingCursorUpdate |
  NSTrackingInVisibleRect |
  NSTrackingAssumeInside;
  
  fTrackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds]
                                               options:options
                                                 owner:self
                                              userInfo:nil];
  
  [self addTrackingArea:fTrackingArea];
  [super updateTrackingAreas];
}

- (void)keyDown:(NSEvent *)event {
  skui::Key key = get_key([event keyCode]);
  /*if (key != skui::Key::kNONE)*/ {
    if (!fWindow->onKey(event, [event keyCode], skui::InputState::kDown, get_modifiers(event))) {
      if (skui::Key::kEscape == key) {
        [NSApp terminate:fWindow->window()];
      }
    }
  }
  
  NSString* characters = [event charactersIgnoringModifiers];
  NSUInteger len = [characters length];
  if (len > 0) {
    unichar* charBuffer = new unichar[len+1];
    [characters getCharacters:charBuffer range:NSMakeRange(0, len)];
    for (NSUInteger i = 0; i < len; ++i) {
      (void) fWindow->onChar(event, (SkUnichar) charBuffer[i], get_modifiers(event));
    }
    delete [] charBuffer;
  }
}

- (void)keyUp:(NSEvent *)event {
  skui::Key key = get_key([event keyCode]);
  /*if (key != skui::Key::kNONE)*/ {
    (void) fWindow->onKey(event, [event keyCode], skui::InputState::kUp, get_modifiers(event));
  }
}

- (void)mouseDown:(NSEvent *)event {
  const NSPoint pos = [event locationInWindow];
  const NSRect rect = [fWindow->window().contentView frame];
  fWindow->onMouse(event, pos.x, rect.size.height - pos.y, skui::InputState::kDown,
                   get_modifiers(event));
}

- (void)mouseUp:(NSEvent *)event {
  const NSPoint pos = [event locationInWindow];
  const NSRect rect = [fWindow->window().contentView frame];
  fWindow->onMouse(event, pos.x, rect.size.height - pos.y, skui::InputState::kUp,
                   get_modifiers(event));
}

- (void)mouseDragged:(NSEvent *)event {
  [self mouseMoved:event];
}

- (void)mouseMoved:(NSEvent *)event {
  const NSPoint pos = [event locationInWindow];
  const NSRect rect = [fWindow->window().contentView frame];
  fWindow->onMouse(event, pos.x, rect.size.height - pos.y, skui::InputState::kMove,
                   get_modifiers(event));
}

- (void)scrollWheel:(NSEvent *)event {
  // TODO: support hasPreciseScrollingDeltas?
  fWindow->onMouseWheel(event, [event scrollingDeltaY], get_modifiers(event));
}

- (void)drawRect:(NSRect)rect {
  fWindow->onPaint();
}

@end
