cmake_minimum_required(VERSION 3.8)
project("Chain Workers")

set (
	CHAIN_WORKERS_HEADER_FILES
		"worker_interface.h"
		"file_workers.h"
)

set (
	CHAIN_WORKERS_SOURCE_FILES
		"file_workers.cpp"
)

set(TARGET_LIB chain_workers)

add_library(
	${TARGET_LIB} STATIC 
		${CHAIN_WORKERS_HEADER_FILES} 
		${CHAIN_WORKERS_SOURCE_FILES}
)
target_include_directories(
	${TARGET_LIB} INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
	${TARGET_LIB} PRIVATE
		object_pool
		xml_parser
)
