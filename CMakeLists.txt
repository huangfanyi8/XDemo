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
    cmake_parse_arguments(DONG_DONG "" "QRC" "SOURCE_DIR;MODULES" ${ARGN})
    get_filename_component(target_name ${main_src} NAME_WE )
    file(GLOB_RECURSE source_files
            CONFIGURE_DEPENDS
            "${DONG_DONG_SOURCE_DIR}/*.cpp"
            "${DONG_DONG_SOURCE_DIR}/*.c"
            "${DONG_DONG_SOURCE_DIR}/*.h"
            "${DONG_DONG_SOURCE_DIR}/*.hpp"
            "${DONG_DONG_SOURCE_DIR}/*.ui"
            "${DONG_DONG_SOURCE_DIR}/*.qrc"
    )
    set(all_sources ${main_src} ${source_files})
    if(ARG_QRC)
        list(APPEND all_sources ${ARG_QRC})
    endif()
    add_executable(${target_name}  ${all_sources})

    target_include_directories(${target_name} PUBLIC ${DONG_DONG_SOURCE_DIR})

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
        SOURCE_DIR    ${CMAKE_SOURCE_DIR}/src/PluginManagementView
        MODULES  Qt${QT_VERSION_MAJOR}::Widgets  Qt${QT_VERSION_MAJOR}::UiPlugin)

# TemplateRenderer 测试（可执行文件命名为 TestTemplateRenderer 避免与库冲突）
add_executable(TestTemplateRenderer
        test/TemplateRenderer.cpp
        ${CMAKE_SOURCE_DIR}/src/TemplateRenderer/TemplateRenderer.cpp
        ${CMAKE_SOURCE_DIR}/src/TemplateRenderer/TemplateRenderer.h
)
target_include_directories(TestTemplateRenderer PUBLIC ${CMAKE_SOURCE_DIR}/src/TemplateRenderer)
target_link_libraries(TestTemplateRenderer PRIVATE TemplateRenderer)
set_target_properties(TestTemplateRenderer PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/bin/$<CONFIG>"
)


function(generate_configure_info template_file)
    # ========== 解析可选参数 ==========
    cmake_parse_arguments(
            CFG
            ""
            "OUTPUT"
            ""
            ${ARGN}
    )

    get_filename_component(template_name "${template_file}" NAME_WE)
    if(DEFINED CFG_OUTPUT AND NOT CFG_OUTPUT STREQUAL "")
        # 用户指定了输出路径
        set(output_file "${CFG_OUTPUT}/${template_name}.h")
    else()
        # 默认
        get_filename_component(template_dir "${template_file}" DIRECTORY)

        set(output_file "${template_dir}/${template_name}.h")
    endif()


    set(CMAKE_PATH "${CMAKE_COMMAND}")

    set(PROJECT_ROOT_PATH "${CMAKE_SOURCE_DIR}")

    set(QT_PATH "")

    if(DEFINED Qt6_DIR AND NOT Qt6_DIR STREQUAL "")
        set(QT_PATH "${Qt6_DIR}")
    elseif(DEFINED Qt5_DIR AND NOT Qt5_DIR STREQUAL "")
        set(QT_PATH "${Qt5_DIR}")
    elseif(DEFINED CMAKE_PREFIX_PATH AND NOT CMAKE_PREFIX_PATH STREQUAL "")
        list(GET CMAKE_PREFIX_PATH 0 EXPORT_QT_PATH)
    endif()

    configure_file(
            "${template_file}"
            "${output_file}"
            @ONLY
    )

endfunction()

generate_configure_info(${CMAKE_SOURCE_DIR}/templates/plugin/configure_info.h.in
        OUTPUT ${CMAKE_SOURCE_DIR}/plugins
)
target_include_directories(PluginManagementView  PRIVATE ${CMAKE_SOURCE_DIR}/plugins)
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
