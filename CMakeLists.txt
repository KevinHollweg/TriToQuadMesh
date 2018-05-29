cmake_minimum_required(VERSION 2.8)

set(PROG_NAME triToQuad)
project (${PROG_NAME})

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH} )
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_BUILD_TYPE Debug)

set (PROGRAM_VERSION_MAJOR 1)
set (PROGRAM_VERSION_MINOR 0)

############# Required LIBRARIES ###############

#SDL
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})
SET(LIBS ${LIBS} ${SDL2_LIBRARY})

#OPENGL
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})
SET(LIBS ${LIBS} ${OPENGL_LIBRARIES})

#GLM
find_package(GLM REQUIRED)
include_directories( ${GLM_INCLUDE_DIRS}) 

#GLEW
find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})
SET(LIBS ${LIBS} ${GLEW_LIBRARIES})

#png
find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIRS})
SET(LIBS ${LIBS} ${PNG_LIBRARIES})

############# COMPILER FLAGS ###############


if(UNIX)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wno-long-long")
endif(UNIX)

if(MSVC)
	#multiprocessor compilation for visual studio
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP") 
endif()

############# SOURCE ###############

add_subdirectory(src)

#############  output dir ############# 

if(MSVC)
	#visual studio working dir
	set_target_properties(${PROG_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()

#copies executable to top level source tree
set_target_properties( ${PROG_NAME}
	PROPERTIES
    	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")


