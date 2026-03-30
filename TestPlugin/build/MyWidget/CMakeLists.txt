cmake_minimum_required(VERSION 3.20)

# 设置 Qt 路径（必须在 project 之前）
list(APPEND CMAKE_PREFIX_PATH "C:/Qt/Qt5.12.9/5.12.9/msvc2017_64")

# Qt path can be provided by the builder or overridden by configure arguments.
list(APPEND CMAKE_PREFIX_PATH "C:/Qt/Qt5.12.9/5.12.9/msvc2017_64")

project(MyWidget LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

add_library(MyWidget MODULE
    MyWidget.h
    MyWidget.cpp
    metadata.json
)

target_link_libraries(MyWidget PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

target_compile_definitions(MyWidget PRIVATE QT_PLUGIN)
