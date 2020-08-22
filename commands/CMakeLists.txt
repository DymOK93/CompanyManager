cmake_minimum_required(VERSION 3.8)
project(OperationManagement)

set(CMAKE_CXX_STANDARD_REQUIRED 17)

set (
	OPERATION_MANAGEMENT_HEADER_FILES
		command_interface.h
		file_io_command.h
		xml_wrapper_command.h
)

set (
	OPERATION_MANAGEMENT_HEADER_FILES
		file_io_command.cpp
		xml_wrapper_command.cpp
)

add_library(
	OperationManagement STATIC
		${OPERATION_MANAGEMENT_HEADER_FILES} 
		${OPERATION_MANAGEMENT_SOURCE_FILES}
)
target_include_directories(
	OperationManagement PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(OperationManagement ObjectPool)
target_link_libraries(OperationManagement CompanyManagerEngine)
