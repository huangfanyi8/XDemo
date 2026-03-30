cmake_minimum_required(VERSION 3.20)

project(XDemo
        LANGUAGES CXX
        DESCRIPTION "测试自定义控件")

if(MSVC)
    add_compile_options(/utf-8)
endif()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)          # <<< 已经打开
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ================= 1. 查找 Qt (核心修正点) =================
find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets UiPlugin REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets UiPlugin REQUIRED)

# ================= 2. 部署函数 =================
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

function(make_qt_clean_run TARGET_NAME)
    get_target_property(_qmake_path Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_root "${_qmake_path}" DIRECTORY)
    get_filename_component(_qt_root "${_qt_root}" DIRECTORY)

    set(_conf_file "${CMAKE_CURRENT_BINARY_DIR}/qt.conf")
    file(WRITE "${_conf_file}" "[Paths]\nPrefix = ${_qt_root}\nBinaries = bin")

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

    # ---------- 1️⃣ 取得 .cpp ----------
    list(GET ARG_UNPARSED_ARGUMENTS 0 source_file)
    list(REMOVE_AT ARG_UNPARSED_ARGUMENTS 0)
    set(lib_args ${ARG_UNPARSED_ARGUMENTS})

    get_filename_component(target_name ${source_file} NAME_WE)

    # ---------- 2️⃣ 组装所有源文件 ----------
    set(all_sources ${source_file})

    # 自动把同目录下同名 .h 加进去（如果存在）
    get_filename_component(src_dir ${source_file} DIRECTORY)
    set(possible_header "${src_dir}/${target_name}.h")
    if(EXISTS "${possible_header}")
        list(APPEND all_sources "${possible_header}")
    endif()

    # 如果用户给了 QRC，直接加入
    if(ARG_QRC)
        list(APPEND all_sources ${ARG_QRC})
    endif()

    # ---------- 3️⃣ 创建可执行文件 ----------
    add_executable(${target_name} ${all_sources})
    target_link_libraries(${target_name} PRIVATE ${lib_args})

    set_target_properties(${target_name} PROPERTIES
            PREFIX ""
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/bin/$<CONFIG>"
    )
    deploy_qt_target(${target_name})
endfunction()

# ================= 4. 定义目标 =================
add_test_exe_func(test/module1.cpp QRC resources/resources.qrc Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::UiPlugin)
add_test_exe_func(test/module2.cpp QRC resources/resources.qrc Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::UiPlugin)
add_test_exe_func(test/module3.cpp QRC resources/resources.qrc Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::UiPlugin)
add_test_exe_func(test/module4.cpp QRC resources/resources.qrc Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::UiPlugin)
add_test_exe_func(test/module5.cpp QRC resources/resources.qrc Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::UiPlugin)

add_test_exe_func(test/PluginManagementView.cpp Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::UiPlugin)

add_test_exe_func(test/XListWidget.cpp Qt${QT_VERSION_MAJOR}::Widgets)
add_test_exe_func(test/XFileSystem.cpp Qt${QT_VERSION_MAJOR}::Core)

# 其它目标保持不变
target_sources(PluginManagementView PRIVATE
        test/PluginVersionManager.cpp
        test/PluginVersionManager.h
)

add_library(PluginBuilder test/PluginBuilder.h test/PluginBuilder.cpp)
target_link_libraries(PluginBuilder PUBLIC Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::UiPlugin)

add_executable(test_plugin_builder test/test.cpp resources/resources.qrc)
target_link_libraries(test_plugin_builder PUBLIC PluginBuilder)
target_link_libraries(test_plugin_builder  PUBLIC Qt${QT_VERSION_MAJOR}::Widgets  Qt${QT_VERSION_MAJOR}::UiPlugin)

# add_subdirectory(plugins)   # 如需插件子目录自行打开

deploy_qt_target(test_plugin_builder)
