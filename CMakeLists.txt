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
        Src/Addons/BaseAddon.cpp
        Src/Addons/BaseAddon.h
        Src/Communications/TCPServer.cpp
        Src/Communications/TCPServer.h
        Src/Communications/UDPServer.cpp
        Src/Communications/UDPServer.h
        Src/Modules/CommunicationManager.cpp
        Src/Modules/CommunicationManager.h
        Src/Communications/ICommunication.h
        Src/Modules/UDPVideoStreamer.cpp
        Src/Modules/UDPVideoStreamer.h
        Src/Modules/AddonsManager.cpp
        Src/Modules/AddonsManager.h
)

# Set the path to OpenCV based on the operating system
if(WIN32)
    # Windows specific OpenCV settings
    message(STATUS "Using manually specified OpenCV for Windows")

    set(OpenCV_DIR "${CMAKE_SOURCE_DIR}/opencv/build")
    set(MAVSDK_DIR "${CMAKE_SOURCE_DIR}/install/lib/cmake/MAVSDK")

    find_package(OpenCV REQUIRED)
    find_package(MAVSDK REQUIRED)

    # Add OpenCV include and library directories for Windows
    include_directories(${OpenCV_INCLUDE_DIRS})
    link_directories(${OpenCV_LIBS_DIR})

    include_directories(${MAVSDK_INCLUDE_DIRS})
    link_directories(${MAVSDK_LIBS_DIR})

    target_link_libraries(base ${OpenCV_LIBS})
    target_link_libraries(base ${MAVSDK_LIBS})

    # Post-build step: Copy OpenCV DLLs to the output directory (Windows only)
    add_custom_command(TARGET base POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${OpenCV_DLL_DIR}" $<TARGET_FILE_DIR:base>)

    add_custom_command(TARGET base POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${MAVSDK_DLL_DIR}" $<TARGET_FILE_DIR:base>)

    set(LIBUSB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libusb/include")
    include_directories(${LIBUSB_INCLUDE_DIR})

    set(LIBUSB_LIB_DIR "${CMAKE_SOURCE_DIR}/libusb/VS2022/MS64/static")
    link_directories(${LIBUSB_LIB_DIR})

    target_link_libraries(base libusb-1.0)

    add_custom_command(TARGET base POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/libusb/VS2022/MS64/dll" $<TARGET_FILE_DIR:base>)



elseif(UNIX)
    # Linux specific OpenCV settings
    message(STATUS "Using system-installed OpenCV and libusb for Linux")

    # Find system-installed OpenCV
    find_package(OpenCV REQUIRED)

    # Find system-installed libusb-1.0
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

    # Add system-installed OpenCV include directories for Linux
    include_directories(${OpenCV_INCLUDE_DIRS})

    # Add system-installed libusb include directories for Linux
    include_directories(${LIBUSB_INCLUDE_DIRS})

    # Link system-installed libusb-1.0 library for Linux
    link_directories(${LIBUSB_LIBRARY_DIRS})

    # Link libraries
    target_link_libraries(base
            MAVSDK::mavsdk  # Link MAVSDK on Linux
            ${OpenCV_LIBS}  # System-installed OpenCV on Linux
            ${LIBUSB_LIBRARIES}  # Link libusb-1.0 on Linux
    )
endif()

# Find MAVSDK (this will locate MAVSDK installed via vcpkg or other package managers)
find_package(MAVSDK REQUIRED)

# Link libraries to the executable
if(WIN32)
    target_link_libraries(base
            MAVSDK::mavsdk  # Link MAVSDK on Windows
    )
elseif(UNIX)
    target_link_libraries(base
            MAVSDK::mavsdk  # Link MAVSDK on Linux
            ${OpenCV_LIBS}  # System-installed OpenCV on Linux
    )
endif()
