cmake_minimum_required(VERSION 3.20)

add_library(licenx_examples_common INTERFACE)
target_include_directories(licenx_examples_common
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/common
)

if (WIN32)
    add_executable(licenx_example_gpu_gui WIN32
        gui_gpu/main.cpp
    )
    target_compile_features(licenx_example_gpu_gui PRIVATE cxx_std_20)
    target_link_libraries(licenx_example_gpu_gui
        PRIVATE
            licenx
            licenx_examples_common
            user32
            gdi32
    )
    target_compile_definitions(licenx_example_gpu_gui
        PRIVATE
            UNICODE
            _UNICODE
    )
endif()

add_executable(licenx_example_cli
    cli_features/main.cpp
)
target_compile_features(licenx_example_cli PRIVATE cxx_std_20)
target_link_libraries(licenx_example_cli
    PRIVATE
        licenx
        licenx_examples_common
)
