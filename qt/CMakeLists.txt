cmake_minimum_required(VERSION 3.24)

project(SederVideoRewrapQt VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Network Qml Quick QuickControls2 Widgets Test)

# Stamp PROJECT_VERSION into a header so every translation unit sees the same
# string. configure_file is bulletproof across all CMake generators (Ninja,
# MSBuild, Xcode), unlike target_compile_definitions with embedded quotes.
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/seder_version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/seder_version.h
    @ONLY
)

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
    src/RecentFilesModel.cpp
    src/RustBridge.cpp
    src/SegmentCommands.cpp
    src/SegmentTableModel.cpp
    src/UiSettings.cpp
    src/UpdateChecker.cpp
    ${SEDER_APP_PLATFORM_SOURCES}
)

add_dependencies(seder-video-rewrap seder_video_rewrap_core)

target_include_directories(seder-video-rewrap PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(seder-video-rewrap PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
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

# Theme and Icons are QML singletons: pragma Singleton in the file plus
# this source property (which writes the singleton line into the
# generated qmldir) are BOTH required on Qt 6.4.
set_source_files_properties(qml/Theme.qml qml/Icons.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(seder-video-rewrap
    URI Seder.VideoRewrap
    VERSION 1.0
    RESOURCE_PREFIX "/qt/qml"
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/Icons.qml
        qml/SIcon.qml
        qml/SButton.qml
        qml/SIconButton.qml
        qml/SCheckBox.qml
        qml/SField.qml
        qml/SSlider.qml
        qml/SScrollBar.qml
        qml/SProgressBar.qml
        qml/SPill.qml
        qml/SToolTip.qml
        qml/STimecodeLabel.qml
        qml/SPanel.qml
        qml/SPanelTabs.qml
        qml/SMenu.qml
        qml/SMenuItem.qml
        qml/SMenuSeparator.qml
        qml/SDialog.qml
        qml/SToast.qml
        qml/ToastHost.qml
        qml/AppMenuBar.qml
        qml/HeaderBar.qml
        qml/UpdateBanner.qml
        qml/StatusBar.qml
        qml/WelcomeView.qml
        qml/ProjectPanel.qml
        qml/ExportPanel.qml
        qml/MonitorPanel.qml
        qml/TransportBar.qml
        qml/SegmentsPanel.qml
        qml/AboutDialog.qml
        qml/FfmpegSettingsDialog.qml
        qml/ErrorLogDialog.qml
        qml/ShortcutsDialog.qml
)

qt_add_resources(seder-video-rewrap "branding"
    PREFIX "/branding"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/../assets"
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/../assets/icon-64.png"
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
    src/RecentFilesModel.cpp
    src/RustBridge.cpp
    src/SegmentCommands.cpp
    src/SegmentTableModel.cpp
    src/UpdateChecker.cpp
)

add_dependencies(app-controller-tests seder_video_rewrap_core)

target_include_directories(app-controller-tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(app-controller-tests PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
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
