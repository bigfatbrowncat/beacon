/*
 * Copyright 2019 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#import <Cocoa/Cocoa.h>

#include "app_base/Application.h"
#include "app_base/mac/Window_mac.h"

@interface AppDelegate : NSObject<NSApplicationDelegate, NSWindowDelegate>

@property (nonatomic, assign) BOOL done;
@property (nonatomic, assign) app_base::Application* app;

@end

@implementation AppDelegate : NSObject

@synthesize done = _done;
@synthesize app = _app;

- (id)init:(app_base::Application *)app {
  self = [super init];
  _done = FALSE;
  _app = app;
  return self;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
  //_done = TRUE;
  _app->onUserQuit();
  return NSTerminateCancel;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
  [NSApp stop:nil];
}

@end

///////////////////////////////////////////////////////////////////////////////////////////

using app_base::Application;
using app_base::Window_mac;

int main(int argc, char * argv[]) {
#if MAC_OS_X_VERSION_MAX_ALLOWED < 1070
  // we only run on systems that support at least Core Profile 3.2
  return EXIT_FAILURE;
#endif
  
  NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
  [NSApplication sharedApplication];
  
  [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
  
  //Create the application menu.
  NSMenu* menuBar=[[NSMenu alloc] initWithTitle:@"AMainMenu"];
  [NSApp setMainMenu:menuBar];
  
  NSMenuItem* item;
  NSMenu* subMenu;
  
  item=[[NSMenuItem alloc] initWithTitle:@"Apple" action:NULL keyEquivalent:@""];
  [menuBar addItem:item];
  subMenu=[[NSMenu alloc] initWithTitle:@"Apple"];
  [menuBar setSubmenu:subMenu forItem:item];
  [item release];
  item=[[NSMenuItem alloc] initWithTitle:@"Quit" action:@selector(terminate:) keyEquivalent:@"q"];
  [subMenu addItem:item];
  [item release];
  [subMenu release];
  
  std::shared_ptr<app_base::PlatformData> platformData =
  std::make_shared<app_base::PlatformData>();
  
  Application* app = Application::Create(argc, argv, platformData);
  
  // Set AppDelegate to catch certain global events
  AppDelegate* appDelegate = [[AppDelegate alloc] init:app];
  [NSApp setDelegate:appDelegate];
  
  //Window_mac::PaintWindows();
  
  // This will run until the application finishes launching, then lets us take over
  [NSApp run];
  
  // Now we process the events
  while (![appDelegate app]->isQuitPending()) {
    NSEvent* event;
    do {
      event = [NSApp nextEventMatchingMask:NSAnyEventMask
                                 untilDate:[NSDate distantPast]
                                    inMode:NSDefaultRunLoopMode
                                   dequeue:YES];
      [NSApp sendEvent:event];
    } while (event != nil);
    
    [pool drain];
    pool = [[NSAutoreleasePool alloc] init];
    
    // We are running onIdle before PaintWindows because the first
    // frame should be rendered by the window before the window is
    // shown to the user, or the user will experience flicker
    app->onIdle();
    
    // Rather than depending on a Mac event to drive this, we treat our window
    // invalidation flag as a separate event stream. Window::onPaint() will clear
    // the invalidation flag, effectively removing it from the stream.
    Window_mac::PaintWindows();
    
  }
  
  delete app;
  
  [NSApp setDelegate:nil];
  [appDelegate release];
  
  [menuBar release];
  [pool release];
  
  return EXIT_SUCCESS;
}
