# LAF OS
# Copyright (C) 2018-2025  Igara Studio S.A.
# Copyright (C) 2012-2018  David Capello

######################################################################
# Common source code

set(LAF_OS_SOURCES
  common/event_queue.cpp
  common/main.cpp
  common/system.cpp
  dnd.cpp
  error.cpp
  event.cpp
  none/system.cpp
  window.cpp)
if(WIN32)
  list(APPEND LAF_OS_SOURCES
    win/color_space.cpp
    win/dnd.cpp
    win/event_queue.cpp
    win/keys.cpp
    win/system.cpp
    win/winapi.cpp
    win/window.cpp
    win/window_dde.cpp
    win/wintab.cpp)

  # Support IME to input CJK characters
  IF (LAF_WITH_IME)
    list(APPEND LAF_OS_SOURCES
      win/ime_manager.cpp)
  endif()
elseif(APPLE)
  find_library(COCOA_LIBRARY Cocoa)
  find_library(CARBON_LIBRARY Carbon)
  if(COCOA_LIBRARY)
    list(APPEND LAF_OS_SOURCES
      osx/app.mm
      osx/app_delegate.mm
      osx/dnd.mm
      osx/color_space.mm
      osx/event_queue.mm
      osx/keys.mm
      osx/logger.mm
      osx/menus.mm
      osx/system.mm
      osx/view.mm
      osx/window.mm
      gl/gl_context_nsgl.mm)
  endif()
elseif(EMSCRIPTEN)
  # Milestone 1: real (if minimal) window/surface backend, using
  # SDL2 (Emscripten's built-in port, linked via -sUSE_SDL=2 below)
  # for the actual <canvas> and a plain software Surface (no Skia).
  list(APPEND LAF_OS_SOURCES
    wasm/system.cpp)
else()
  list(APPEND LAF_OS_SOURCES
    x11/dnd.cpp
    x11/event_queue.cpp
    x11/keys.cpp
    x11/monitor.cpp
    x11/screen.cpp
    x11/system.cpp
    x11/window.cpp
    x11/x11.cpp
    x11/xinput.cpp)
endif()

######################################################################
# Skia backend

if(LAF_BACKEND STREQUAL "skia")
  if(APPLE)
    add_definitions(-Wno-c++14-extensions)
  endif()
  list(APPEND LAF_OS_SOURCES
    skia/skia_color_space.cpp
    skia/skia_gl.cpp
    skia/skia_surface.cpp
    skia/skia_system.cpp
    skia/skia_window.cpp)
  if(WIN32)
    list(APPEND LAF_OS_SOURCES
      skia/skia_window_win.cpp)
  elseif(APPLE)
    list(APPEND LAF_OS_SOURCES
      skia/skia_window_osx.mm)
  else()
    list(APPEND LAF_OS_SOURCES
      skia/skia_window_x11.cpp)
  endif()
endif()

######################################################################

add_library(laf-os ${LAF_OS_SOURCES})
target_link_libraries(laf-os laf-gfx laf-base)
if(LAF_WITH_CLIP)
  target_link_libraries(laf-os clip)
  target_compile_definitions(laf-os PUBLIC
    LAF_WITH_CLIP)
endif()

set(LAF_OS_PLATFORM_LIBS)
if(WIN32)
  list(APPEND LAF_OS_PLATFORM_LIBS
    kernel32 user32 gdi32 comdlg32 ole32 winmm
    shlwapi psapi wininet comctl32 dbghelp dwmapi
    msimg32)

  # Support IME to input CJK characters
  IF (LAF_WITH_IME)
    target_compile_definitions(laf-os PUBLIC
      LAF_WITH_IME)
    list(APPEND LAF_OS_PLATFORM_LIBS
      imm32)
  endif()

  # Don't define min/max() macros when including <windows.h>
  target_compile_options(laf-os PRIVATE -DNOMINMAX)

  # To include wacom header files
  target_include_directories(laf-os PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../third_party)
elseif(APPLE)
  if(COCOA_LIBRARY)
    list(APPEND LAF_OS_PLATFORM_LIBS
      ${COCOA_LIBRARY}
      ${CARBON_LIBRARY})
  endif()

  target_compile_options(laf-os PRIVATE
    -Wno-ignored-attributes
    -Wno-unused-result
    -Wno-deprecated-declarations
    -Werror=inconsistent-missing-override
    # Use Automatic Reference Counting
    -fobjc-arc)
