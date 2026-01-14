project(app)

set(HSOURCES
        main.cpp
        main_window.h
        main_window.cpp
)
add_executable(ZeroJitterWindow
        main.cpp
        dark_window.h
        dark_window.cpp
)

target_link_libraries(ZeroJitterWindow PRIVATE
        Qt5::GuiPrivate
        Qt5::Widgets
        Qt5::Gui
        user32
        dwmapi
)

