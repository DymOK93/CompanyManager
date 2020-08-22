cmake_minimum_required (VERSION 3.8)
project(XmlWrappers)

set(CMAKE_CXX_STANDARD_REQUIRED 17)

set(
	XML_WRAPPERS_HEADER_FILES
		xml_wrappers.h
		xml_wrappers_builders.h
)
set(
	XML_WRAPPERS_SOURCE_FILES
		xml_wrappers.cpp
		xml_wrappers_builders.cpp
)

#Объявляем проект как статическую библиотеку и добавляем в него все исходники
add_library(XmlWrappers STATIC ${XML_WRAPPERS_HEADER_FILES} ${XML_WRAPPERS_SOURCE_FILES})

target_include_directories(XmlWrappers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(XmlWrappers XML)
