cmake_minimum_required(VERSION 3.8)
project(ObjectPool)

set(CMAKE_CXX_STANDARD_REQUIRED 17)

set (
	OBJECT_POOL_HEADER_FILES
		memory_management.h
		pool_allocator_base.h
		pool_allocator.h
		object_pool.h
)

#�������� ����������� ����������
add_library(
	ObjectPool STATIC 
		${OBJECT_POOL_HEADER_FILES}
)

#��������� ���������� ������ ������������ ������ ����������
target_include_directories(
	ObjectPool PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}
)
