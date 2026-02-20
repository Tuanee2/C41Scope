cmake_minimum_required(VERSION 3.16)

project(C41Scope VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Quick Qml)

qt_standard_project_setup(REQUIRES 6.5)
qt_policy(SET QTP0004 NEW)

set(APP_SOURCES
    main.cpp
    src/core/scope_types.h
    src/core/iscope_data_store.h
    src/core/iscope_data_api.h
    src/core/tools_interfaces.h
    src/store/history_store.h
    src/store/history_store.cpp
    src/store/frame_store.h
    src/store/frame_store.cpp
    src/scope_controller.h
    src/scope_controller.cpp
    src/scope_view.h
    src/scope_view.cpp
    src/demo/fake_data_generator.h
    src/demo/fake_data_generator.cpp
    src/tools/view/callback_view_tool.h
    src/tools/view/callback_view_tool.cpp
    src/tools/command/stats_command_tool.h
    src/tools/command/stats_command_tool.cpp
)

qt_add_executable(appC41Scope
    ${APP_SOURCES}
)

target_include_directories(appC41Scope
    PRIVATE
        src
)

qt_add_qml_module(appC41Scope
    URI C41Scope
    VERSION 1.0
    QML_FILES
        Main.qml
        qml/scopeui/ScopeWorkbench.qml
        qml/scopeui/ScopeSplitGrid.qml
        qml/scopeui/ScopeToolbar.qml
        qml/scopeui/ScopeSplitPicker.qml
        qml/scopeui/ScopeCommandMenu.qml
        qml/scopeui/ScopeCommandResultDialog.qml
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appC41Scope PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appC41Scope
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appC41Scope
    PRIVATE
        Qt6::Quick
        Qt6::Qml
)

include(GNUInstallDirs)
install(TARGETS appC41Scope
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
