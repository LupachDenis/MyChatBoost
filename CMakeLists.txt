cmake_minimum_required(VERSION 3.9)
PROJECT(BoostChat)

SET(Boost_USE_STATIC_LIBS ON)
SET(Boost_USE_MULTITHREAD ON)
FIND_PACKAGE(Boost 1.65.0 REQUIRED COMPONENTS system)
IF(Boost_FOUND)
  INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
  LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
ENDIF(Boost_FOUND)

if(WIN32)
    add_definitions("-D_WIN32_WINNT=0x0501 ")
endif(WIN32)

SET(USED_LIBS ${Boost_SYSTEM_LIBRARY})

ADD_EXECUTABLE(MyChatServer src/mainServer.cpp src/MyChatNetwork.h)
TARGET_LINK_LIBRARIES(MyChatServer ${USED_LIBS})

ADD_EXECUTABLE(MyChatClient src/mainClient.cpp src/MyChatNetwork.h)
TARGET_LINK_LIBRARIES(MyChatServer ${USED_LIBS})
