cmake_minimum_required(VERSION 3.8)
project("Company Manager UI")

set(
	COMPANY_MANAGER_UI_HEADER_FILES
		"cm_gui_dll_interface.hpp"
		"main_window.h"
		"internal_workers.h"
		"message.h"
		"tree_item.h"
		"tree_model.h"
		"tree_model_command.h"
)
set(
	DIALOGS_HEADER_FILES
		"dialog_base.h"
		"new_entry_dialog.h"
		"entry_view.h"
)
set(
	COMPANY_MANAGER_UI_SOURCE_FILES
		"main_window.cpp"
		"message.cpp"
		"tree_item.cpp"
		"tree_model.cpp"
		"tree_model_command.cpp"
)
set(
	DIALOGS_SOURCE_FILES
		"new_entry_dialog.cpp"		
		"entry_view.cpp"		
)

set(
	COMPANY_MANAGER_UI_FORM_FILES
		"main_window_ui.ui"
)
set(
	DIALOGS_FORM_FILES
		"new_department_dialog.ui"
		"new_employee_dialog.ui"
		"department_view.ui"
		"employee_view.ui"
)

set(TARGET_DLL cm_gui)

add_library( 
	${TARGET_DLL} SHARED
		${COMPANY_MANAGER_UI_HEADER_FILES}
		${COMPANY_MANAGER_UI_SOURCE_FILES}
		${COMPANY_MANAGER_UI_FORM_FILES}
		${DIALOGS_HEADER_FILES}
		${DIALOGS_SOURCE_FILES}	
		${DIALOGS_FORM_FILES}
)
target_compile_definitions(
	${TARGET_DLL} PRIVATE
		CM_GUI_EXPORT
)
target_include_directories(
	${TARGET_DLL} INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}
)
#Графический интерфейс
target_link_libraries(
	${TARGET_DLL}
		PUBLIC Qt5::Widgets
		PRIVATE cm_engine
)