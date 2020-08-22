cmake_minimum_required(VERSION 3.8)
project(ObjectPool)

set(CMAKE_CXX_STANDARD_REQUIRED 17)

set (
	OBJECT_POOL_HEADER_FILES
		pool_allocator.h
		object_pool.h
)

#Линковка статической библиотеки
add_library(
	ObjectPool STATIC 
		${OBJECT_POOL_HEADER_FILES}
)

#Настройка директории поиска заголовочных файлов библиотеки
target_include_directories(
	ObjectPool PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}
)
