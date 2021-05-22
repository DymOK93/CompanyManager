cmake_minimum_required(VERSION 3.8)
project("Task Manager")

set (
	TASK_MANAGER_HEADER_FILES
		"task_manager.h"
)

set(TARGET_LIB task_mgr)

add_library(
	${TARGET_LIB} INTERFACE
		${TASK_MANAGER_HEADER_FILES} 
)
target_include_directories(
	${TARGET_LIB} INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}
)

