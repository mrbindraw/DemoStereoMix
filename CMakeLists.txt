cmake_minimum_required(VERSION 3.19)
project(DemoStereoMix LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(DemoStereoMix
    WIN32 MACOSX_BUNDLE
    main.cpp
    sysaudio.cpp
    sysaudio.h
    PolicyConfig.h
    widget.cpp
    widget.h
    widget.ui
)

target_link_libraries(DemoStereoMix
    PRIVATE
        Qt::Core
        Qt::Widgets
)

include(GNUInstallDirs)

install(TARGETS DemoStereoMix
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET DemoStereoMix
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
