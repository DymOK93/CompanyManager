cmake_minimum_required(VERSION 3.8)
project(CompanyManagerUI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

set(
	COMPANY_MANAGER_UI_HEADER_FILES
		company_manager_ui.h
		internal_workers.h
		message.h
		tree_item.h
		tree_model.h
		tree_model_command.h
)
set(
	DIALOGS_HEADER_FILES
		dialog_base.h
		new_entry_dialog.h
		entry_view.h
)
set(
	COMPANY_MANAGER_UI_SOURCE_FILES
		company_manager_ui.cpp
		message.cpp
		tree_item.cpp
		tree_model.cpp	
		tree_model_command.cpp
)
set(
	DIALOGS_SOURCE_FILES
		new_entry_dialog.cpp		
		entry_view.cpp		
)

set(
	COMPANY_MANAGER_UI_FORM_FILES
		company_manager_ui.ui
)
set(
	DIALOGS_FORM_FILES
		new_department_dialog.ui
		new_employee_dialog.ui
		department_view.ui
		employee_view.ui
)

add_executable(
	CompanyManagerUI
		main.cpp
		${COMPANY_MANAGER_UI_HEADER_FILES}
		${COMPANY_MANAGER_UI_SOURCE_FILES}
		${COMPANY_MANAGER_UI_FORM_FILES}
		${DIALOGS_HEADER_FILES}
		${DIALOGS_SOURCE_FILES}	
		${DIALOGS_FORM_FILES}
  )

#������� ���������
add_subdirectory(allocator)
target_link_libraries(CompanyManagerUI ObjectPool)

#���������� ��� ������ � XML
add_subdirectory(xml)
target_link_libraries(CompanyManagerUI XML)

#������ ��� XML-������
add_subdirectory(xml_wrappers)
target_link_libraries(CompanyManagerUI XmlWrappers)

#��������� ������� ������
add_subdirectory(chain_workers)
target_link_libraries(CompanyManagerUI ChainWorkers)

#C������ ������
add_subdirectory(commands)
target_link_libraries(CompanyManagerUI OperationManagement)

#�������� ����� (��������� �����������, ������� � �������� ��������)
add_subdirectory(task_manager)
target_link_libraries(CompanyManagerUI TaskManager)

#������ CompanyManager
add_subdirectory(company_manager_engine)
target_link_libraries(CompanyManagerUI CompanyManagerEngine)

#����������� ���������
target_link_libraries(CompanyManagerUI Qt5::Widgets)