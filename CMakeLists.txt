
cmake_minimum_required(VERSION 3.12)

project(pwart)

option(PWART_SYSLIB_ENABLED "compile pwart_syslib, require uv_a and libffi_a defined" OFF)
option(BUILD_TESTING "enable test" OFF)

option(DEPS_SOURCE_DIRS "dependency sources directory, where DEPS_SOURCE_DIRS/libffi DEPS_SOURCE_DIRS/libuv exists, if defined, add these as subdirectory.")


add_library(pwart STATIC pwart/pwart.c sljit/sljit_src/sljitLir.c)
target_include_directories(pwart PUBLIC include sljit/sljit_src)


if (PWART_SYSLIB_ENABLED)
if (DEPS_SOURCE_DIRS)
add_subdirectory("${DEPS_SOURCE_DIRS}/libffi" build-libffi)
add_subdirectory("${DEPS_SOURCE_DIRS}/libuv" build-libuv)
endif()
add_library(pwart_syslib STATIC pwart_syslib/pwart_syslib.c)
target_link_libraries(pwart_syslib PUBLIC pwart uv_a ffi_static)
endif()

set(WAT2WASM $ENV{WAT2WASM})

if (NOT WAT2WASM)
find_program(WAT2WASM wat2wasm)
endif()



if (BUILD_TESTING)
  enable_testing()
  execute_process(COMMAND ${WAT2WASM} "--enable-multi-memory" "test1.wat" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests" COMMAND_ECHO STDOUT)
  execute_process(COMMAND ${WAT2WASM} "--enable-multi-memory" "extension1.wat" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests" COMMAND_ECHO STDOUT)
  execute_process(COMMAND ${WAT2WASM} "unary.wat" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests" COMMAND_ECHO STDOUT)
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
endif()