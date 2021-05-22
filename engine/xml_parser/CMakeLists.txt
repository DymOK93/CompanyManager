cmake_minimum_required (VERSION 3.8)
project("XML Parser")

set(
	XML_PARSER_HEADER_FILES
		"range.h"
		"xml.h"
		"xml_parse.h"
		"xml_serialize.h"
		"xml_exceptions.h"
		"builder_base.h"
		"xml_node_builders.h"
)
set(
	XML_PARSER_SOURCE_FILES 
		"xml.cpp"
		"xml_parse.cpp"
		"xml_serialize.cpp"
)

set(TARGET_LIB xml_parser)

add_library(
	${TARGET_LIB} STATIC
		${XML_PARSER_HEADER_FILES} 
		${XML_PARSER_SOURCE_FILES}
)
target_include_directories(
	${TARGET_LIB} PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(
	${TARGET_LIB} PRIVATE
		object_pool
)