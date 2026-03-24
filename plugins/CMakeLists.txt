add_library( ButtonV1 SHARED Button/ButtonV1.cpp)
add_library( ButtonV2 SHARED Button/ButtonV2.cpp)
# 链接 Qt5 模块
target_link_libraries(ButtonV1
        Qt5::Core
        Qt5::Widgets
        Qt5::UiPlugin
)
# 链接 Qt5 模块
target_link_libraries(ButtonV2
        Qt5::Core
        Qt5::Widgets
        Qt5::UiPlugin
)
message(${CMAKE_SOURCE_DIR}/bin/plugins)
# 设置输出目录到 bin/plugins，并将导入库输出到临时目录
set_target_properties(ButtonV1 PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/plugins   # Windows DLL
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/plugins   # Linux .so / macOS .dylib
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build_temp      # 将 .lib 输出到临时目录
)

set_target_properties(ButtonV2 PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/plugins   # Windows DLL
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/plugins   # Linux .so / macOS .dylib
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build_temp      # 将 .lib 输出到临时目录
)

# Windows 平台：不生成导入库（.lib 和 .exp）
if(WIN32)
    set_target_properties(ButtonV1 PROPERTIES
        LINK_FLAGS "/IMPLIB:NUL"
    )
    set_target_properties(ButtonV2 PROPERTIES
        LINK_FLAGS "/IMPLIB:NUL"
    )
endif()