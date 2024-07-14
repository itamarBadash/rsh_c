cmake_minimum_required(VERSION 3.10)
project(base)

set(CMAKE_CXX_STANDARD 17)

# Set CMAKE_PREFIX_PATH to the install directory of MAVSDK
set(CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/MAVSDK/install)

find_package(MAVSDK REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)
include_directories(${Boost_INCLUDE_DIRS})

# Add MAVSDK includes
include_directories(${CMAKE_PREFIX_PATH}/include)
include_directories(${CMAKE_PREFIX_PATH}/include/mavsdk)

# Add MAVSDK library paths
#ink_directories(${CMAKE_PREFIX_PATH}/lib)

add_executable(base main.cpp
        TelemetryManager.cpp
        TelemetryManager.h
        CommandManager.cpp
        CommandManager.h
        CommunicationManager.cpp
        CommunicationManager.h
        inih/cpp/INIReader.cpp
        inih/cpp/INIReader.h

)


# Link MAVSDK libraries
target_link_libraries(base
        MAVSDK::mavsdk
        ${Boost_INCLUDE_DIRS}
)
