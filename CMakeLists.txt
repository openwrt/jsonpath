cmake_minimum_required(VERSION 2.6)

PROJECT(jsonpath C)
ADD_DEFINITIONS(-Os -Wall -Werror --std=gnu99 -Wmissing-declarations)

SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

IF(APPLE)
  INCLUDE_DIRECTORIES(/opt/local/include)
  LINK_DIRECTORIES(/opt/local/lib)
ENDIF()

find_library(json NAMES json-c json)

IF(DEBUG)
  ADD_DEFINITIONS(-DDEBUG -g3)
ENDIF()

INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(JSONC json-c json)
IF(JSONC_FOUND)
  ADD_DEFINITIONS(-DJSONC)
  INCLUDE_DIRECTORIES(${JSONC_INCLUDE_DIRS})
ENDIF()

FIND_PACKAGE(BISON REQUIRED)
IF(BISON_FOUND)
    ADD_CUSTOM_COMMAND(
      OUTPUT parser.c
      DEPENDS parser.y
      COMMAND ${BISON_EXECUTABLE} parser.y
      COMMENT "Generating parser.c"
    )
ENDIF()

FIND_PACKAGE(FLEX REQUIRED)
IF(FLEX_FOUND)
    ADD_CUSTOM_COMMAND(
      OUTPUT lexer.c
      DEPENDS lexer.l
      COMMAND ${FLEX_EXECUTABLE} lexer.l
      COMMENT "Generating lexer.c"
    )
ENDIF()

SET_PROPERTY(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES "lexer.h;parser.h")
SET_SOURCE_FILES_PROPERTIES("lexer.c;parser.c" PROPERTIES GENERATED TRUE)
ADD_EXECUTABLE(jsonpath main.c lexer.c parser.c matcher.c)
TARGET_LINK_LIBRARIES(jsonpath ubox ${json})

INSTALL(TARGETS jsonpath RUNTIME DESTINATION bin)
