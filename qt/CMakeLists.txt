cmake_minimum_required(VERSION 3.24)

project(SederVideoRewrapQt VERSION 0.1.28 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Widgets Test)

set(RUST_TARGET_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../target)
set(RUST_CORE_LIBRARY ${RUST_TARGET_DIR}/release/${CMAKE_STATIC_LIBRARY_PREFIX}seder_video_rewrap${CMAKE_STATIC_LIBRARY_SUFFIX})

add_custom_target(seder_video_rewrap_core ALL
    COMMAND cargo build --release --lib
    BYPRODUCTS ${RUST_CORE_LIBRARY}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
    COMMENT "Building Rust video rewrap core"
    VERBATIM
)

set(SEDER_APP_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../assets/icon.ico")
set(SEDER_APP_ICON_MACOS "${CMAKE_CURRENT_SOURCE_DIR}/../assets/icon.icns")

set(SEDER_APP_PLATFORM_SOURCES "")
if(APPLE AND EXISTS "${SEDER_APP_ICON_MACOS}")
    set_source_files_properties("${SEDER_APP_ICON_MACOS}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")
    list(APPEND SEDER_APP_PLATFORM_SOURCES "${SEDER_APP_ICON_MACOS}")
endif()
if(WIN32 AND EXISTS "${SEDER_APP_ICON_PATH}")
    # Copy the icon next to the generated app.rc so RC.exe sees a simple
    # relative path without any forward/backslash translation issues.
    file(COPY "${SEDER_APP_ICON_PATH}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    set(SEDER_ICON_PATH "icon.ico")
    configure_file(app.rc.in app.rc)
    list(APPEND SEDER_APP_PLATFORM_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/app.rc")
endif()

qt_add_executable(seder-video-rewrap
    MACOSX_BUNDLE
    WIN32
    src/main.cpp
    src/AppController.cpp
    src/ExportEngine.cpp
    src/ProbeEngine.cpp
    src/ProcessUtil.cpp
    src/RustBridge.cpp
    src/SegmentTableModel.cpp
    ${SEDER_APP_PLATFORM_SOURCES}
)

add_dependencies(seder-video-rewrap seder_video_rewrap_core)

target_include_directories(seder-video-rewrap PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(seder-video-rewrap PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Widgets
    ${RUST_CORE_LIBRARY}
)

if(WIN32)
    # Rust static libraries on MSVC depend on Windows system libs that rustc
    # auto-injects for executables but not for staticlib consumers.
    target_link_libraries(seder-video-rewrap PRIVATE
        ntdll ws2_32 userenv bcrypt
    )
endif()

if(APPLE)
    target_link_libraries(seder-video-rewrap PRIVATE
        "-framework Security"
        "-framework SystemConfiguration"
    )
    set_target_properties(seder-video-rewrap PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER com.sederproductions.media-suite.video-rewrap
        MACOSX_BUNDLE_BUNDLE_NAME "SEDER Media Suite Video Rewrap"
        MACOSX_BUNDLE_ICON_FILE icon.icns
    )
endif()

qt_add_qml_module(seder-video-rewrap
    URI Seder.VideoRewrap
    VERSION 1.0
    RESOURCE_PREFIX "/qt/qml"
    QML_FILES
        qml/Main.qml
)

if(Qt6_VERSION VERSION_GREATER_EQUAL "6.5.0")
    qt_generate_deploy_qml_app_script(
        TARGET seder-video-rewrap
        OUTPUT_SCRIPT seder_deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
else()
    qt_generate_deploy_qml_app_script(
        TARGET seder-video-rewrap
        FILENAME_VARIABLE seder_deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
endif()
install(SCRIPT ${seder_deploy_script})

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

if(WIN32)
    target_link_libraries(segment-model-tests PRIVATE
        ntdll ws2_32 userenv bcrypt
    )
endif()

add_test(NAME segment-model-tests COMMAND segment-model-tests)

qt_add_executable(app-controller-tests
    tests/AppControllerTests.cpp
    src/AppController.cpp
    src/ExportEngine.cpp
    src/ProbeEngine.cpp
    src/ProcessUtil.cpp
    src/RustBridge.cpp
    src/SegmentTableModel.cpp
)

add_dependencies(app-controller-tests seder_video_rewrap_core)

target_include_directories(app-controller-tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(app-controller-tests PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
    ${RUST_CORE_LIBRARY}
)

if(WIN32)
    target_link_libraries(app-controller-tests PRIVATE
        ntdll ws2_32 userenv bcrypt
    )
endif()

if(APPLE)
    target_link_libraries(app-controller-tests PRIVATE
        "-framework Security"
        "-framework SystemConfiguration"
    )
endif()

add_test(NAME app-controller-tests COMMAND app-controller-tests)
set_tests_properties(app-controller-tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
