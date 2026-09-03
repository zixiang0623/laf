// Milestone 1 smoke test (base rendering) + milestone 2 (real
// input): paints a rectangle to prove pixels reach the browser
// <canvas>, and now also drains os::EventQueue every frame to prove
// mouse/keyboard/window events actually flow end-to-end from the
// browser through SDL2 into os::Event. Deliberately does not link
// laf-text (fonts are a later milestone).
//
// Visible proof of milestone 2: a small square follows the mouse,
// the inner panel's color cycles on each click, and key presses are
// logged to the browser console (F12 devtools).

#include "gfx/rect.h"
#include "os/os.h"

#include <cstdio>
#include <emscripten.h>

namespace {
os::WindowRef g_window;
gfx::Point g_mousePos(-100, -100); // off-surface until we get a real move
int g_clickCount = 0;
}

void main_loop()
{
  // Milestone 2: drain everything SDL/os::EventQueue collected since
  // the last frame (pumping happens inside getEvent() itself).
  os::Event ev;
  for (;;) {
    os::EventQueue::instance()->getEvent(ev, 0.0);
    if (ev.type() == os::Event::None)
      break;

    switch (ev.type()) {
      case os::Event::MouseMove: g_mousePos = ev.position(); break;
      case os::Event::MouseDown: g_clickCount++; break;
      case os::Event::KeyDown:
        std::printf("[wasm-milestone2] KeyDown scancode=%d mods=%d\n",
                    int(ev.scancode()),
                    int(ev.modifiers()));
        break;
      case os::Event::CloseWindow: emscripten_cancel_main_loop(); return;
      default: break;
    }
  }

  os::Surface* surf = g_window->surface();
  os::Paint paint;
  paint.color(gfx::rgba(200, 60, 60, 255));
  surf->drawRect(gfx::Rect(0, 0, surf->width(), surf->height()), paint);

  // Inner panel cycles through a few colors on each click, to make
  // MouseDown handling visible without needing devtools open.
  static const gfx::Color kPanelColors[] = { gfx::rgba(250, 250, 250, 255),
                                              gfx::rgba(250, 220, 120, 255),
                                              gfx::rgba(120, 220, 250, 255) };
  paint.color(kPanelColors[g_clickCount % 3]);
  surf->drawRect(gfx::Rect(40, 40, surf->width() - 80, surf->height() - 80), paint);

  // Small marker that tracks the mouse, to make MouseMove visible.
  paint.color(gfx::rgba(40, 160, 40, 255));
  surf->drawRect(gfx::Rect(g_mousePos.x - 5, g_mousePos.y - 5, 10, 10), paint);

  g_window->invalidateRegion(gfx::Region(surf->bounds()));
}

int app_main(int argc, char* argv[])
{
  os::SystemRef system = os::System::make();
  system->setAppName("aseprite-wasm-milestone2");

  os::WindowSpec spec;
  spec.contentRect(gfx::Rect(0, 0, 400, 300));
  g_window = system->makeWindow(spec);
  g_window->setVisible(true);

  emscripten_set_main_loop(main_loop, 0, 1);
  return 0;
}
