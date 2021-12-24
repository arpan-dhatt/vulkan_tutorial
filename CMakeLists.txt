# warning: degeneracy in the following file considered harmful

cmake_minimum_required(VERSION 3.21)
project(vulkan_tutorial)

set(CMAKE_CXX_STANDARD 17)

add_executable(vulkan_tutorial main.cpp)

target_include_directories(vulkan_tutorial PUBLIC
        /usr/local/include /opt/homebrew/include)
target_link_libraries(vulkan_tutorial PUBLIC
        /usr/local/lib/libvulkan.1.2.198.dylib
        /usr/local/lib/libvulkan.1.dylib
        /usr/local/lib/libvulkan.dylib
        /opt/homebrew/lib/libglfw.3.3.dylib
        /opt/homebrew/lib/libglfw.3.dylib
        /opt/homebrew/lib/libglfw.dylib)