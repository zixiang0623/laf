// LAF OS Library
// Copyright (C) 2026  darumin (wasm milestone 0)
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.
//
// EventQueueImpl for Emscripten builds. Milestone 2: real input
// now flows through here. SDL2 (via Emscripten's port) delivers
// mouse/keyboard/window events; WindowWasm::pumpEvents() drains
// SDL's queue each time we're asked for an event, translates each
// SDL event into an os::Event, and pushes it via queueEvent() --
// same architecture as os/win's message pump living inside
// getEvent(), just non-blocking since a browser tab can't block its
// main thread waiting for input the way GetMessage() can.

#ifndef OS_WASM_EVENT_QUEUE_INCLUDED
#define OS_WASM_EVENT_QUEUE_INCLUDED
#pragma once

#include "base/concurrent_queue.h"
#include "os/event.h"
#include "os/event_queue.h"
#include "os/wasm/window.h"

namespace os {

class EventQueueWasm : public EventQueue {
public:
  void queueEvent(const Event& ev) override { m_events.push(ev); }

  void getEvent(Event& ev, double timeout) override
  {
    // Pull in whatever SDL has queued (mouse/keyboard/window
    // events) since the last call, translated into os::Events by
    // WindowWasm. Non-blocking regardless of timeout: the browser's
    // event loop already paces us via emscripten_set_main_loop, so
    // there's nothing to usefully wait on here.
    WindowWasm::pumpEvents();

    if (!m_events.try_pop(ev))
      ev = Event();
  }

  void clearEvents() override
  {
    Event ev;
    while (m_events.try_pop(ev))
      ;
  }

private:
  base::concurrent_queue<Event> m_events;
};

using EventQueueImpl = EventQueueWasm;

} // namespace os

#endif
