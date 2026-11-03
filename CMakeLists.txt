cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/finders")

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
find_package(FFmpeg REQUIRED COMPONENTS avcodec avutil)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs FFmpeg::avcodec FFmpeg::avutil)

# Rust ProRes backend, built as a static library behind a small C ABI (see rust/src/lib.rs).
find_program(CARGO_EXECUTABLE cargo HINTS "$ENV{USERPROFILE}/.cargo/bin" "$ENV{HOME}/.cargo/bin" REQUIRED)
set(RUST_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/rust/target/release")
if(WIN32)
  set(RUST_LIB "${RUST_LIB_DIR}/obs_prores_rs.lib")
else()
  set(RUST_LIB "${RUST_LIB_DIR}/libobs_prores_rs.a")
endif()
add_custom_target(
  obs_prores_rs_build
  COMMAND "${CARGO_EXECUTABLE}" build --release --manifest-path "${CMAKE_CURRENT_SOURCE_DIR}/rust/Cargo.toml"
  BYPRODUCTS "${RUST_LIB}"
  COMMENT "Building Rust ProRes backend"
  USES_TERMINAL
)
add_dependencies(${CMAKE_PROJECT_NAME} obs_prores_rs_build)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE "${RUST_LIB}")
if(WIN32)
  # system libraries required by the Rust standard library
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ws2_32 userenv ntdll bcrypt advapi32 kernel32)
endif()

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE src/plugin-main.c src/encoder-common.c src/encoder-common.h src/encoder-rust.c src/encoder-oxideav.c src/encoder-ffmpeg.c src/prores-rs.h
)

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})
