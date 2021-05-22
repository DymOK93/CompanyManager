cmake_minimum_required(VERSION 3.8)
cmake_policy(SET CMP0091 NEW)	# Äëÿ CMAKE_MSVC_RUNTIME_LIBRARY 
project(CompanyManager)

set(COMPANY_MANAGER_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "$COMPANY_MANAGER_DIR/exe")

set(
	CMAKE_MSVC_RUNTIME_LIBRARY 
		"MultiThreaded$<$<CONFIG:Debug>:Debug>"	# static MSVC runtime (/MT)
)	

set(
	COMPANY_MANAGER_SOURCE_FILES
		main.cpp
)

set(TARGET_EXE CompanyManager)

add_subdirectory(engine)
#add_subdirectory(interface)

add_executable(
	${TARGET_EXE}
		${COMPANY_MANAGER_SOURCE_FILES}
)
