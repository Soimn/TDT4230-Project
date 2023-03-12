cmake_minimum_required(VERSION 3.6)
project(TDT4230-Project)

include(FetchContent)
Set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(
        SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.26.4
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(SDL2)

set(CMAKE_CXX_STANDARD 14)
if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
endif()

set (CMAKE_SUPPRESS_REGENERATION TRUE)

add_executable(${PROJECT_NAME} "src/main.cpp")
target_link_libraries(${PROJECT_NAME} SDL2::SDL2main SDL2::SDL2-static)