elseif(EMSCRIPTEN)
  # Milestone 4: stb_image (public domain, single header) decodes
  # the PNGs we load via loadRgbaSurface() -- e.g. sprite-sheet
  # fonts. No Emscripten port needed, no extra libs to link.
  target_include_directories(laf-os PRIVATE ${PROJECT_SOURCE_DIR}/third_party/stb)
  target_compile_options(laf-os PUBLIC -sUSE_SDL=2)
  target_link_options(laf-os PUBLIC -sUSE_SDL=2)
else()
  find_package(Threads REQUIRED)
  list(APPEND LAF_OS_PLATFORM_LIBS ${CMAKE_THREAD_LIBS_INIT})

  find_package(X11 REQUIRED)
  target_include_directories(laf-os PRIVATE ${X11_INCLUDE_DIR})
  list(APPEND LAF_OS_PLATFORM_LIBS ${X11_LIBRARIES})
  if(NOT X11_Xcursor_FOUND)
    message(FATAL_ERROR "Xcursor library not found")
  endif()
  if(NOT X11_Xinput_FOUND)
    message(FATAL_ERROR "Xinput library not found")
  endif()
  if(NOT X11_Xrandr_FOUND)
    message(FATAL_ERROR "Xrandr library not found")
  endif()
  list(APPEND LAF_OS_PLATFORM_LIBS ${X11_Xcursor_LIB} ${X11_Xrandr_LIB})

  check_library_exists(X11 XOpenIM "${X11_LIB_SEARCH_PATH}" XIM_FOUND)

  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(laf-os PRIVATE -Werror=suggest-override)
  endif()
endif()

target_link_libraries(laf-os ${LAF_OS_PLATFORM_LIBS})

if(LAF_BACKEND STREQUAL "skia")
  target_link_libraries(laf-os skia)
endif()

set(_LAF_BACKEND_INITIAL_LINK_FLAGS "")
if(MSVC)
  # Linking with "wsetargv.obj" to add support to expand filename
  # wildcards in argc/argv.
  if(LAF_BACKEND STREQUAL "skia")
    set(_LAF_BACKEND_INITIAL_LINK_FLAGS "-LINK wsetargv.obj -ENTRY:\"wWinMainCRTStartup\"")
  else()
    set(_LAF_BACKEND_INITIAL_LINK_FLAGS "-LINK wsetargv.obj -ENTRY:\"wmainCRTStartup\"")
  endif()
elseif(EMSCRIPTEN)
  # Milestone 6: ui::Manager::run() (and other laf/app code) has
  # real blocking loops -- ASYNCIFY lets Emscripten transform them
  # to cooperatively yield back to the browser's event loop instead
  # of freezing the tab, without a full rewrite into a callback
  # style. Paired with an emscripten_sleep(0) yield point inside
  # Manager::run()'s loop (see src/ui/manager.cpp).
  set(_LAF_BACKEND_INITIAL_LINK_FLAGS
    "-sUSE_SDL=2 -sALLOW_MEMORY_GROWTH=1 -sEXIT_RUNTIME=0 -sASYNCIFY=1")
endif()
set(LAF_BACKEND_LINK_FLAGS "${_LAF_BACKEND_INITIAL_LINK_FLAGS}"
  CACHE STRING "Extra flags to link an application that uses laf-os")
mark_as_advanced(LAF_BACKEND_LINK_FLAGS)

if(EMSCRIPTEN)
  # Milestone 1-4 smoke test: prove pixels reach an actual browser
  # <canvas>, then input, then real font rendering. Deliberately not
  # using laf_add_example() from examples/CMakeLists.txt since that
  # assumes a desktop-style asset layout; we preload our own tiny
  # synthetic font sheet below instead.
  add_executable(laf-wasm-smoketest wasm/smoketest_main.cpp)
  target_link_libraries(laf-wasm-smoketest laf-os laf-text laf-gfx laf-base)
  set_target_properties(laf-wasm-smoketest PROPERTIES
    LINK_FLAGS "-sUSE_SDL=2 -sALLOW_MEMORY_GROWTH=1 -sEXIT_RUNTIME=0 -sSINGLE_FILE=1 --embed-file ${CMAKE_CURRENT_SOURCE_DIR}/wasm/assets/wasm_test_font.png@wasm_test_font.png")
endif()

if(LAF_WITH_TESTS)
  laf_find_tests(. laf-os)
endif()
