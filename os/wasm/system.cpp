// LAF OS Library
// Copyright (C) 2026  darumin (wasm milestone 1)
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "os/common/system.h"
#include "os/wasm/surface.h"
#include "os/wasm/window.h"

#include <cstring>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace os {

class SystemWasm : public CommonSystem {
public:
  Ref<Window> makeWindow(const WindowSpec& spec) override
  {
    return os::make_ref<WindowWasm>(spec);
  }

  Ref<Surface> makeSurface(int width, int height, const os::ColorSpaceRef& cs) override
  {
    return os::make_ref<SurfaceWasm>(width, height, cs);
  }

  Ref<Surface> makeRgbaSurface(int width, int height, const os::ColorSpaceRef& cs) override
  {
    return os::make_ref<SurfaceWasm>(width, height, cs);
  }

  // Milestone 4: decode a PNG (read from Emscripten's virtual FS --
  // typically populated via --preload-file/--embed-file at link
  // time) into a Surface. This is what SpriteSheetTypeface::FromFile()
  // calls to load e.g. a bitmap UI font.
  Ref<Surface> loadRgbaSurface(const char* filename) override
  {
    int w = 0, h = 0, channels = 0;
    stbi_uc* pixels = stbi_load(filename, &w, &h, &channels, 4 /* force RGBA */);
    if (!pixels)
      return nullptr;

    auto surface = os::make_ref<SurfaceWasm>(w, h, os::ColorSpaceRef());
    // stbi_load's output is already tightly-packed row-major RGBA8,
    // one byte per channel -- exactly SurfaceWasm's internal format
    // (see packColor()/getFormat() in os/wasm/surface.h), so this
    // is a straight memcpy rather than a per-pixel conversion.
    std::memcpy(surface->getData(0, 0), pixels, size_t(w) * size_t(h) * 4);
    stbi_image_free(pixels);
    return surface;
  }
};

SystemRef System::makeWasm()
{
  return os::make_ref<SystemWasm>();
}

} // namespace os
