project(node_plugin)

# 包含插件入口和之前的编辑器逻辑
set(PLUGIN_SOURCES
        _plugin.h
        _plugin.cpp
        ${CMAKE_SOURCE_DIR}/include/NodeEditor.cpp
        ${CMAKE_SOURCE_DIR}/include/NodeEditor.h
)

# [核心] 以模块方式创建库
add_library(node_editor_plugin MODULE ${PLUGIN_SOURCES})

# 链接 Qt 窗口库
target_link_libraries(node_editor_plugin PRIVATE Qt5::Widgets)

# 自动部署：将生成的插件存放到主程序的 plugins 目录下
set_target_properties(node_editor_plugin PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
)