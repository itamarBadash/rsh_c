cmake_minimum_required(VERSION 3.10)
project(base)

set(CMAKE_CXX_STANDARD 17)

# Set CMAKE_PREFIX_PATH to the install directory of MAVSDK
set(CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/install)

find_package(MAVSDK REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

# Add MAVSDK includes
include_directories(${CMAKE_PREFIX_PATH}/include)
include_directories(${CMAKE_PREFIX_PATH}/include/mavsdk)
include_directories(${CMAKE_PREFIX_PATH}/inih)

# Add MAVSDK library paths
#ink_directories(${CMAKE_PREFIX_PATH}/lib)

add_executable(base main.cpp
        Src/Modules/TelemetryManager.cpp
        Src/Modules/TelemetryManager.h
        Src/Modules/CommandManager.cpp
        Src/Modules/CommandManager.h
        CommunicationManager.cpp
        CommunicationManager.h
        inih/ini.c
        inih/ini.h
        inih/cpp/INIReader.cpp
        inih/cpp/INIReader.h
        Events/EventManager.h
        Src/Modules/BaseAddon.cpp
        Src/Modules/BaseAddon.h
        Src/Communications/TCPServer.cpp
        Src/Communications/TCPServer.h
)

# Link MAVSDK libraries
target_link_libraries(base
        MAVSDK::mavsdk
)
