FILE(GLOB_RECURSE main_SRC  *.cpp)
FILE(GLOB_RECURSE PROG_HEADER  *.h)


SET(PROG_SRC ${main_SRC})

include_directories(.)

add_executable(${PROG_NAME} ${PROG_SRC} ${PROG_HEADER})
target_link_libraries(${PROG_NAME} ${LIBS} )



