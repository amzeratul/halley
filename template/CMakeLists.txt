cmake_minimum_required (VERSION 3.10)

project (halleyGame)

set(HALLEY_PROJECT_EMBED 1)
#set(HALLEY_PATH "" CACHE FILEPATH "Path to the Halley root")
set(HALLEY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/halley)
set(HOTRELOAD 0 CACHE BOOL "Sets whether to hot-reload this project")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${HALLEY_PATH}/cmake/")
include(HalleyProject)

if(NOT (DEFINED HALLEY_GAME_BIN_DIR))
	set(HALLEY_GAME_BIN_DIR "bin")
endif()

set (SOURCES
	"prec.cpp"
	"src/game.cpp"
	"src/game_stage.cpp"
	)

set (HEADERS
	"prec.h"
	"src/game.h"
	"src/game_stage.h"
	)

set (GEN_DEFINITIONS
	)

halleyProject(halleyGame "${SOURCES}" "${HEADERS}" "${GEN_DEFINITIONS}" ${CMAKE_CURRENT_SOURCE_DIR}/${HALLEY_GAME_BIN_DIR})
