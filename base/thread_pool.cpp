// LAF Base Library
// Copyright (C) 2019-2022  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "base/debug.h"
#include "base/log.h"
#include "base/thread_pool.h"

namespace base {

#if LAF_WASM

// Emscripten's default (non-pthread) build can't spawn real OS
// threads at all -- pthread_create() aborts at runtime (no
// -pthread/-sUSE_PTHREADS=1 opt-in, which would also need a
// cross-origin-isolated context this port can't assume). Run
// everything synchronously on the calling thread instead: less
// concurrent, but correct, and every existing call site (this is a
// generic pool used app-wide) keeps working unchanged.

thread_pool::thread_pool(const size_t n) : m_running(true), m_doingWork(0)
{
  (void)n;
}

thread_pool::~thread_pool() {}

const thread_pool::work* thread_pool::execute(std::function<void()>&& func)
{
  thread_pool::work_ptr work = std::make_unique<thread_pool::work>(std::move(func));
  const thread_pool::work* result = work.get();
  if (work->m_func) {
    try {
      work->m_func();
    }
    catch (const std::exception& e) {
      LOG(FATAL, "Exception from synchronous thread_pool task: %s", e.what());
      ASSERT(false);
    }
    catch (...) {
      LOG(FATAL, "Exception from synchronous thread_pool task\n");
      ASSERT(false);
    }
  }
  // Keep it alive for the pool's lifetime so a caller holding the
  // returned pointer (e.g. to later try_pop() it) doesn't dangle,
  // even though the work is already done by the time we return.
  m_finished.push_back(std::move(work));
  return result;
}

bool thread_pool::try_pop(const work* w)
{
  // Already ran synchronously inside execute() -- nothing left to
  // cancel.
  (void)w;
  return false;
}

void thread_pool::wait_all()
{
  // Nothing async in flight; execute() already ran everything
  // before returning.
}

#else

thread_pool::thread_pool(const size_t n) : m_running(true), m_threads(n), m_doingWork(0)
{
  const std::unique_lock lock(m_mutex);
  for (size_t i = 0; i < n; ++i)
    m_threads[i] = std::thread([this] { worker(); });
}

thread_pool::~thread_pool()
{
  join_all();
}

const thread_pool::work* thread_pool::execute(std::function<void()>&& func)
{
  thread_pool::work_ptr work = std::make_unique<thread_pool::work>(std::move(func));
  const thread_pool::work* result = work.get();
  const std::unique_lock lock(m_mutex);
  ASSERT(m_running);
  m_work.push_back(std::move(work));
  m_cv.notify_one();
  return result;
}

bool thread_pool::try_pop(const work* w)
{
  std::unique_lock<std::mutex> lock(m_mutex);
  for (auto it = m_work.begin(); it != m_work.end(); ++it) {
    if (w == it->get()) {
      m_work.erase(it);
      return true;
    }
  }
  return false;
}

void thread_pool::wait_all()
{
  std::unique_lock<std::mutex> lock(m_mutex);
  m_cvWait.wait(lock,
                [this]() -> bool { return !m_running || (m_work.empty() && m_doingWork == 0); });
}

void thread_pool::join_all()
{
  {
    const std::unique_lock lock(m_mutex);
    m_running = false;
  }
  m_cv.notify_all();

  for (auto& j : m_threads) {
    try {
      if (j.joinable())
        j.join();
    }
    catch (const std::exception& ex) {
      LOG(FATAL, "Exception joining threads: %s\n", ex.what());
      ASSERT(false);
    }
    catch (...) {
      LOG(FATAL, "Exception joining threads\n");
      ASSERT(false);
    }
  }
}

void thread_pool::worker()
{
  bool running;
  {
    const std::unique_lock lock(m_mutex);
    running = m_running;
  }
  while (running) {
    std::function<void()> func;
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_cv.wait(lock, [this]() -> bool { return !m_running || !m_work.empty(); });
      running = m_running;
      if (m_running && !m_work.empty()) {
        func = std::move(m_work.front()->m_func);
        ++m_doingWork;
        m_work.pop_front();
      }
    }
    try {
      if (func)
        func();
    }
    // TODO handle exceptions in a better way
    catch (const std::exception& e) {
      LOG(FATAL, "Exception from worker: %s", e.what());
      ASSERT(false);
    }
    catch (...) {
      LOG(FATAL, "Exception from worker\n");
      ASSERT(false);
    }

    // Decrement m_doingWork only if we've incremented it
    if (func) {
      const std::unique_lock lock(m_mutex);
      --m_doingWork;
      m_cvWait.notify_all();
    }
  }
}

#endif // LAF_WASM

} // namespace base
