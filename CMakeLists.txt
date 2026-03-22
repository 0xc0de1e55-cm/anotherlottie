cmake_minimum_required(VERSION 3.16)

project(anotherlottie VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED
    COMPONENTS
        Quick
        Concurrent
)

qt_standard_project_setup(REQUIRES 6.8)

set(BUILD_SHARED_LIBS        OFF CACHE BOOL "" FORCE)
set(LOTTIE_MODULE            OFF CACHE BOOL "" FORCE)
set(LOTTIE_THREAD            ON  CACHE BOOL "" FORCE)
set(LOTTIE_CACHE             ON  CACHE BOOL "" FORCE)
add_subdirectory(rlottie)

set(APP_SOURCES
    src/AnotherLottie.cpp
    src/AnotherLottie.h
)

qt_add_executable(appanotherlottie
    main.cpp

    ${APP_SOURCES}
)

qt_add_qml_module(appanotherlottie
    URI anotherlottie
    VERSION 1.0
    QML_FILES
        Main.qml
    SOURCES
        ${APP_SOURCES}
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appanotherlottie PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appanotherlottie
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_include_directories(appanotherlottie
    PRIVATE
        ./src
        # ${CMAKE_CURRENT_SOURCE_DIR}/libs/include/
)

target_link_libraries(appanotherlottie
    PRIVATE
        Qt6::Quick
        Qt6::Concurrent
        rlottie
        # ${CMAKE_CURRENT_SOURCE_DIR}/libs/librlottie.a
)

include(GNUInstallDirs)
install(TARGETS appanotherlottie
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
