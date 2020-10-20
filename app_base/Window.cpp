/*
* Copyright 2016 Google Inc.
*
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*/

#include "app_base/Window.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"
#include "app_base/WindowContext.h"

namespace sk_app {

Window::Window() {}

Window::~Window() {}

void Window::detach() { fWindowContext = nullptr; }

void Window::visitLayers(std::function<void(Layer*)> visitor) {
    for (int i = 0; i < fLayers.count(); ++i) {
        if (fLayers[i]->fActive) {
            visitor(fLayers[i]);
        }
    }
}

bool Window::signalLayers(std::function<bool(Layer*)> visitor) {
    for (int i = fLayers.count() - 1; i >= 0; --i) {
        if (fLayers[i]->fActive && visitor(fLayers[i])) {
            return true;
        }
    }
    return false;
}

void Window::onBackendCreated() {
    this->visitLayers([](Layer* layer) { layer->onBackendCreated(); });
}

bool Window::onChar(const ui::PlatformEvent& platformEvent, SkUnichar c,
                    skui::ModifierKey modifiers) {
    return this->signalLayers(
            [=](Layer* layer) { return layer->onChar(platformEvent, c, modifiers); });
}

bool Window::onKey(const ui::PlatformEvent& platformEvent, uint64_t key, skui::InputState state,
                   skui::ModifierKey modifiers) {
    return this->signalLayers(
            [=](Layer* layer) { return layer->onKey(platformEvent, key, state, modifiers); });
}

bool Window::onMouse(const ui::PlatformEvent& platformEvent, int x, int y, skui::InputState state,
                     skui::ModifierKey modifiers) {
    return this->signalLayers(
            [=](Layer* layer) { return layer->onMouse(platformEvent, x, y, state, modifiers); });
}

bool Window::onMouseWheel(const ui::PlatformEvent& platformEvent, float delta,
                          skui::ModifierKey modifiers) {
    return this->signalLayers(
            [=](Layer* layer) { return layer->onMouseWheel(platformEvent, delta, modifiers); });
}

bool Window::onTouch(const ui::PlatformEvent& platformEvent, intptr_t owner, skui::InputState state,
                     float x, float y) {
    return this->signalLayers(
            [=](Layer* layer) { return layer->onTouch(platformEvent, owner, state, x, y); });
}

bool Window::onFling(skui::InputState state) {
    return this->signalLayers([=](Layer* layer) { return layer->onFling(state); });
}

bool Window::onPinch(skui::InputState state, float scale, float x, float y) {
    return this->signalLayers([=](Layer* layer) { return layer->onPinch(state, scale, x, y); });
}

void Window::onUIStateChanged(const SkString& stateName, const SkString& stateValue) {
    this->visitLayers([=](Layer* layer) { layer->onUIStateChanged(stateName, stateValue); });
}

void Window::onPaint() {
    if (!fWindowContext) {
        return;
    }
    markInvalProcessed();
    this->visitLayers([](Layer* layer) { layer->onPrePaint(); });
    sk_sp<SkSurface> backbuffer = fWindowContext->getBackbufferSurface();
    if (backbuffer) {
        // draw into the canvas of this surface
        this->visitLayers([=](Layer* layer) { layer->onPaint(backbuffer.get()); });

        backbuffer->flush();

        fWindowContext->swapBuffers();
    } else {
        printf("no backbuffer!?\n");
        // try recreating testcontext
    }
}

void Window::onResize(int w, int h) {
    if (!fWindowContext) {
        return;
    }
    fWindowContext->resize(w, h);
    this->visitLayers([=](Layer* layer) { layer->onResize(w, h); });
}

void Window::onBeginResizing() {
    if (!fWindowContext) {
        return;
    }
    this->visitLayers([=](Layer* layer) { layer->onBeginResizing(); });
}

void Window::onEndResizing() {
    if (!fWindowContext) {
        return;
    }
    this->visitLayers([=](Layer* layer) { layer->onEndResizing(); });
}


int Window::width() const {
    if (!fWindowContext) {
        return 0;
    }
    return fWindowContext->width();
}

int Window::height() const {
    if (!fWindowContext) {
        return 0;
    }
    return fWindowContext->height();
}

void Window::setRequestedDisplayParams(const DisplayParams& params, bool /* allowReattach */) {
    fRequestedDisplayParams = params;
    if (fWindowContext) {
        fWindowContext->setDisplayParams(fRequestedDisplayParams);
    }
}

int Window::sampleCount() const {
    if (!fWindowContext) {
        return 0;
    }
    return fWindowContext->sampleCount();
}

int Window::stencilBits() const {
    if (!fWindowContext) {
        return -1;
    }
    return fWindowContext->stencilBits();
}

GrContext* Window::getGrContext() const {
    if (!fWindowContext) {
        return nullptr;
    }
    return fWindowContext->getGrContext();
}

void Window::inval() {
    if (!fWindowContext) {
        return;
    }
    if (!fIsContentInvalidated) {
        fIsContentInvalidated = true;
        onInval();
    }
}

void Window::markInvalProcessed() {
    fIsContentInvalidated = false;
}

uint32_t Window::getDPI() {
  return 96;    // Default screen DPI value
}

}   // namespace sk_app
