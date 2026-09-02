// LAF OS Library
// Copyright (C) 2026  darumin (wasm milestone 1)
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "os/common/system.h"
#include "os/wasm/surface.h"
#include "os/wasm/window.h"

namespace os {

class SystemWasm : public CommonSystem {
public:
  Ref<Window> makeWindow(const WindowSpec& spec) override
  {
    return make_ref<WindowWasm>(spec);
  }

  Ref<Surface> makeSurface(int width, int height, const os::ColorSpaceRef& cs) override
  {
    return make_ref<SurfaceWasm>(width, height, cs);
  }

  Ref<Surface> makeRgbaSurface(int width, int height, const os::ColorSpaceRef& cs) override
  {
    return make_ref<SurfaceWasm>(width, height, cs);
  }
};

SystemRef System::makeWasm()
{
  return make_ref<SystemWasm>();
}

} // namespace os
