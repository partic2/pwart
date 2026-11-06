
cmake_minimum_required(VERSION 3.15)

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif()

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

project(pwart)


option(PWART_SYSLIB_ENABLED "compile pwart_syslib, require some dependencies. See deps/pull_deps.py" OFF)

if("${DEPS_SOURCE_DIRS}" STREQUAL "")
  if(NOT ("$ENV{DEPS_SOURCE_DIRS}" STREQUAL ""))
    set(DEPS_SOURCE_DIRS $ENV{DEPS_SOURCE_DIRS})
  else()
    get_filename_component(DEPS_SOURCE_DIRS deps ABSOLUTE)
  endif()
endif()

option(BUILD_TESTING "enable test" OFF)


if(BUILD_TESTING)
add_compile_options(-ggdb3 -Wall)
add_definitions(-DDEBUG_BUILD=1)
endif()

add_library(pwart STATIC pwart/pwart.c sljit/sljit_src/sljitLir.c)
target_link_libraries(pwart m)
target_include_directories(pwart PUBLIC include sljit/sljit_src)


if (PWART_SYSLIB_ENABLED)
add_library(pwart_syslib STATIC pwart_syslib/pwart_syslib.c)
add_executable(pwart_wasiexec pwart_syslib/wasiexec.c)
if (NOT DEPS_SOURCE_DIRS)
get_filename_component(DEPS_SOURCE_DIRS .. ABSOLUTE)
endif()


if(NOT TARGET uv_a)
add_subdirectory("${DEPS_SOURCE_DIRS}/libuv" ./build-libuv EXCLUDE_FROM_ALL)
endif()
if(NOT TARGET simpleffi)
add_subdirectory("${DEPS_SOURCE_DIRS}/simpleffi" ./build-simpleffi EXCLUDE_FROM_ALL)
endif()
if(NOT TARGET uvwasi_a)
add_subdirectory("${DEPS_SOURCE_DIRS}/uvwasi" ./build-uvwasi EXCLUDE_FROM_ALL)
endif()
# uvwasi mark the header as PRIVATE(Strange), So we need include it manually.
target_include_directories(pwart_syslib PUBLIC "${DEPS_SOURCE_DIRS}/uvwasi/include")
target_include_directories(pwart_wasiexec PUBLIC "${DEPS_SOURCE_DIRS}/uvwasi/include")


target_link_libraries(pwart_syslib PUBLIC pwart uvwasi_a uv_a simpleffi)

target_link_libraries(pwart_wasiexec pwart_syslib)
endif(PWART_SYSLIB_ENABLED)

set(WAT2WASM $ENV{WAT2WASM})

if (NOT WAT2WASM)
find_program(WAT2WASM wat2wasm)
endif()



if (BUILD_TESTING)
  enable_testing()
  execute_process(COMMAND ${WAT2WASM} "test1.wat" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests" COMMAND_ECHO STDOUT)
  execute_process(COMMAND ${WAT2WASM} "extension1.wat" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests" COMMAND_ECHO STDOUT)
  execute_process(COMMAND ${WAT2WASM} "unary.wat" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests" COMMAND_ECHO STDOUT)
  execute_process(COMMAND ${WAT2WASM} "benchsort.wat" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests" COMMAND_ECHO STDOUT)
  execute_process(COMMAND ${WAT2WASM} "binary.wat" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests" COMMAND_ECHO STDOUT)
  execute_process(COMMAND ${WAT2WASM} "control.wat" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests" COMMAND_ECHO STDOUT)
  execute_process(COMMAND ${WAT2WASM} "convert.wat" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests" COMMAND_ECHO STDOUT)
  execute_process(COMMAND ${WAT2WASM} "compare.wat" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests" COMMAND_ECHO STDOUT)
  add_executable(pwarttest tests/testmain.c)
  add_executable(pwarttest2 tests/testmain2.c)
  target_link_libraries(pwarttest pwart)
  target_link_libraries(pwarttest2 pwart)
  add_test(NAME test1 COMMAND pwarttest WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests")
  add_test(NAME test2 COMMAND pwarttest2 WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests")

  if(PWART_SYSLIB_ENABLED)
  execute_process(COMMAND ${WAT2WASM} "syslibtest1.wat" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests" COMMAND_ECHO STDOUT)
  add_library(ffitestdll SHARED tests/ffitestdll.c)
  add_executable(syslibtest1 tests/syslibtest1.c)
  target_link_libraries(syslibtest1 pwart pwart_syslib)
  add_test(NAME syslibtest1 COMMAND syslibtest1 $<TARGET_FILE:ffitestdll> WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests")
  endif()

endif()