cmake_minimum_required(VERSION 3.20)

project(XDemo
        LANGUAGES CXX
        DESCRIPTION "测试自定义控件")

if(MSVC)
    add_compile_options(/utf-8)
endif()

set(CMAKE_FIND_DEBUG_MODE TRUE)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets UiPlugin REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets UiPlugin REQUIRED)
if (QT_VERSION_MAJOR EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS Core5Compat)
endif()
# 部署函数
function(deploy_qt_target TARGET_NAME)
    if(NOT WIN32)
        return()
    endif()

    find_program(WINDEPLOYQT_EXECUTABLE
            NAMES windeployqt${QT_VERSION_MAJOR} windeployqt
            HINTS "${CMAKE_PREFIX_PATH}/bin")

    if(NOT WINDEPLOYQT_EXECUTABLE)
        return()
    endif()

    set(_deploy_mode "$<$<CONFIG:Debug>:--debug>$<$<NOT:$<CONFIG:Debug>>:--release>")

    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
            ${_deploy_mode}
            --compiler-runtime
            --no-plugins
            "$<TARGET_FILE:${TARGET_NAME}>"

            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
            ${_deploy_mode}
            --no-compiler-runtime
            --no-translations
            --no-libraries
            "$<TARGET_FILE:${TARGET_NAME}>"
            VERBATIM
    )
endfunction()

#指定相关的源代码，添加测试函数
function(add_test_func  main_src)
    cmake_parse_arguments(DONG_DONG "" "QRC;EXE_NAME" "SOURCE_DIR;MODULES" ${ARGN})
    if(DEFINED DONG_DONG_EXE_NAME)
        set(target_name ${DONG_DONG_EXE_NAME}  )
    else ()
        get_filename_component(target_name ${main_src} NAME_WE )
    ENDIF ()
    set(source_files "")
    foreach(dir IN LISTS DONG_DONG_SOURCE_DIR)
        file(GLOB_RECURSE dir_files
                CONFIGURE_DEPENDS
                "${dir}/*.cpp"
                "${dir}/*.c"
                "${dir}/*.h"
                "${dir}/*.hpp"
                "${dir}/*.ui"
                "${dir}/*.qrc"
        )
        list(APPEND source_files ${dir_files})
    endforeach()
    set(all_sources ${main_src} ${source_files})
    if(DONG_DONG_QRC)
        list(APPEND all_sources ${ARG_QRC})
    endif()
    add_executable(${target_name}  ${all_sources})

    target_include_directories(${target_name} PUBLIC ${DONG_DONG_SOURCE_DIR})
    target_include_directories(${target_name} PRIVATE
            "${CMAKE_SOURCE_DIR}/configure/$<CONFIG>"
    )
    message("add target  : ${target_name}")
    message("add sources  : ${all_sources}")

    target_link_libraries(${target_name} PRIVATE ${DONG_DONG_MODULES})

    set_target_properties(${target_name} PROPERTIES
            PREFIX ""
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/bin/$<CONFIG>"
    )

    if (QT_VERSION_MAJOR EQUAL 6)
        target_link_libraries(${target_name} PRIVATE Qt6::Core5Compat)
    endif()
    deploy_qt_target(${target_name})
endfunction()

add_test_func(test/FileSystem.cpp
        SOURCE_DIR  ${CMAKE_SOURCE_DIR}/src/FileSystem
        MODULES Qt${QT_VERSION_MAJOR}::Core)

add_test_func(test/ModernListWidget.cpp
        SOURCE_DIR  ${CMAKE_SOURCE_DIR}/src/ModernListWidget
        MODULES Qt${QT_VERSION_MAJOR}::Widgets)

add_test_func(test/ModernTabWidget.cpp
        SOURCE_DIR  ${CMAKE_SOURCE_DIR}/src/ModernTabWidget
        MODULES Qt${QT_VERSION_MAJOR}::Widgets)

add_test_func(test/NodeEditor.cpp
        SOURCE_DIR  ${CMAKE_SOURCE_DIR}/src/NodeEditor
        MODULES Qt${QT_VERSION_MAJOR}::Widgets)

add_test_func(test/PluginManagementView.cpp
        SOURCE_DIR  ${CMAKE_SOURCE_DIR}/src/PluginManagementView
        ${CMAKE_SOURCE_DIR}/src/TemplateRenderer
        MODULES Qt${QT_VERSION_MAJOR}::Widgets)


function(generate_configure_info output_dir)
    set(CMAKE_PATH "${CMAKE_COMMAND}")
    set(GENERATOR_TYPE "${CMAKE_GENERATOR}")
    set(PROJECT_ROOT_PATH "${CMAKE_SOURCE_DIR}")
    set(QT_PATH "")
    set(QT_PREFIX_PATH "")

    if(DEFINED Qt6_DIR AND NOT Qt6_DIR STREQUAL "")
        set(QT_PATH "${Qt6_DIR}")
    elseif(DEFINED Qt5_DIR AND NOT Qt5_DIR STREQUAL "")
        set(QT_PATH "${Qt5_DIR}")
    elseif(DEFINED CMAKE_PREFIX_PATH AND NOT CMAKE_PREFIX_PATH STREQUAL "")
        list(GET CMAKE_PREFIX_PATH 0 QT_PREFIX_PATH)
    endif()

    if(NOT QT_PATH STREQUAL "")
        cmake_path(GET QT_PATH PARENT_PATH _qt_tmp_1)
        cmake_path(GET _qt_tmp_1 PARENT_PATH _qt_tmp_2)
        cmake_path(GET _qt_tmp_2 PARENT_PATH QT_PREFIX_PATH)
    endif()

    set(_content [=[
#pragma once
namespace env_config
{
    inline constexpr auto qt_path = "@QT_PATH@";
    inline constexpr auto cmake_path = "@CMAKE_PATH@";
    inline constexpr auto project_root_path = "@PROJECT_ROOT_PATH@";
    inline constexpr auto qt_prefix_path = "@QT_PREFIX_PATH@";
    inline constexpr auto cmake_generator = "@GENERATOR_TYPE@";
    inline constexpr auto cmake_build_type = "$<CONFIG>";
}
]=])

    string(CONFIGURE "${_content}" _content @ONLY)

    file(GENERATE
            OUTPUT "${output_dir}/$<CONFIG>/configure_info.h"
            CONTENT "${_content}"
    )
endfunction()

generate_configure_info( ${CMAKE_SOURCE_DIR}/configure)

function(add_lib  lib_name)
    cmake_parse_arguments(LIB "" "SOURCE_DIR"  "MODULES"   ${ARGN})
    file(GLOB_RECURSE LIB_SOURCES
            CONFIGURE_DEPENDS
            "${LIB_SOURCE_DIR}/*.cpp"
            "${LIB_SOURCE_DIR}/*.c"
            "${LIB_SOURCE_DIR}/*.h"
            "${LIB_SOURCE_DIR}/*.hpp"
            "${LIB_SOURCE_DIR}/*.ui"
            "${LIB_SOURCE_DIR}/*.qrc"
    )
    add_library(${lib_name} ${LIB_SOURCES})
    target_link_libraries(${lib_name} ${LIB_MODULES})
    target_include_directories(${lib_name} PUBLIC ${LIB_SOURCE_DIR})
    message("add lib : ${lib_name},源代码目录在 :${LIB_SOURCE_DIR} ,包含了  ${LIB_SOURCES}")
endfunction()

add_lib(TemplateRenderer
        SOURCE_DIR   ${CMAKE_SOURCE_DIR}/src/TemplateRenderer
        MODULES "")
