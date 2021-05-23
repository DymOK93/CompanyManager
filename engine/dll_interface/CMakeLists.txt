cmake_minimum_required(VERSION 3.8)
project("CM Engine DLL Export/Import interface")

set (
	CM_ENGINE_INTERFACE_HEADER_FILES
		"cm_engine_dll_interface.hpp"
)

set(TARGET_LIB cm_engine_dll_interface)

add_library(
	${TARGET_LIB} INTERFACE
		${CM_ENGINE_INTERFACE_HEADER_FILES} 
)
target_include_directories(
	${TARGET_LIB} INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}
)

