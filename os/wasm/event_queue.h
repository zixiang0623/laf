// LAF OS Library
// Copyright (C) 2026  darumin (wasm milestone 0)
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.
//
// Minimal EventQueueImpl for Emscripten builds. No real event
// source is wired up yet (no window, no input) -- this only exists
// so that os/common/event_queue.cpp has something to instantiate
// and the library links. A real implementation will push events
// from JS (mouse/keyboard/resize) via emscripten_set_*_callback
// once the actual os/wasm window backend exists.

#ifndef OS_WASM_EVENT_QUEUE_INCLUDED
#define OS_WASM_EVENT_QUEUE_INCLUDED
#pragma once

#include "base/concurrent_queue.h"
#include "os/event.h"
#include "os/event_queue.h"

namespace os {

class EventQueueWasm : public EventQueue {
public:
  void queueEvent(const Event& ev) override { m_events.push(ev); }

  void getEvent(Event& ev, double timeout) override
  {
    // Milestone 0: no blocking/timeout wait yet, just drain
    // whatever's queued (there's nothing pushing events in yet).
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
