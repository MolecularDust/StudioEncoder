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
# On Linux cmake/linux/defaults.cmake may already have created OBS::obs-frontend-api from the system OBS packages
if(NOT TARGET OBS::obs-frontend-api)
  find_package(obs-frontend-api REQUIRED)
endif()
find_package(Qt6 REQUIRED COMPONENTS Widgets)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs OBS::obs-frontend-api Qt6::Widgets FFmpeg::avcodec FFmpeg::avutil)

# Rust ProRes backend, built as a static library behind a small C ABI (see rust/src/lib.rs).
find_program(CARGO_EXECUTABLE cargo HINTS "$ENV{USERPROFILE}/.cargo/bin" "$ENV{HOME}/.cargo/bin" REQUIRED)
set(RUST_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/rust/target/release")
if(WIN32)
  set(RUST_LIB "${RUST_LIB_DIR}/studioencoder_rs.lib")
else()
  set(RUST_LIB "${RUST_LIB_DIR}/libstudioencoder_rs.a")
endif()
add_custom_target(
  studioencoder_rs_build
  COMMAND "${CARGO_EXECUTABLE}" build --release --manifest-path "${CMAKE_CURRENT_SOURCE_DIR}/rust/Cargo.toml"
  BYPRODUCTS "${RUST_LIB}"
  COMMENT "Building Rust ProRes backend"
  USES_TERMINAL
)
add_dependencies(${CMAKE_PROJECT_NAME} studioencoder_rs_build)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE "${RUST_LIB}")
# system libraries required by the Rust standard library
if(WIN32)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ws2_32 userenv ntdll bcrypt advapi32 kernel32)
elseif(UNIX AND NOT APPLE)
  find_package(Threads REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Threads::Threads ${CMAKE_DL_LIBS} m rt)
endif()

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE src/plugin-main.c src/encoder-common.c src/encoder-common.h src/encoder-rust.c src/encoder-oxideav.c src/encoder-ffmpeg.c src/prores-rs.h src/settings.c src/settings.h src/settings-dialog.cpp src/prores-aw.c src/prores-aw.h src/encoder-awplus.c
)

# 10-bit 4:2:2 (P210) capture source: DirectShow, Windows only
if(WIN32)
  target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/capture-p210.cpp)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE strmiids ole32 oleaut32)
endif()

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

# Plugin Manager metadata (Tools > Plugin Manager reads the version from data/manifest.json).
if(WIN32)
  set(PLUGIN_OS_ARCH "windows-x64")
else()
  set(PLUGIN_OS_ARCH "linux-x86_64")
endif()
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/manifest.json.in" "${CMAKE_CURRENT_BINARY_DIR}/manifest.json" @ONLY)
if(WIN32)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/manifest.json" DESTINATION "${CMAKE_PROJECT_NAME}/data" COMPONENT Runtime)
else()
  # same place the template installs data/ on Linux (cmake/linux/helpers.cmake)
  install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/manifest.json"
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/obs/obs-plugins/${CMAKE_PROJECT_NAME}"
    COMPONENT Runtime
  )
endif()

# Test program for the AW+ ProRes encoder core (not part of the plugin; build with --target prores-aw-test).
add_executable(prores-aw-test EXCLUDE_FROM_ALL tools/prores-aw-test.c src/prores-aw.c)
