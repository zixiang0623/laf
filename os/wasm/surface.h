// LAF OS Library
// Copyright (C) 2026  darumin (wasm milestone 1)
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.
//
// Minimal, Skia-free software Surface for Emscripten. Correctness
// and speed are NOT the goal here -- this exists only to get real
// pixels into a browser <canvas> (via SDL2) so we can validate the
// rest of the pipeline. Transform matrix, clip paths, and most
// blend modes are ignored/approximated. Revisit once something is
// actually visible.

#ifndef OS_WASM_SURFACE_INCLUDED
#define OS_WASM_SURFACE_INCLUDED
#pragma once

#include "gfx/matrix.h"
#include "gfx/path.h"
#include "gfx/rect.h"
#include "gfx/region.h"
#include "os/paint.h"
#include "os/ref.h"
#include "os/surface.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace os {

class SurfaceWasm : public Surface {
public:
  SurfaceWasm(int width, int height, const os::ColorSpaceRef& cs)
    : m_width(width)
    , m_height(height)
    , m_colorSpace(cs)
    , m_data(size_t(std::max(width, 1)) * size_t(std::max(height, 1)), 0)
  {
    m_clipStack.push_back(gfx::Rect(0, 0, width, height));
  }

  int width() const override { return m_width; }
  int height() const override { return m_height; }
  const ColorSpaceRef& colorSpace() const override { return m_colorSpace; }
  bool isDirectToScreen() const override { return m_directToScreen; }
  void setDirectToScreen(bool v) { m_directToScreen = v; }

  void setImmutable() override {}

  int getSaveCount() const override { return int(m_clipStack.size()); }
  gfx::Rect getClipBounds() const override { return m_clipStack.back(); }
  void saveClip() override { m_clipStack.push_back(m_clipStack.back()); }
  void restoreClip() override
  {
    if (m_clipStack.size() > 1)
      m_clipStack.pop_back();
  }
  bool clipRect(const gfx::Rect& rc) override
  {
    m_clipStack.back() = m_clipStack.back().createIntersection(rc);
    return !m_clipStack.back().isEmpty();
  }
  // Paths/regions as clip: approximate with their bounding rect.
  void clipPath(const gfx::Path& path) override
  {
    clipRect(gfx::Rect(int(path.bounds().x),
                        int(path.bounds().y),
                        int(path.bounds().w),
                        int(path.bounds().h)));
  }
  void clipRegion(const gfx::Region& region) override { clipRect(region.bounds()); }

  // No transform support yet (milestone 1: axis-aligned only).
  void save() override {}
  void concat(const gfx::Matrix&) override {}
  void setMatrix(const gfx::Matrix&) override {}
  void resetMatrix() override {}
  void restore() override {}
  gfx::Matrix matrix() const override { return gfx::Matrix(); }

  void lock() override {}
  void unlock() override {}

  void clear() override { std::fill(m_data.begin(), m_data.end(), 0u); }

  uint8_t* getData(int x, int y) const override
  {
    return reinterpret_cast<uint8_t*>(
      const_cast<uint32_t*>(&m_data[size_t(y) * size_t(m_width) + size_t(x)]));
  }

  void getFormat(SurfaceFormatData* fd) const override
  {
    fd->format = kRgbaSurfaceFormat;
    fd->bitsPerPixel = 32;
    fd->redShift = 0;
    fd->greenShift = 8;
    fd->blueShift = 16;
    fd->alphaShift = 24;
    fd->redMask = 0x000000ff;
    fd->greenMask = 0x0000ff00;
    fd->blueMask = 0x00ff0000;
    fd->alphaMask = 0xff000000;
    fd->pixelAlpha = PixelAlpha::kStraight;
  }

  gfx::Color getPixel(int x, int y) const override
  {
    if (x < 0 || y < 0 || x >= m_width || y >= m_height)
      return gfx::ColorNone;
    uint32_t p = m_data[size_t(y) * size_t(m_width) + size_t(x)];
    return gfx::rgba(p & 0xff, (p >> 8) & 0xff, (p >> 16) & 0xff, (p >> 24) & 0xff);
  }

  void putPixel(gfx::Color color, int x, int y) override
  {
    if (x < 0 || y < 0 || x >= m_width || y >= m_height)
      return;
    if (!m_clipStack.back().contains(gfx::Point(x, y)))
      return;
    m_data[size_t(y) * size_t(m_width) + size_t(x)] = packColor(color);
  }

  void drawLine(float x0, float y0, float x1, float y1, const os::Paint& paint) override
  {
    // Naive DDA, no width/antialiasing.
    float dx = x1 - x0, dy = y1 - y0;
    int steps = int(std::max(std::abs(dx), std::abs(dy)));
    if (steps == 0) {
      putPixel(paint.color(), int(x0), int(y0));
      return;
    }
    for (int i = 0; i <= steps; ++i) {
      float t = float(i) / float(steps);
      putPixel(paint.color(), int(x0 + dx * t), int(y0 + dy * t));
    }
  }

