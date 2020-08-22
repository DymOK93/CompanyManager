cmake_minimum_required(VERSION 3.8)
project(CompanyManagerEngine)

set(CMAKE_CXX_STANDARD_REQUIRED 17)

set (
	COMPANY_MANAGER_ENGINE_HEADER_FILES
		company_manager_engine.h
)

set (
	COMPANY_MANAGER_ENGINE_SOURCE_FILES
		company_manager_engine.cpp
)

add_library(
	CompanyManagerEngine STATIC 
	${COMPANY_MANAGER_ENGINE_HEADER_FILES} 
	${COMPANY_MANAGER_ENGINE_SOURCE_FILES}
)
target_include_directories(
	CompanyManagerEngine PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(CompanyManagerEngine XmlWrappers)
target_link_libraries(CompanyManagerEngine ChainWorkers)
