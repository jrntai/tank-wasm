cmake_minimum_required(VERSION 3.20)
project(tank CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Platform-independent game simulation: no GL, no Emscripten headers.
add_library(sim STATIC
  src/sim/world.cpp
  src/sim/stages.cpp
  src/sim/app.cpp
)
target_include_directories(sim PUBLIC src)
target_compile_options(sim PRIVATE -Wall -Wextra -Wpedantic)

if(EMSCRIPTEN)
  add_executable(tank
    src/platform/main.cpp
    src/platform/renderer.cpp
    src/platform/sprites.cpp
  )
  target_link_libraries(tank PRIVATE sim)
  target_include_directories(tank PRIVATE src)
  target_compile_options(tank PRIVATE -Wall -Wextra)
  # Only the game's files go in web/, the folder the server exposes (WEB-11).
  set_target_properties(tank PROPERTIES
    SUFFIX ".html"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/web
  )
  target_link_options(tank PRIVATE
    -sUSE_WEBGL2=1
    -sMIN_WEBGL_VERSION=2
    -sMAX_WEBGL_VERSION=2
    -sALLOW_MEMORY_GROWTH=1
    $<$<CONFIG:Debug>:-sASSERTIONS=2>
    $<$<CONFIG:Debug>:-gsource-map>
  )

  # `cmake --build build-web --target serve`: rebuild, then serve on http://127.0.0.1:8080/
  find_program(NODE_EXECUTABLE node)
  if(NODE_EXECUTABLE)
    add_custom_target(serve
      COMMAND ${NODE_EXECUTABLE} ${CMAKE_SOURCE_DIR}/server/server.mjs --dir ${CMAKE_BINARY_DIR}/web
      DEPENDS tank
      USES_TERMINAL
    )
  endif()
else()
  # Native build: unit tests for the simulation only.
  enable_testing()
  add_executable(sim_tests
    tests/main.cpp
    tests/arena_tests.cpp
    tests/player_tests.cpp
    tests/bullet_tests.cpp
    tests/enemy_tests.cpp
    tests/game_tests.cpp
    tests/menu_tests.cpp
    tests/progression_tests.cpp
    tests/powerup_tests.cpp
    tests/select_tests.cpp
  )
  target_link_libraries(sim_tests PRIVATE sim)
  target_compile_options(sim_tests PRIVATE -Wall -Wextra)
  # One CTest entry per spec; each runs the tests whose IDs start with that prefix.
  foreach(spec ARN PLY BUL ENM GAM MNU PRG PWR SEL)
    add_test(NAME ${spec} COMMAND sim_tests ${spec})
  endforeach()

  # Web server tests (specs/11-web-server.md) run under Node's built-in test runner.
  find_program(NODE_EXECUTABLE node)
  if(NODE_EXECUTABLE)
    add_test(NAME WEB COMMAND ${NODE_EXECUTABLE} --test ${CMAKE_SOURCE_DIR}/server/test/server.test.mjs)
  endif()
endif()
