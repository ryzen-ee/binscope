cmake_minimum_required(VERSION 3.16)
project(BinaryAnalyzer)

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

add_executable(pe ${SOURCES} ${HEADERS})

target_link_libraries(pe Qt5::Widgets OpenSSL::SSL OpenSSL::Crypto)