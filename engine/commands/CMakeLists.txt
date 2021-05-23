cmake_minimum_required(VERSION 3.8)
project("Command Interface")

set (
	COMMAND_INTERFACE_HEADER_FILES
		"command_interface.h"
)

set(TARGET_LIB command_interface)

add_library(
	${TARGET_LIB} INTERFACE
		${COMMAND_INTERFACE_HEADER_FILES} 
)
target_include_directories(
	${TARGET_LIB} INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(
	${TARGET_LIB} INTERFACE
		cm_engine_dll_interface
		object_pool
)

