cmake_minimum_required(VERSION 3.6)
project(TDT4230-Project)

add_definitions(-DGLEW_STATIC)
add_subdirectory(vendor/glew-cmake)
add_subdirectory(vendor/SDL)

set(CMAKE_CXX_STANDARD 14)
if(MSVC)
	set(CMAKE_CXX_FLAGS_DEBUG_INIT "/W4 /Od /Zo /Z7")
	set(CMAKE_CXX_FLAGS_RELEASE_INIT "/W4 /O2")
else()
	set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall -Wextra -Wpedantic -Og")
	set(CMAKE_CXX_FLAGS_RELEASE_INIT "-Wall -Wextra -Wpedantic -O2")
endif()

set (CMAKE_SUPPRESS_REGENERATION TRUE)

file(GLOB IMGUI_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui*.cpp)

add_executable(${PROJECT_NAME} src/main.cpp ${IMGUI_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_sdl2.cpp ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_opengl3.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vendor/SDL/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2-static SDL2main libglew_static)
