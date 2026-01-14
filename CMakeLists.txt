cmake_minimum_required(VERSION 3.20)

project(node_editor
        LANGUAGES CXX)

# 指定C和C++的编译标志，使用UTF-8字符集
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fexec-charset=UTF-8")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexec-charset=UTF-8")
set(CMAKE_PREFIX_PATH  "C:/Qt/Qt5.12.12/5.12.12/mingw73_64")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets UiPlugin REQUIRED)

add_library(NodeEditor SHARED
        include/NodeEditor.cpp
)

target_link_libraries(NodeEditor
        PUBLIC
        Qt5::Widgets
        Qt5::UiPlugin
        Qt5::Core
)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR})
set_target_properties(NodeEditor PROPERTIES PREFIX ""
AUTOMOC ON)

function(add_test_module_func file)
    get_filename_component(filename ${file} NAME_WE)
    add_executable(${filename} ${file})
    target_link_libraries(${filename}  PRIVATE Qt5::Widgets Qt5::UiPlugin  Qt5::Core)
endfunction()

add_test_module_func(test/module1.cpp)
add_test_module_func(test/module2.cpp)
add_test_module_func(test/module3.cpp)
add_test_module_func(test/module4.cpp)
add_executable(test  test/test.cpp include/main.hpp)

add_test_module_func(test/module5.cpp)
target_link_libraries(test  PRIVATE Qt5::Widgets Qt5::UiPlugin  Qt5::Core)

get_target_property(QT_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_BIN_DIR}")
message("${WINDEPLOYQT_EXECUTABLE}")
# 添加部署命令
add_custom_command(TARGET test POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
        --no-compiler-runtime
        --dir "${CMAKE_CURRENT_BINARY_DIR}"
        "$<TARGET_FILE:test>"
        COMMENT "Deploying Qt libraries..."
)

add_subdirectory(plugins)
add_subdirectory(app)