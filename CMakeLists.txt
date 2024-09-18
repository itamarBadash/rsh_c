cmake_minimum_required(VERSION 3.10)
project(base)

# Set C++ standard to 17
set(CMAKE_CXX_STANDARD 17)

add_executable(base
        main.cpp
        Src/Modules/TelemetryManager.cpp
        Src/Modules/TelemetryManager.h
        Src/Modules/CommandManager.cpp
        Src/Modules/CommandManager.h
        Src/Communications/SerialCommunication.cpp
        Src/Communications/SerialCommunication.h
        inih/ini.c
        inih/ini.h
        inih/cpp/INIReader.cpp
        inih/cpp/INIReader.h
        Events/EventManager.h
        Src/Modules/BaseAddon.cpp
        Src/Modules/BaseAddon.h
        Src/Communications/TCPServer.cpp
        Src/Communications/TCPServer.h
        Src/Communications/UDPServer.cpp
        Src/Communications/UDPServer.h
        Src/Modules/CommunicationManager.cpp
        Src/Modules/CommunicationManager.h
        Src/Communications/ICommunication.h
        Src/Modules/BaseCamera.cpp
        Src/Modules/BaseCamera.h
)

# Set the path to OpenCV based on the operating system
if(WIN32)
    # Windows specific OpenCV settings
    message(STATUS "Using manually specified OpenCV for Windows")

    set(OpenCV_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/opencv/build/include")
    set(OpenCV_LIBS_DIR "${CMAKE_SOURCE_DIR}/opencv/build/x64/vc16/lib")
    set(OpenCV_DLL_DIR "${CMAKE_SOURCE_DIR}/opencv/build/x64/vc16/bin")

    # Link the required OpenCV libraries (adjust for your setup)
    set(OpenCV_LIBS
            ${OpenCV_LIBS_DIR}/opencv_core480.lib
            ${OpenCV_LIBS_DIR}/opencv_imgproc480.lib
            ${OpenCV_LIBS_DIR}/opencv_highgui480.lib
            ${OpenCV_LIBS_DIR}/opencv_videoio480.lib
    )

    # Add OpenCV include and library directories for Windows
    include_directories(${OpenCV_INCLUDE_DIRS})
    link_directories(${OpenCV_LIBS_DIR})

    # Post-build step: Copy OpenCV DLLs to the output directory (Windows only)
    add_custom_command(TARGET base POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${OpenCV_DLL_DIR}" $<TARGET_FILE_DIR:base>)

elseif(UNIX)
    # Linux specific OpenCV settings
    message(STATUS "Using system-installed OpenCV for Linux")

    # Find system-installed OpenCV
    find_package(OpenCV REQUIRED)

    # Add system-installed OpenCV include directories for Linux
    include_directories(${OpenCV_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

# Find MAVSDK (this will locate MAVSDK installed via vcpkg or other package managers)
find_package(MAVSDK REQUIRED)

# Link libraries to the executable
if(WIN32)
    target_link_libraries(base
            MAVSDK::mavsdk  # Link MAVSDK on Windows
            ${OpenCV_LIBS}  # Manually link OpenCV on Windows
    )
elseif(UNIX)
    target_link_libraries(base
            MAVSDK::mavsdk  # Link MAVSDK on Linux
            ${OpenCV_LIBS}  # System-installed OpenCV on Linux
    )
endif()
