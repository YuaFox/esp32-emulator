cmake_minimum_required(VERSION 3.0.0)
project(esp32-emulator VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

include_directories(${CMAKE_SOURCE_DIR}/src)

file(GLOB_RECURSE SRCS ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE HDRS ${CMAKE_SOURCE_DIR}/src/*.h)

add_executable(esp32-emulator main.cpp ${SRCS} ${HDRS})

TARGET_LINK_LIBRARIES(esp32-emulator) 

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
