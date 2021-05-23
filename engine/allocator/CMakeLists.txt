cmake_minimum_required(VERSION 3.8)
project(ObjectPool)

set (
	OBJECT_POOL_HEADER_FILES
		"memory_management.h"
		"pool_allocator_base.h"
		"pool_allocator.h"
		"object_pool.h"
)
set(TARGET_LIB object_pool)

#Линковка статической библиотеки
add_library(
	${TARGET_LIB} INTERFACE
		${OBJECT_POOL_HEADER_FILES}
)

#Настройка директории поиска заголовочных файлов библиотеки
target_include_directories(
	${TARGET_LIB} INTERFACE 
		${CMAKE_CURRENT_SOURCE_DIR}
)
