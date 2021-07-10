
/*
 * Copyright 2015 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "include/gpu/gl/GrGLInterface.h"
#include "src/utils/win/SkWGL.h"
#include "app_base/GLWindowContext.h"
#include "app_base/win/WindowContextFactory_win.h"

#include <GL/gl.h>
#include <Windows.h>

#include <iostream>

using app_base::DisplayParams;
using app_base::GLWindowContext;

#if defined(_M_ARM64)

namespace app_base {
namespace window_context_factory {

std::unique_ptr<WindowContext> MakeGLForWin(HWND, const DisplayParams&) { return nullptr; }

}  // namespace window_context_factory
}  // namespace app_base

#else

namespace {

class GLWindowContext_win : public GLWindowContext {
public:
    GLWindowContext_win(HWND, const DisplayParams&);
    ~GLWindowContext_win() override;

    void activate() override;

    void resize(int w, int h) override;

protected:
    void onSwapBuffers() override;

    sk_sp<const GrGLInterface> onInitializeContext() override;
    void onDestroyContext() override;

private:
    HWND fHWND;
    HGLRC fHGLRC;

    typedef GLWindowContext INHERITED;
};

GLWindowContext_win::GLWindowContext_win(HWND wnd, const DisplayParams& params)
        : INHERITED(params), fHWND(wnd), fHGLRC(NULL) {
    // any config code here (particularly for msaa)?

    this->initializeContext();
}

GLWindowContext_win::~GLWindowContext_win() { this->destroyContext(); }

sk_sp<const GrGLInterface> GLWindowContext_win::onInitializeContext() {
    HDC dc = GetDC(fHWND);
    if (dc == NULL) {
        std::cout << "GLWindowContext_win::onInitializeContext: dc = NULL" << std::endl;
        return nullptr;
    }

    if (NULL == fHGLRC) {
        std::cout << "GLWindowContext_win::onInitializeContext: initializing the context" << std::endl;
        fHGLRC = SkCreateWGLContext(dc, fDisplayParams.fMSAASampleCount, false /* deepColor */,
                                    kGLPreferCompatibilityProfile_SkWGLContextRequest);
        if (NULL == fHGLRC) {
            std::cout << "GLWindowContext_win::onInitializeContext: SkCreateWGLContext returns NULL"
                      << std::endl;
            return nullptr;
        }

        SkWGLExtensions extensions;
        if (extensions.hasExtension(dc, "WGL_EXT_swap_control")) {
            extensions.swapInterval(fDisplayParams.fDisableVsync ? 0 : 1);
        }

        // Look to see if RenderDoc is attached. If so, re-create the context with a core profile
        if (wglMakeCurrent(dc, fHGLRC)) {
            auto interface = GrGLMakeNativeInterface();
            bool renderDocAttached = interface->hasExtension("GL_EXT_debug_tool");
            interface.reset(nullptr);
            if (renderDocAttached) {
                wglDeleteContext(fHGLRC);
                fHGLRC = SkCreateWGLContext(dc, fDisplayParams.fMSAASampleCount,
                                            false /* deepColor */,
                                            kGLPreferCoreProfile_SkWGLContextRequest);
                if (NULL == fHGLRC) {
                    std::cout << "GLWindowContext_win::onInitializeContext: SkCreateWGLContext "
                                 "returns NULL (2)" << std::endl;
                    return nullptr;
                }
            }
        
            glClearStencil(0);
            glClearColor(0, 0, 0, 0);
            glStencilMask(0xffffffff);
            glClear(GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

            // use DescribePixelFormat to get the stencil and color bit depth.
            int pixelFormat = GetPixelFormat(dc);
            PIXELFORMATDESCRIPTOR pfd;
            DescribePixelFormat(dc, pixelFormat, sizeof(pfd), &pfd);
            fStencilBits = pfd.cStencilBits;

            // Get sample count if the MSAA WGL extension is present
            if (extensions.hasExtension(dc, "WGL_ARB_multisample")) {
                static const int kSampleCountAttr = SK_WGL_SAMPLES;
                extensions.getPixelFormatAttribiv(dc, pixelFormat, 0, 1, &kSampleCountAttr,
                                                  &fSampleCount);
                fSampleCount = SkTMax(fSampleCount, 1);
            } else {
                fSampleCount = 1;
            }

            RECT rect;
            GetClientRect(fHWND, &rect);
            fWidth = rect.right - rect.left;
            fHeight = rect.bottom - rect.top;

            // Instead of creating a viewport with our window size,
            // we are creating the biggest possible one.
            // Changing the viweport size takes time that makes
            // window resizing slow
            int vsWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
            int vsHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
            glViewport(0, 0, vsWidth, vsHeight);
        } else {
            std::cout << "GLWindowContext_win::onInitializeContext: wglMakeCurrent returns NULL (2)"
                      << std::endl;
        }
    }


    return GrGLMakeNativeInterface();
}

void GLWindowContext_win::resize(int w, int h) {
  GLWindowContext::resize(w, h);
}


void GLWindowContext_win::activate() {
    HDC dc = GetDC((HWND)fHWND);
    if (!wglMakeCurrent(dc, fHGLRC)) {
        std::cout << "wglMakeCurrent() returned false" << std::endl;
    }
}


void GLWindowContext_win::onDestroyContext() {
    wglDeleteContext(fHGLRC);
    fHGLRC = NULL;
}


void GLWindowContext_win::onSwapBuffers() {
    HDC dc = GetDC((HWND)fHWND);
    if (dc != NULL) {
        SwapBuffers(dc);
        ReleaseDC((HWND)fHWND, dc);
    } else {
        std::cout << "GetDC() returned NULL" << std::endl;
    }
}


}  // anonymous namespace

namespace app_base {
namespace window_context_factory {

std::unique_ptr<WindowContext> MakeGLForWin(HWND wnd, const DisplayParams& params) {
    std::unique_ptr<WindowContext> ctx(new GLWindowContext_win(wnd, params));
    if (!ctx->isValid()) {
        return nullptr;
    }
    return ctx;
}

}  // namespace window_context_factory
}  // namespace app_base

#endif
