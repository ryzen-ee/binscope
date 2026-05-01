cmake_minimum_required(VERSION 3.16)
project(BinScope)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets)
find_package(OpenSSL REQUIRED)

include_directories(${Qt5_INCLUDE_DIRS})

# Source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/parser.cpp
    src/hasher.cpp
    src/strings.cpp
    src/indicators.cpp
    src/icon_data.cpp
    src/resources.qrc
)

set(HEADERS
    src/mainwindow.h
    src/parser.h
    src/icon_data.h
)

# Set output name - only .exe on Windows
if(WIN32)
    set(CMAKE_EXECUTABLE_SUFFIX ".exe")
endif()
add_executable(binscope WIN32 ${SOURCES} ${HEADERS})

# Set icon for Windows
if(WIN32)
    set(BINSCOPE_ICON ${CMAKE_SOURCE_DIR}/ico/main.ico)
    set_target_properties(binscope PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

target_link_libraries(binscope Qt5::Widgets OpenSSL::SSL OpenSSL::Crypto)

# Install rules for Windows
if(WIN32)
    install(TARGETS binscope RUNTIME DESTINATION bin)
endif()