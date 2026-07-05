cmake_minimum_required(VERSION 3.16)

project(TheLevelTwo VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(TheLevelTwo
    Codes/main.cpp
    Codes/mainwindow.cpp
    Codes/mainwindow.h
    Codes/mainwindow.ui

    Codes/GameObject.h
    Codes/Player.h
    Codes/Enemy.h
    Codes/Bullet.h
    Codes/PowerUp.h
    Codes/Weapon.h
    Codes/Chest.h
    Codes/ParticleEffect.h
    Codes/SaveManager.h
)

target_link_libraries(TheLevelTwo
    PRIVATE
        Qt6::Widgets
)
