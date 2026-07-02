cmake_minimum_required(VERSION 3.19)
project(MyShootingGame LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(MyShootingGame
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    Bullet.h
    Chest.h
    GameObject.h
    Player.h
    Weapon.h
    Enemy.h

)

target_link_libraries(MyShootingGame
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS MyShootingGame
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET MyShootingGame
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
