
/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef RasterWindowContext_DEFINED
#define RasterWindowContext_DEFINED

#include "app_base/WindowContext.h"

namespace app_base {

class RasterWindowContext : public WindowContext {
public:
    RasterWindowContext(const DisplayParams& params) : WindowContext(params) {}

protected:
    bool isGpuContext() override { return false; }
};

}   // namespace app_base

#endif
