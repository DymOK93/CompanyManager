#pragma once
/*Engine headers*/
#include "task_manager.h"
#include "internal_workers.h"
#include "file_io_command.h"
#include "xml_wrapper_command.h"
#include "tree_model_command.h"
#include "company_manager_engine.h"
#include "message.h"
#include "tree_model.h"
#include "entry_view.h"
#include "new_entry_dialog.h"

/*GUI forms*/
#include "ui_company_manager_ui.h"

/*Standart Library headers*/
#include <memory>
#include <array>
#include <optional>
#include <unordered_map>
#include <type_traits>
#include <functional>

/*Qt headers*/
#include <QString>
#include <QtWidgets>
#include <QFile>
#include <QEvent>

class CompanyManagerUI : public QMainWindow {
    Q_OBJECT
public:
	using PipelineBuilder = worker::ui_internal::PipelineBuilder<CompanyManagerUI>;
	using DepartmentViewInfo = DepartmentView::view_info;
	using EmployeeViewInfo = EmployeeView::view_info;
	using EmployeePersonalFile = command::xml_wrapper::EmployeePersonalFile;
private:
	enum class WarningMode {														//����� ������ ��������������� ���������
		Hide,
		Show
	};
	struct TaskManagement {
		TaskManager<CompanyManager> service{ TaskManager<CompanyManager>(0) };		//������� �� ����������� � �������� ��� ������
		TaskManager<CompanyManager> modify_xml;										//����������� ������� ������� ������
		TaskManager<CompanyTreeModel> tree_model;
	};
	struct FileHandlers {
		mutable QFileInfo info;
		QFileDialog dialog;
	};
	struct ItemViewers {											//��������� ����� ������� QStackedWidget 
		QWidget* dummy;												//��������, ���� ��������� �����������
		DepartmentView* department;
		EmployeeView* employee;
	};
public:
	CompanyManagerUI(QWidget *parent = nullptr);
public slots:
	/*File menu*/
	bool create();
	bool load();
	bool save();
	bool save_as();
	void close();																		
	void exit();

	/*Edit menu*/
	bool undo();
	bool redo();

	/*About program menu*/
	void about_program() const;
private slots:
	void selection_changed(const QItemSelection& selected, const QItemSelection& deselected);
	bool new_department();
	bool rename_department(const DepartmentViewInfo& view_info, const QString& new_name);
	bool new_employee(const DepartmentViewInfo& view_info);
	bool remove_department(const DepartmentViewInfo& view_info);

	bool change_employee_surname(const EmployeeViewInfo& view_info, const QString& new_middle_name);		
	bool change_employee_name(const EmployeeViewInfo& view_info, const QString& new_middle_name);			
	bool change_employee_middle_name(const EmployeeViewInfo& view_info, const QString& new_middle_name);	
	bool change_employee_function(const EmployeeViewInfo& view_info, const QString& new_function);			
	bool update_employee_salary(const EmployeeViewInfo& view_info, wrapper::Employee::salary_t new_salary);
	bool remove_employee(const EmployeeViewInfo& view_info);
private:
	template <class FirstHandler, class SecondHandler>
	void redo_helper(FirstHandler&& first_handler, SecondHandler&& second_handler){
		PipelineBuilder()
			.AddHandler(std::forward<FirstHandler>(first_handler))
			.AddHandler(std::forward<SecondHandler>(second_handler))
			.Assemble()->Process(*this);
	}
	void connect_menu_bar();
	void initialize_stacked_viewers();
	void connect_item_viewers();

	void reset_redo_queues();										//����� ����������� ������� ��������
	void update_undo_redo_buttons();								//���������/����������� ������ "��������" � "�������"
	void update_redo_undo_actions_after_editing();					//���������� ��� ����������������� ��������
	void update_viewers();											//������ ���������� ����� ������ ��� ������� ��������
	void setup_viewers();											//��������� ����� �������� ������ �����
	void select_viewer(const QModelIndex& selected_item);
	void update_department_viewer(const QModelIndex& selected_item);
	void update_employee_viewer(const QModelIndex& selected_item);
	void show_viewers(bool on);
	void reset_helper();
	bool close_helper(WarningMode no_loaded);
	void closeEvent(QCloseEvent* event) override;					//�������� ������� ��� ������� ��������� �����
private:
	QMessageBox::StandardButton show_message(
		message::Type,
		QString title,
		std::optional<QString> text = std::nullopt,
		QMessageBox::StandardButtons buttons = QMessageBox::Button::NoButton
	) const;

