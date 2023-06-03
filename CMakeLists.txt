cmake_minimum_required(VERSION 3.15)

project(imgui_test_engine)

find_package(imgui REQUIRED)
add_library(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PUBLIC imgui::imgui)

target_sources(${PROJECT_NAME} PRIVATE
    imgui_test_engine/imgui_capture_tool.cpp
    imgui_test_engine/imgui_te_context.cpp
    imgui_test_engine/imgui_te_coroutine.cpp
    imgui_test_engine/imgui_te_engine.cpp
    imgui_test_engine/imgui_te_exporters.cpp
    imgui_test_engine/imgui_te_perftool.cpp
    imgui_test_engine/imgui_te_ui.cpp
    imgui_test_engine/imgui_te_utils.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER 
"imgui_test_engine/imgui_te_engine.h"
)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/imgui_test_engine/
)