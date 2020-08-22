cmake_minimum_required(VERSION 3.8)
project(TaskManager)

set(CMAKE_CXX_STANDARD_REQUIRED 17)

set (
	TASK_MANAGER_HEADER_FILES
		task_manager.h
)

add_library(
	TaskManager STATIC 
		${FILE_WORKERS_HEADER_FILES} 
)
target_include_directories(TaskManager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(TaskManager OperationManagement)