	template <class TargetTy>
	void handle_internal_fatal_error(TaskManager<TargetTy>& tm) const noexcept {
		internal_fatal_error_msg(
			u8"����������� ������",
			tm.GetErrorLog().back()									
		);
	}

	void internal_fatal_error_msg(
		QString title,
		const std::string& error_definition = ""					//��������� operator+= ��������� �����������, � ��� ������ std::string, ��������
	) const noexcept;												//�� ������ � ���������� ������������ �� std::string_view::data() �������� ���� �������� QString

	QMessageBox::StandardButton save_before_close_request(				//� ������� ���������� ����� �������� ������� �� ������� ������������� ������
		const QString& file_name
	);						
	QString extract_file_name(const QString& filename);
private:
	void empty_field_msg(const QString& field) const;
	void already_exists_msg(const QString& element_category) const;
	int empty_optional_field_msg_dlg(const QString& field) const;
	int zero_salary_msg_dlg() const;

	QString request_load_file_path() const;
	QString request_save_file_path() const;

	void not_loaded_msg() const;
	void invalid_load_path_msg() const;
	void invalid_save_path_msg() const;
	void unable_to_load_msg() const;
	void unable_to_save_msg() const;
	void already_saved_msg();

	bool is_readable(const QString& path) const;
	bool is_writable(const QString& path) const;

	bool is_loaded() const;
	bool is_saved() const;
	QString get_current_file_path();
	void set_file_path(const QString& path);

	bool create_helper();
	std::optional<worker::file_operation::Result> load_helper();
	bool handle_load_result(worker::file_operation::Result result);
	std::optional<worker::file_operation::Result> save_helper();
	bool handle_save_result(worker::file_operation::Result result);

	std::pair<wrapper::Company::department_view_it, bool> add_department_helper(wrapper::Department&& department);	//��������� � ��������	
	std::pair<wrapper::Company::department_view_it, bool> insert_department_helper(wrapper::Department&& department, wrapper::string_ref before);
																													//�������� 1 ����� move c-tor
	bool rename_department_helper(const DepartmentViewInfo& view_info, const QString& new_name);
	bool rename_employee_in_tree_model(const EmployeeViewInfo& view_info, const QString& new_full_name);	//���������� ��������� ������ ����� ������� � ��������� �����

	bool warning_if_employee_already_exists(wrapper::RenameResult result) const;
	bool warning_if_department_already_exists(wrapper::RenameResult result) const;

	QModelIndex get_current_selection_index();
	static const wrapper::Department* extract_department(const DepartmentViewInfo& view_info);
	static wrapper::string_ref extract_department_name(const DepartmentViewInfo& view_info);
	static wrapper::string_ref extract_department_name(const EmployeeViewInfo& view_info);
	static EmployeePersonalFile extract_employee_personal_data(const EmployeeViewInfo& view_info);

	template <typename Ty>
	static Ty extract_value(std::any value) {
		return std::any_cast<Ty>(value);
	}
private:
	message::Plantation m_message_factories{
		message::MakeFactory<message::Question>(),
		message::MakeFactory<message::Info>(),
		message::MakeFactory<message::Warning>(),
		message::MakeFactory<message::Critical>()
	};

	std::unique_ptr<Ui::CompanyManagerGUI> m_gui{ std::make_unique<Ui::CompanyManagerGUI>() };
	std::unique_ptr<CompanyTreeModel> m_tree_model{ std::make_unique<CompanyTreeModel>() };
	ItemViewers m_item_viewers;

	std::unique_ptr<CompanyManager> m_company_manager{std::make_unique<CompanyManager>() };
	std::unique_ptr<TaskManagement> m_tasks{std::make_unique<TaskManagement>()};

	std::unique_ptr<FileHandlers> m_file_handlers{ std::make_unique<FileHandlers>() };
};
