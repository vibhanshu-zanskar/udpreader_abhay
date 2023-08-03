cmake_minimum_required(VERSION 3.16)

project(nsereader)

# Generate compilation database
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
set(CMAKE_VERBOSE_MAKEFILE ON CACHE INTERNAL "")

# Force c++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

string(APPEND CMAKE_CXX_FLAGS_DEBUG " -O0 -ggdb3 -fno-omit-frame-pointer -fsanitize=address")
string(APPEND CMAKE_CXX_FLAGS_RELWITHDEBINFO " -ggdb3 -fno-omit-frame-pointer")
string(APPEND CMAKE_CXX_FLAGS_RELEASE " -s")

# Select all sources from src directory
file(GLOB_RECURSE ${PROJECT_NAME}_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
message("List of source files: ${${PROJECT_NAME}_SOURCES}")

# Add add needed libraries
include(FindPCAP.cmake)
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/")
#find_package(PCAP REQUIRED)
find_package(Threads REQUIRED)

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SOURCES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        # where the library itself will look for its internal headers
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    PUBLIC
        # where top-level project will look for the library's public headers
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        # where external projects will look for the library's public headers
        $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${PCAP_LIBRARY})
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
