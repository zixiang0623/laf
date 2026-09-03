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

#include "base/utf8_decode.h"
#include "os/event.h"
#include "os/wasm/keys.h"
#include "os/wasm/surface.h"
#include "os/window.h"
#include "os/window_spec.h"

#include <SDL2/SDL.h>

#include <string>
#include <unordered_map>

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

    // Milestone 3: turn on IME/text composition so we get
    // SDL_TEXTINPUT events (proper Unicode text, not just raw
    // scancodes) for typing.
    SDL_StartTextInput();

    // Milestone 2: register so pumpEvents() can route SDL events
    // (which carry an SDL windowID) back to this instance.
    s_windows[SDL_GetWindowID(m_sdlWindow)] = this;
  }

  ~WindowWasm()
  {
    SDL_StopTextInput();
    s_windows.erase(SDL_GetWindowID(m_sdlWindow));
    if (m_sdlTexture)
      SDL_DestroyTexture(m_sdlTexture);
    if (m_sdlRenderer)
      SDL_DestroyRenderer(m_sdlRenderer);
    if (m_sdlWindow)
      SDL_DestroyWindow(m_sdlWindow);
  }

  // Milestone 2: drains every pending SDL event (mouse/keyboard/
  // window) and translates+queues each one as an os::Event via the
  // matching WindowWasm (looked up by SDL windowID). Called once
  // per getEvent() poll from EventQueueWasm -- see os/wasm/event_queue.h.
  static void pumpEvents()
  {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      switch (e.type) {
        case SDL_QUIT:
          // No windowID on SDL_QUIT: tell every window we know about.
          for (auto& it : s_windows) {
            Event ev;
            ev.setType(Event::CloseWindow);
            it.second->queueEvent(ev);
          }
          break;
        case SDL_WINDOWEVENT:
          if (auto* w = find(e.window.windowID))
            w->handleSDLWindowEvent(e.window);
          break;
        case SDL_MOUSEMOTION:
          if (auto* w = find(e.motion.windowID))
            w->handleSDLMouseMotion(e.motion);
          break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
          if (auto* w = find(e.button.windowID))
            w->handleSDLMouseButton(e.button);
          break;
        case SDL_MOUSEWHEEL:
          if (auto* w = find(e.wheel.windowID))
            w->handleSDLMouseWheel(e.wheel);
          break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
          if (auto* w = find(e.key.windowID))
            w->handleSDLKey(e.key);
          break;
        case SDL_TEXTINPUT:
          if (auto* w = find(e.text.windowID))
            w->handleSDLTextInput(e.text);
          break;
        default:
          break;
      }
    }
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
  static WindowWasm* find(Uint32 sdlWindowId)
  {
    auto it = s_windows.find(sdlWindowId);
    return (it != s_windows.end() ? it->second : nullptr);
  }

  void handleSDLMouseMotion(const SDL_MouseMotionEvent& e)
  {
    Event ev;
    ev.setType(Event::MouseMove);
    ev.setPosition(gfx::Point(e.x, e.y));
    ev.setModifiers(sdl_modstate_to_os(SDL_GetModState()));
    ev.setPointerType(PointerType::Mouse);
    queueEvent(ev);
  }

  void handleSDLMouseButton(const SDL_MouseButtonEvent& e)
  {
    Event ev;
    const bool isDown = (e.type == SDL_MOUSEBUTTONDOWN);
    ev.setType(isDown && e.clicks >= 2 ? Event::MouseDoubleClick :
               isDown                  ? Event::MouseDown :
                                          Event::MouseUp);
    ev.setPosition(gfx::Point(e.x, e.y));
    ev.setButton(sdl_button_to_os(e.button));
    ev.setModifiers(sdl_modstate_to_os(SDL_GetModState()));
    ev.setPointerType(PointerType::Mouse);
    queueEvent(ev);
  }

  void handleSDLMouseWheel(const SDL_MouseWheelEvent& e)
  {
    Event ev;
    ev.setType(Event::MouseWheel);
    int mx = 0, my = 0;
    SDL_GetMouseState(&mx, &my);
    ev.setPosition(gfx::Point(mx, my));
    gfx::Point delta(e.x, e.y);
    if (e.direction == SDL_MOUSEWHEEL_FLIPPED) {
      delta.x = -delta.x;
      delta.y = -delta.y;
    }
    ev.setWheelDelta(delta);
    ev.setModifiers(sdl_modstate_to_os(SDL_GetModState()));
    ev.setPointerType(PointerType::Mouse);
    queueEvent(ev);
  }

  void handleSDLKey(const SDL_KeyboardEvent& e)
  {
    Event ev;
    ev.setType(e.type == SDL_KEYDOWN ? Event::KeyDown : Event::KeyUp);
    ev.setScancode(sdl_scancode_to_os(e.keysym.scancode));
    ev.setModifiers(sdl_modstate_to_os(e.keysym.mod));
    ev.setRepeat(e.repeat);
    // Unicode text for this key (if any) arrives separately via
    // SDL_TEXTINPUT -- see handleSDLTextInput() below -- since IME
    // composition means the two aren't always 1:1 with this
    // physical key-down/up pair.
    queueEvent(ev);
  }

  // Milestone 3: SDL_TEXTINPUT carries the actual composed Unicode
  // text (UTF-8, up to 32 bytes) for a keystroke -- this is what
  // handles IME, dead keys, and non-ASCII layouts correctly, unlike
  // trying to derive a character from the scancode ourselves. We
  // queue one KeyDown per code point, scancode-less (kKeyNil),
  // matching how other laf backends carry unicodeChar() on a
  // KeyDown that consumers check independently of scancode() for
  // shortcut handling vs. text entry.
  void handleSDLTextInput(const SDL_TextInputEvent& e)
  {
    base::utf8_decode dec{ std::string(e.text) };
    for (;;) {
      const base::codepoint_t cp = dec.next();
      if (cp == 0)
        break;
      Event ev;
      ev.setType(Event::KeyDown);
      ev.setScancode(kKeyNil);
      ev.setUnicodeChar(cp);
      ev.setModifiers(sdl_modstate_to_os(SDL_GetModState()));
      queueEvent(ev);
    }
  }

  void handleSDLWindowEvent(const SDL_WindowEvent& e)
  {
    switch (e.event) {
      case SDL_WINDOWEVENT_CLOSE: {
        Event ev;
        ev.setType(Event::CloseWindow);
        queueEvent(ev);
        break;
      }
      case SDL_WINDOWEVENT_SIZE_CHANGED:
      case SDL_WINDOWEVENT_RESIZED: {
        resizeBuffers(e.data1, e.data2);
        Event ev;
        ev.setType(Event::ResizeWindow);
        queueEvent(ev);
        break;
      }
      case SDL_WINDOWEVENT_ENTER: {
        Event ev;
        ev.setType(Event::MouseEnter);
        queueEvent(ev);
        break;
      }
      case SDL_WINDOWEVENT_LEAVE: {
        Event ev;
        ev.setType(Event::MouseLeave);
        queueEvent(ev);
        break;
      }
      case SDL_WINDOWEVENT_FOCUS_GAINED: {
        Event ev;
        ev.setType(Event::WindowEnter);
        queueEvent(ev);
        break;
      }
      case SDL_WINDOWEVENT_FOCUS_LOST: {
        Event ev;
        ev.setType(Event::WindowLeave);
        queueEvent(ev);
        break;
      }
      default: break;
    }
  }

  // Recreates the SDL texture and the software surface to match a
  // new canvas size. SurfaceWasm has no in-place resize, so we just
  // swap in a fresh one -- whatever was painted before is lost,
  // same as any other backend after a resize.
  void resizeBuffers(int w, int h)
  {
    if (w <= 0 || h <= 0 || (w == m_width && h == m_height))
      return;
    m_width = w;
    m_height = h;
    if (m_sdlTexture)
      SDL_DestroyTexture(m_sdlTexture);
    m_sdlTexture = SDL_CreateTexture(
      m_sdlRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, m_width, m_height);
    m_surface = os::make_ref<SurfaceWasm>(m_width, m_height, os::ColorSpaceRef());
  }

  int m_width, m_height, m_scale;
  std::string m_title;
  os::Ref<SurfaceWasm> m_surface;
  SDL_Window* m_sdlWindow = nullptr;
  SDL_Renderer* m_sdlRenderer = nullptr;
  SDL_Texture* m_sdlTexture = nullptr;

  static inline std::unordered_map<Uint32, WindowWasm*> s_windows;
};

} // namespace os

#endif
