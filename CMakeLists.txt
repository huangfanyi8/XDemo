cmake_minimum_required(VERSION 3.20)

project(XDemo
        LANGUAGES CXX
        DESCRIPTION "测试自定义控件")

if(MSVC)
    add_compile_options(/utf-8)
endif()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ================= 1. 查找 Qt (核心修正点) =================
# 先探测主要版本 (QT_VERSION_MAJOR)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets UiPlugin REQUIRED)
# 必须在这里同时请求 Widgets 和 UiPlugin，否则 target_link_libraries 找不到目标
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets UiPlugin REQUIRED)

# ================= 2. 部署函数 =================
function(deploy_qt_target TARGET_NAME)
    if(NOT WIN32)
        return()
    endif()

    # 根据版本查找对应的执行程序
    find_program(WINDEPLOYQT_EXECUTABLE
            NAMES windeployqt${QT_VERSION_MAJOR} windeployqt
            HINTS "${CMAKE_PREFIX_PATH}/bin")

    if(NOT WINDEPLOYQT_EXECUTABLE)
        return()
    endif()

    set(_deploy_mode "$<$<CONFIG:Debug>:--debug>$<$<NOT:$<CONFIG:Debug>>:--release>")

    # 保持你要求的两次分开部署逻辑
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
            VERBATIM # 建议添加，保证参数转义正确
    )
endfunction()

function(make_qt_clean_run TARGET_NAME)
    # 1. 自动找到你的 Qt 安装在哪里
    get_target_property(_qmake_path Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_root "${_qmake_path}" DIRECTORY)
    get_filename_component(_qt_root "${_qt_root}" DIRECTORY) # 得到 C:/Qt/.../msvc2017_64

    # 2. 在 exe 旁边生成那个小小的配置文件
    set(_conf_file "${CMAKE_CURRENT_BINARY_DIR}/qt.conf")
    file(WRITE "${_conf_file}" "[Paths]\nPrefix = ${_qt_root}\nBinaries = bin")

    # 3. 每次编译完，自动把这个小文件拷贝到 exe 旁边
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy "${_conf_file}" "$<TARGET_FILE_DIR:${TARGET_NAME}>/qt.conf"
    )
endfunction()

# ================= 3. 构建函数 =================
function(add_test_exe_func)
    set(options "")
    set(oneValueArgs QRC)
    set(multiValueArgs "")

    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # 获取源文件和剩余链接库
    list(GET ARG_UNPARSED_ARGUMENTS 0 source_file)
    list(REMOVE_AT ARG_UNPARSED_ARGUMENTS 0)
    set(lib_args ${ARG_UNPARSED_ARGUMENTS})

    get_filename_component(target_name ${source_file} NAME_WE)

    set(all_sources ${source_file})
    if(ARG_QRC)
        list(APPEND all_sources ${ARG_QRC})
    endif()

    add_executable(${target_name} ${all_sources})
    target_link_libraries(${target_name} PRIVATE ${lib_args})

    set_target_properties(${target_name} PROPERTIES
            PREFIX ""
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/bin/$<CONFIG>"
    )
deploy_qt_target(${target_name})
endfunction()

# ================= 4. 定义目标 =================
# 现在 Qt${QT_VERSION_MAJOR}::UiPlugin 应该可以被正常识别了
add_test_exe_func(test/module1.cpp QRC resources/resources.qrc Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::UiPlugin)
add_test_exe_func(test/module2.cpp QRC resources/resources.qrc Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::UiPlugin)
add_test_exe_func(test/module3.cpp QRC resources/resources.qrc Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::UiPlugin)
add_test_exe_func(test/module4.cpp QRC resources/resources.qrc Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::UiPlugin)
add_test_exe_func(test/module5.cpp QRC resources/resources.qrc Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::UiPlugin)
add_test_exe_func(test/PluginManagementView.cpp Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::UiPlugin)
add_test_exe_func(test/XQtPluginBuilder.cpp Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::UiPlugin)
add_test_exe_func(test/XListWidget.cpp Qt${QT_VERSION_MAJOR}::Widgets )
target_sources(PluginManagementView PRIVATE
        test/PluginVersionManager.cpp
        test/PluginVersionManager.h
)
set_target_properties(XQtPluginBuilder PROPERTIES AUTOMOC OFF)
add_subdirectory(plugins)