  void drawRect(const gfx::RectF& rc, const os::Paint& paint) override
  {
    gfx::Rect r(int(rc.x), int(rc.y), int(rc.w), int(rc.h));
    // BlendMode::Clear means "erase to transparent", regardless of
    // paint.color() -- needed by SpriteSheetTypeface::fromFile(),
    // which clears a 1px border around each detected glyph. We
    // don't implement blend modes in general (see file header), but
    // this one specific case is cheap and has a real caller.
    const gfx::Color drawColor =
      (paint.blendMode() == os::BlendMode::Clear ? gfx::ColorNone : paint.color());
    if (paint.style() == os::Paint::Fill || paint.style() == os::Paint::StrokeAndFill) {
      for (int y = r.y; y < r.y + r.h; ++y)
        for (int x = r.x; x < r.x + r.w; ++x)
          putPixel(drawColor, x, y);
    }
    else { // Stroke: outline only
      for (int x = r.x; x < r.x + r.w; ++x) {
        putPixel(drawColor, x, r.y);
        putPixel(drawColor, x, r.y + r.h - 1);
      }
      for (int y = r.y; y < r.y + r.h; ++y) {
        putPixel(drawColor, r.x, y);
        putPixel(drawColor, r.x + r.w - 1, y);
      }
    }
  }

  void drawCircle(float cx, float cy, float radius, const os::Paint& paint) override
  {
    // Naive filled/stroked circle via midpoint check.
    int r = int(radius);
    for (int y = -r; y <= r; ++y) {
      for (int x = -r; x <= r; ++x) {
        int d2 = x * x + y * y;
        bool inside = d2 <= r * r;
        bool onEdge = d2 <= r * r && d2 >= (r - 1) * (r - 1);
        if ((paint.style() == os::Paint::Fill && inside) ||
            (paint.style() != os::Paint::Fill && onEdge)) {
          putPixel(paint.color(), int(cx) + x, int(cy) + y);
        }
      }
    }
  }

  // Not implemented yet (milestone 1 doesn't need vector paths).
  void drawPath(const gfx::Path&, const os::Paint&) override {}

  void blitTo(Surface* dest, int srcx, int srcy, int dstx, int dsty, int w, int h) const override
  {
    for (int y = 0; y < h; ++y)
      for (int x = 0; x < w; ++x)
        dest->putPixel(getPixel(srcx + x, srcy + y), dstx + x, dsty + y);
  }

  void scrollTo(const gfx::Rect& rc, int dx, int dy) override
  {
    // Milestone 1: unused by the smoke test, left as a no-op.
  }

  void drawSurface(const Surface* src, int dstx, int dsty) override
  {
    for (int y = 0; y < src->height(); ++y)
      for (int x = 0; x < src->width(); ++x)
        putPixel(src->getPixel(x, y), dstx + x, dsty + y);
  }

  void drawSurface(const Surface* src,
                    const gfx::Rect& srcRect,
                    const gfx::Rect& dstRect,
                    const os::Sampling&,
                    const os::Paint*) override
  {
    // Nearest-neighbor only.
    for (int y = 0; y < dstRect.h; ++y) {
      int sy = srcRect.y + (dstRect.h > 0 ? y * srcRect.h / dstRect.h : 0);
      for (int x = 0; x < dstRect.w; ++x) {
        int sx = srcRect.x + (dstRect.w > 0 ? x * srcRect.w / dstRect.w : 0);
        putPixel(src->getPixel(sx, sy), dstRect.x + x, dstRect.y + y);
      }
    }
  }

  void drawRgbaSurface(const Surface* src, int dstx, int dsty) override
  {
    drawSurface(src, dstx, dsty);
  }

  void drawRgbaSurface(const Surface* src,
                        int srcx,
                        int srcy,
                        int dstx,
                        int dsty,
                        int w,
                        int h) override
  {
    for (int y = 0; y < h; ++y)
      for (int x = 0; x < w; ++x)
        putPixel(src->getPixel(srcx + x, srcy + y), dstx + x, dsty + y);
  }

  void drawColoredRgbaSurface(const Surface* src,
                               gfx::Color fg,
                               gfx::Color bg,
                               const gfx::Clip& clip) override
  {
    // Milestone 1 approximation: ignore fg/bg recoloring, just blit alpha.
    for (int y = 0; y < clip.size.h; ++y) {
      for (int x = 0; x < clip.size.w; ++x) {
        gfx::Color c = src->getPixel(clip.src.x + x, clip.src.y + y);
        if (gfx::geta(c) > 0)
          putPixel(fg, clip.dst.x + x, clip.dst.y + y);
      }
    }
  }

  void drawSurfaceNine(os::Surface* surface,
                        const gfx::Rect& src,
                        const gfx::Rect& center,
                        const gfx::Rect& dst,
                        bool drawCenter,
                        const os::Paint*) override
  {
    // Milestone 1: not needed yet for a smoke test, no-op.
  }

  [[nodiscard]] SurfaceRef applyScale(float scaleFactor, const Sampling&) override
  {
    // Milestone 1: scaling not implemented, just hand back this
    // surface (with an extra ref, since Ref(T*) doesn't add one).
    this->ref();
    return os::Ref<Surface>(this);
  }

  void* nativeHandle() override { return this; }

  const std::vector<uint32_t>& rawData() const { return m_data; }

private:
  static uint32_t packColor(gfx::Color c)
  {
    return uint32_t(gfx::getr(c)) | (uint32_t(gfx::getg(c)) << 8) |
           (uint32_t(gfx::getb(c)) << 16) | (uint32_t(gfx::geta(c)) << 24);
  }

  int m_width;
  int m_height;
  os::ColorSpaceRef m_colorSpace;
  std::vector<uint32_t> m_data; // RGBA8888, row-major
  std::vector<gfx::Rect> m_clipStack;
  bool m_directToScreen = false;
};

} // namespace os

#endif
