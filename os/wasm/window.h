// LAF OS Library
// Copyright (C) 2026  darumin (wasm milestone 1)
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.
//
// Minimal Window for Emscripten, backed by SDL2 (available via
// Emscripten's built-in port, no separate build step needed --
// just link with -sUSE_SDL=2). Most window-manager-ish features
// (move/resize/maximize/fullscreen/icons/etc.) are no-ops: there's
// no real OS window here, just a <canvas> managed by SDL2.

#ifndef OS_WASM_WINDOW_INCLUDED
#define OS_WASM_WINDOW_INCLUDED
#pragma once

#include "os/wasm/surface.h"
#include "os/window.h"
#include "os/window_spec.h"

#include <SDL2/SDL.h>

namespace os {

class WindowWasm : public Window {
public:
  WindowWasm(const WindowSpec& spec)
    : m_width(spec.contentRect().w > 0 ? spec.contentRect().w : 640)
    , m_height(spec.contentRect().h > 0 ? spec.contentRect().h : 480)
    , m_scale(spec.scale() > 0 ? spec.scale() : 1)
  {
    SDL_Init(SDL_INIT_VIDEO);
    m_sdlWindow = SDL_CreateWindow("aseprite-wasm",
                                    SDL_WINDOWPOS_UNDEFINED,
                                    SDL_WINDOWPOS_UNDEFINED,
                                    m_width,
                                    m_height,
                                    SDL_WINDOW_SHOWN);
    m_sdlRenderer = SDL_CreateRenderer(m_sdlWindow, -1, SDL_RENDERER_SOFTWARE);
    m_sdlTexture = SDL_CreateTexture(m_sdlRenderer,
                                      SDL_PIXELFORMAT_ABGR8888,
                                      SDL_TEXTUREACCESS_STREAMING,
                                      m_width,
                                      m_height);
    m_surface = os::make_ref<SurfaceWasm>(m_width, m_height, os::ColorSpaceRef());
  }

  ~WindowWasm()
  {
    if (m_sdlTexture)
      SDL_DestroyTexture(m_sdlTexture);
    if (m_sdlRenderer)
      SDL_DestroyRenderer(m_sdlRenderer);
    if (m_sdlWindow)
      SDL_DestroyWindow(m_sdlWindow);
  }

  gfx::Rect frame() const override { return gfx::Rect(0, 0, m_width, m_height); }
  void setFrame(const gfx::Rect&) override {}
  gfx::Rect contentRect() const override { return frame(); }
  gfx::Rect restoredFrame() const override { return frame(); }
  int width() const override { return m_width; }
  int height() const override { return m_height; }
  int scale() const override { return m_scale; }
  void setScale(int scale) override { m_scale = scale; }
  bool isVisible() const override { return true; }
  void setVisible(bool) override {}
  Surface* surface() override { return m_surface.get(); }

  void invalidateRegion(const gfx::Region&) override { present(); }

  bool gpuAcceleration() const override { return false; }
  void swapBuffers() override { present(); }

  void activate() override {}
  void maximize() override {}
  void minimize() override {}
  bool isMaximized() const override { return false; }
  bool isMinimized() const override { return false; }
  bool isTransparent() const override { return false; }
  bool isFullscreen() const override { return false; }
  void setFullscreen(bool) override {}

  std::string title() const override { return m_title; }
  void setTitle(const std::string& title) override
  {
    m_title = title;
    if (m_sdlWindow)
      SDL_SetWindowTitle(m_sdlWindow, title.c_str());
  }

  NativeCursor nativeCursor() override { return NativeCursor::Arrow; }
  bool setCursor(NativeCursor) override { return false; }
  bool setCursor(const CursorRef&) override { return false; }
  void setMousePosition(const gfx::Point&) override {}
  void captureMouse() override {}
  void releaseMouse() override {}

  void performWindowAction(WindowAction, const Event*) override {}
  std::string getLayout() override { return std::string(); }
  void setLayout(const std::string&) override {}
  os::ScreenRef screen() const override { return nullptr; }
  void setColorSpace(const os::ColorSpaceRef&) override {}
  NativeHandle nativeHandle() const override { return (NativeHandle)m_sdlWindow; }

  // Pushes the current surface pixels to the actual <canvas> via SDL2.
  void present()
  {
    if (!m_sdlTexture)
      return;
    SDL_UpdateTexture(
      m_sdlTexture, nullptr, m_surface->rawData().data(), m_width * int(sizeof(uint32_t)));
    SDL_RenderClear(m_sdlRenderer);
    SDL_RenderCopy(m_sdlRenderer, m_sdlTexture, nullptr, nullptr);
    SDL_RenderPresent(m_sdlRenderer);
  }

private:
  int m_width, m_height, m_scale;
  std::string m_title;
  os::Ref<SurfaceWasm> m_surface;
  SDL_Window* m_sdlWindow = nullptr;
  SDL_Renderer* m_sdlRenderer = nullptr;
  SDL_Texture* m_sdlTexture = nullptr;
};

} // namespace os

#endif
