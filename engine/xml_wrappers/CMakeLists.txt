cmake_minimum_required (VERSION 3.8)
project("XML Tree Wrappers")

set(CMAKE_CXX_STANDARD_REQUIRED 17)

set(
	XML_WRAPPERS_HEADER_FILES
		"xml_wrappers.h"
		"xml_wrappers_builders.h"
)
set(
	XML_WRAPPERS_SOURCE_FILES
		"xml_wrappers.cpp"
		"xml_wrappers_builders.cpp"
)

set(TARGET_LIB xml_wrappers)

add_library(
	${TARGET_LIB} STATIC 
		${XML_WRAPPERS_HEADER_FILES} 
		${XML_WRAPPERS_SOURCE_FILES}
)
target_include_directories(
	${TARGET_LIB} PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(
	${TARGET_LIB} PRIVATE
		object_pool
		xml_parser
)
