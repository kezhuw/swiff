CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(SWIFF)

ENABLE_TESTING()

SET(CMAKE_C_FLAGS "-std=c99 -Werror -Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable -Wno-unused-label")

ADD_SUBDIRECTORY(base)
