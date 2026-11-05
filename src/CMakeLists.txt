add_library(lab4_core STATIC
    core/version.cpp
    core/error.cpp
)

target_include_directories(lab4_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lab4_core PRIVATE lab4_warnings)

add_executable(lab4 main.cpp)
target_link_libraries(lab4 PRIVATE lab4_core lab4_warnings)
