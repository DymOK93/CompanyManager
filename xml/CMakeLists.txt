cmake_minimum_required (VERSION 3.8)
project(XML)

set(CMAKE_CXX_STANDARD_REQUIRED 17)

set(
	XML_HEADER_FILES
		range.h
		xml.h
		xml_parse.h
		xml_serialize.h
		xml_exceptions.h
		builder_base.h
		xml_node_builders.h
)
set(
	XML_SOURCE_FILES 
		xml.cpp
		xml_parse.cpp
		xml_serialize.cpp
)

add_library(XML STATIC ${XML_HEADER_FILES} ${XML_SOURCE_FILES})
target_include_directories(XML PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(XML ObjectPool)