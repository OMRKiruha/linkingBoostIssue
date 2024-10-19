cmake_minimum_required(VERSION 3.10.2)

string(ASCII 27 Esc)
set(ColourReset "${Esc}[m")
set(Red "${Esc}[31m")
set(Green "${Esc}[32m")
set(Yellow "${Esc}[33m")
set(Blue "${Esc}[34m")

project(linkingBoostIssue)

message("\n${Yellow}==> Project: ${PROJECT_NAME} (${CMAKE_CURRENT_LIST_DIR}) ${ColourReset}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_EXE_LINKER_FLAGS "-static-libstdc++ -static-libgcc")
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")   # -O2 -g0
cmake_policy(SET CMP0167 NEW)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

# Threads =====================================================================
find_package(Threads REQUIRED)
if (NOT Threads_FOUND)
    message("${Red}==> !!! Threads not found${ColourReset}")
else ()
    message("${Green}==> !!! Threads found${ColourReset}")
endif ()

# OpenSSL =====================================================================
find_package(OpenSSL REQUIRED)
if (NOT OpenSSL_FOUND)
    message("${Red}==> !!! OpenSSL not found${ColourReset}")
else ()
    message("${Green}==> !!! OpenSSL found${ColourReset}")
endif ()

# Boost =======================================================================
set(BOOST_ROOT "../../boost/")
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "D:/boost/" )
message("${Blue}==> BOOST_ROOT ${BOOST_ROOT} ${ColourReset}")
message("${Blue}==> CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${ColourReset}")

set(Boost_USE_STATIC_LIBS ON)  # only find static libs
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME ON)
set(Boost_USE_DEBUG_LIBS ON)  # only find release libs
set(Boost_USE_RELEASE_LIBS OFF)  # only find release libs

find_package(Boost COMPONENTS
        headers
        system
        asio
        beast
        context
        REQUIRED)

if (NOT Boost_FOUND)
    message("${Red}==> !!! Boost not found${ColourReset}")
else ()
    message("${Green}==> !!! Boost found${ColourReset}")
endif ()

include_directories(${Boost_INCLUDE_DIR})

add_executable(${PROJECT_NAME}
        main.cpp
        download.h
        download.cpp
        root_cert.h
)

if (WIN32)
    set(SYS_LIB_DIR C:/msys64/usr/lib)
    set(win_libs wsock32 ws2_32 ${SYS_LIB_DIR}/libdl.a)
endif ()

message("${Blue}==> Boost_LIBRARIES ${Boost_LIBRARIES} ${ColourReset}")

target_link_libraries(${PROJECT_NAME} PUBLIC
        ${OPENSSL_LIBRARIES}
        Threads::Threads
        pthread
        ${win_libs}
        ${Boost_LIBRARIES}
)

message("${Yellow}<== Project: ${PROJECT_NAME} (${CMAKE_CURRENT_LIST_DIR}) ${ColourReset}\n")