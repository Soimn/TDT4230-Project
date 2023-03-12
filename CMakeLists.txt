cmake_minimum_required(VERSION 3.6)
project(TDT4230-Project)

add_subdirectory(vendor/SDL)

set(CMAKE_CXX_STANDARD 14)
if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
endif()

set (CMAKE_SUPPRESS_REGENERATION TRUE)

add_executable(${PROJECT_NAME} "src/main.cpp")
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vendor/SDL/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2-static SDL2main)
