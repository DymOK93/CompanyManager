cmake_minimum_required(VERSION 3.8)
project(CompanyManagerEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(
	CM_ENGINE_HEADER_FILES
		"engine.h"
		"file_io_command.h"
		"xml_wrapper_command.h"
)
set(
	CM_ENGINE_SOURCE_FILES
		"engine.cpp"
		"file_io_command.cpp"
		"xml_wrapper_command.cpp"
)

set(TARGET_DLL cm_engine)

#Пуловый аллокатор
add_subdirectory(allocator)

#Интерфейс цепочек команд
add_subdirectory(chain_workers)

#Cистема команд
add_subdirectory(commands)

#Менеджер задач (управляет выполнением, отменой и повтором операций)
add_subdirectory(task_manager)

#Библиотеки для работы с XML
add_subdirectory(xml_parser)

#Обёртки для XML-дерева
add_subdirectory(xml_wrappers)

add_library(
	${TARGET_DLL} SHARED
		${CM_ENGINE_HEADER_FILES}
		${CM_ENGINE_SOURCE_FILES}
)

target_link_libraries(
	${TARGET_DLL} PUBLIC
		object_pool
		chain_workers
		command_interface
		task_mgr
		xml_parser
		xml_wrappers
)