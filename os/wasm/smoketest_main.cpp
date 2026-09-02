// Milestone 1 smoke test: create a wasm window and paint a solid
// rectangle to it, to prove pixels actually reach the browser
// <canvas>. Deliberately does not link laf-text (fonts are a later
// milestone).

#include "gfx/rect.h"
#include "os/os.h"

#include <emscripten.h>

namespace {
os::WindowRef g_window;
}

void main_loop()
{
  os::Surface* surf = g_window->surface();
  os::Paint paint;
  paint.color(gfx::rgba(200, 60, 60, 255));
  surf->drawRect(gfx::Rect(0, 0, surf->width(), surf->height()), paint);

  paint.color(gfx::rgba(250, 250, 250, 255));
  surf->drawRect(gfx::Rect(40, 40, surf->width() - 80, surf->height() - 80), paint);

  g_window->invalidateRegion(gfx::Region(surf->bounds()));
}

int main()
{
  os::SystemRef system = os::System::make();
  system->setAppName("aseprite-wasm-milestone1");

  os::WindowSpec spec;
  spec.contentRect(gfx::Rect(0, 0, 400, 300));
  g_window = system->makeWindow(spec);
  g_window->setVisible(true);

  emscripten_set_main_loop(main_loop, 0, 1);
  return 0;
}
