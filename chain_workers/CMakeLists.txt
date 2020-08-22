cmake_minimum_required(VERSION 3.8)
project(ChainWorkers)

set(CMAKE_CXX_STANDARD_REQUIRED 17)

set (
	CHAIN_WORKERS_HEADER_FILES
		worker_interface.h
		file_workers.h
)

set (
	CHAIN_WORKERS_SOURCE_FILES
		file_workers.cpp
)

add_library(
	ChainWorkers STATIC 
		${CHAIN_WORKERS_HEADER_FILES} 
		${CHAIN_WORKERS_SOURCE_FILES}
)
target_include_directories(
	ChainWorkers PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ChainWorkers XML)
target_link_libraries(ChainWorkers ObjectPool)
