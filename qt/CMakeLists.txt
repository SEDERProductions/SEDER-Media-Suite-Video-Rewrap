cmake_minimum_required(VERSION 3.24)

project(SederVideoRewrapQt VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Test)

set(RUST_TARGET_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../target)
set(RUST_CORE_LIBRARY ${RUST_TARGET_DIR}/release/${CMAKE_STATIC_LIBRARY_PREFIX}seder_video_rewrap${CMAKE_STATIC_LIBRARY_SUFFIX})

add_custom_target(seder_video_rewrap_core ALL
    COMMAND cargo build --release --lib
    BYPRODUCTS ${RUST_CORE_LIBRARY}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
    COMMENT "Building Rust video rewrap core"
    VERBATIM
)

qt_add_executable(seder-video-rewrap
    src/main.cpp
    src/AppController.cpp
    src/RustBridge.cpp
    src/SegmentTableModel.cpp
)

add_dependencies(seder-video-rewrap seder_video_rewrap_core)

target_include_directories(seder-video-rewrap PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(seder-video-rewrap PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    ${RUST_CORE_LIBRARY}
)

if(APPLE)
    target_link_libraries(seder-video-rewrap PRIVATE
        "-framework Security"
        "-framework SystemConfiguration"
    )
endif()

qt_add_qml_module(seder-video-rewrap
    URI Seder.VideoRewrap
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

enable_testing()

qt_add_executable(segment-model-tests
    tests/SegmentTableModelTests.cpp
    src/RustBridge.cpp
    src/SegmentTableModel.cpp
)

add_dependencies(segment-model-tests seder_video_rewrap_core)

target_include_directories(segment-model-tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(segment-model-tests PRIVATE
    Qt6::Core
    Qt6::Test
    ${RUST_CORE_LIBRARY}
)

add_test(NAME segment-model-tests COMMAND segment-model-tests)
