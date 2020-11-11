/*
* Copyright 2016 Google Inc.
*
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*/

#ifndef Window_win_DEFINED
#define Window_win_DEFINED

#include "app_base/Window.h"

#include <windows.h>

namespace sk_app {

class Window_win : public Window {
public:
    Window_win(const std::shared_ptr<PlatformData>& platformData) 
        : Window(), platformData(platformData) {
    }
    
    ~Window_win() override;

    bool init();

    void setTitle(const char*) override;
    void show() override;

    bool attach(BackendType) override;

    void onInval() override;

    void setRequestedDisplayParams(const DisplayParams&, bool allowReattach) override;
    PlatformData& getPlatformData() { return *platformData; }

    float getScale() override;


    bool GetDefaultUIFont(PlatformFont& result) override;
    SkColor GetFocusRingColor() const override;

   private:
    int GetFontSize(const LOGFONT& font);

    void closeWindow();

    std::shared_ptr<PlatformData> platformData;
    HWND fHWnd;
    BackendType fBackend;

    typedef Window INHERITED;
};

}   // namespace sk_app

#endif
