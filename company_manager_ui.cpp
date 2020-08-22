#include "company_manager_ui.h"

CompanyManagerUI::CompanyManagerUI(QWidget *parent)
	: QMainWindow(parent) {

	m_gui->setupUi(this);

	/*Подключение кнопок меню*/
	connect_menu_bar();
	update_undo_redo_buttons();

	/*Настройка древовидного отображения*/
	auto& tree_view{ *m_gui->tree_view };
	tree_view.setModel(m_tree_model.get());
	connect(tree_view.selectionModel(), &QItemSelectionModel::selectionChanged, this, &CompanyManagerUI::selection_changed);
	show_viewers(false);

	/*Инициализация и подключение вспомогательных виджетов*/
	initialize_stacked_viewers();
	connect_item_viewers();
}

void CompanyManagerUI::connect_menu_bar() {
	connect(m_gui->new_btn, &QAction::triggered, this, &CompanyManagerUI::create);
	connect(m_gui->open_btn, &QAction::triggered, this, &CompanyManagerUI::load);
	connect(m_gui->save_btn, &QAction::triggered, this, &CompanyManagerUI::save);
	connect(m_gui->save_as_btn, &QAction::triggered, this, &CompanyManagerUI::save_as);
	connect(m_gui->close_btn, &QAction::triggered, this, &CompanyManagerUI::close);
	connect(m_gui->exit_btn, &QAction::triggered, this, &CompanyManagerUI::exit);
	connect(m_gui->undo_btn, &QAction::triggered, this, &CompanyManagerUI::undo);
	connect(m_gui->redo_btn, &QAction::triggered, this, &CompanyManagerUI::redo);
	connect(m_gui->about_program, &QAction::triggered, this, &CompanyManagerUI::about_program);
}

void CompanyManagerUI::initialize_stacked_viewers() {
	auto& stacked_viewer{ *m_gui->stacked_viewer };
	m_item_viewers = {															//Такая инициализация полей обеспечивает защиту от утечки памяти
		new QWidget(this),														//при выбросе исключения в одном из вызовов new, 
		new DepartmentView(this),												//т.к. к тому моменту остальные объекты гарантированно будут 
		new EmployeeView(this)													//приняты во владение главным окном приложения
	};
	stacked_viewer.addWidget(m_item_viewers.dummy);
	stacked_viewer.addWidget(m_item_viewers.department);
	stacked_viewer.addWidget(m_item_viewers.employee);
}

void CompanyManagerUI::connect_item_viewers() {
	connect(m_gui->new_department_btn, &QPushButton::clicked, this, &CompanyManagerUI::new_department);
	connect(m_item_viewers.department, &DepartmentView::name_changed, this, &CompanyManagerUI::rename_department);
	connect(m_item_viewers.department, &DepartmentView::erased, this, &CompanyManagerUI::remove_department);
	connect(m_item_viewers.department, &DepartmentView::employee_added, this, &CompanyManagerUI::new_employee);
	connect(m_item_viewers.employee, &EmployeeView::surname_changed, this, &CompanyManagerUI::change_employee_surname);
	connect(m_item_viewers.employee, &EmployeeView::name_changed, this, &CompanyManagerUI::change_employee_name);
	connect(m_item_viewers.employee, &EmployeeView::middle_name_changed, this, &CompanyManagerUI::change_employee_middle_name);
	connect(m_item_viewers.employee, &EmployeeView::function_changed, this, &CompanyManagerUI::change_employee_function);
	connect(m_item_viewers.employee, &EmployeeView::salary_updated, this, &CompanyManagerUI::update_employee_salary);
	connect(m_item_viewers.employee, &EmployeeView::erased, this, &CompanyManagerUI::remove_employee);
}

void CompanyManagerUI::reset_redo_queues() {
	m_tasks->modify_xml.ResetRepeatQueue();
	m_tasks->tree_model.ResetRepeatQueue();
}


void CompanyManagerUI::update_undo_redo_buttons() {
	m_gui->undo_btn->setEnabled(
		static_cast<bool>(m_tasks->modify_xml.CanBeCanceled())
	);
	m_gui->redo_btn->setEnabled(
		static_cast<bool>(m_tasks->modify_xml.CanBeRepeated())
	);
}

void CompanyManagerUI::update_redo_undo_actions_after_editing() {
	reset_redo_queues();
	update_undo_redo_buttons();
}

void CompanyManagerUI::update_viewers() {
	select_viewer(																			//Обновляем данные в окне просмотра
		get_current_selection_index()
	);			
}

void CompanyManagerUI::setup_viewers() {
	m_tree_model->SetCompany(*m_company_manager);
	m_gui->stacked_viewer->setCurrentWidget(m_item_viewers.dummy);							//Устанавливаем пустое окно просмотра
	show_viewers(true);
}

void CompanyManagerUI::select_viewer(const QModelIndex& selected_item) {
	using ItemType = CompanyTreeModel::ItemType;
	auto item_type{ m_tree_model->GetItemType(selected_item) };
	if (!item_type) {
		m_gui->stacked_viewer->setCurrentWidget(m_item_viewers.dummy);
	}
	switch (*item_type) {
	case ItemType::Department: {
		update_department_viewer(selected_item);
		m_gui->stacked_viewer->setCurrentWidget(m_item_viewers.department);
	} break;
	case ItemType::Employee: {
		update_employee_viewer(selected_item);
		m_gui->stacked_viewer->setCurrentWidget(m_item_viewers.employee);
	} break;
	};
}

void CompanyManagerUI::update_department_viewer(const QModelIndex& selected_item) {
	m_item_viewers.department->SetDepartment(
		DepartmentView::view_info{ 
			selected_item, 
			std::get<const wrapper::Department*>(
				m_tree_model->GetItemNode(selected_item)
				)
		}
	);
}

void CompanyManagerUI::update_employee_viewer(const QModelIndex& selected_item) {
	m_item_viewers.employee->SetEmployee(
		EmployeeView::view_info{ 
			selected_item, 
			std::tuple{
				*m_tree_model->GetDepartmentNameRef(selected_item),		//Раз объект выбран - он точно существует и является подразделением или сотрудником
				std::get<const wrapper::Employee*>(
						m_tree_model->GetItemNode(selected_item)
				)
			}
		}
	);
}

void CompanyManagerUI::show_viewers(bool on) {
	if (on) {
		m_gui->company_viewer->show();
		m_gui->stacked_viewer->show();
	}
	else {
		m_gui->company_viewer->hide();
		m_gui->stacked_viewer->hide();
	}
}

void CompanyManagerUI::reset_helper() {														//Сброк при закрытии документа
	m_tasks->service.Process(command::file_io::Reset::make_instance(*m_company_manager));
	m_tasks->service.ResetQueues();															//Сброк очереди команд без очистки лога ошибок
	m_tasks->modify_xml.ResetQueues();	
	m_tasks->tree_model.ResetQueues();
	update_undo_redo_buttons();
	m_tree_model->Reset();
	m_item_viewers.department->Reset();
	m_item_viewers.employee->Reset();
	show_viewers(false);
}

bool CompanyManagerUI::create() {
	bool successfully_created{ false };

	PipelineBuilder()
		.AddHandler(
			[close_if_opened = &CompanyManagerUI::close_helper](CompanyManagerUI& cm_ui) {
				return std::invoke(close_if_opened, cm_ui, WarningMode::Hide);				//Закрываем ранее открытые документы. false, если пользователь отменил операцию
			})
		.AddHandler(
			[creator = &CompanyManagerUI::create_helper, &successfully_created](CompanyManagerUI& cm_ui) {
				successfully_created = std::invoke(creator, cm_ui);
				return successfully_created;
			})
		.AddHandler(
			[view_updater = &CompanyManagerUI::setup_viewers](CompanyManagerUI& cm_ui) {
				std::invoke(view_updater, cm_ui);
				return false;
			})
		.Assemble()->Process(*this);
	return successfully_created;
}

bool CompanyManagerUI::load() {
	bool successfully_loaded{ false };
	QString path;

	PipelineBuilder()
		.AddHandler(
			[request = &CompanyManagerUI::request_load_file_path, &path](CompanyManagerUI& cm_ui) {
				path = std::invoke(request, cm_ui);											
				return !path.isEmpty();														//В случае отмены операции старый документ останется открытым
			})
		.AddHandler(
			[close_if_opened = &CompanyManagerUI::close_helper](CompanyManagerUI& cm_ui) {
				return std::invoke(close_if_opened, cm_ui, WarningMode::Hide);				//Закрываем ранее открытые документы. false, если пользователь отмени операцию
			})
		.AddHandler(
			[set_path = &CompanyManagerUI::set_file_path, &path](CompanyManagerUI& cm_ui) {
				std::invoke(set_path, cm_ui, path);
				return true;
			})
		.AddHandler(
			[loader = &CompanyManagerUI::load_helper,
			result_handler = &CompanyManagerUI::handle_load_result, &successfully_loaded](CompanyManagerUI& cm_ui) {
				auto result{ std::invoke(loader, cm_ui) };
				if (result) {
					successfully_loaded = std::invoke(result_handler, cm_ui, *result);
				}
				return successfully_loaded;
			})
		.AddHandler(
			[view_updater = &CompanyManagerUI::setup_viewers](CompanyManagerUI& cm_ui) {
				std::invoke(view_updater, cm_ui);
				return false;
			})
		.Assemble()->Process(*this);
	return successfully_loaded;
}

bool CompanyManagerUI::save() {
	bool successfully_saved{ false };

	PipelineBuilder()
		.AddHandler(
			[get_path = &CompanyManagerUI::get_current_file_path,
			checker = &CompanyManagerUI::is_writable,
			save_as = &CompanyManagerUI::save_as, &successfully_saved](CompanyManagerUI& cm_ui) {
				QString path{ std::invoke(get_path, cm_ui) };
				if (
					path.isEmpty()
					|| !std::invoke(checker, cm_ui,path)
				){																	//Если путь пуст или некорректен - переходим в "Сохранить как"
					successfully_saved = std::invoke(save_as, cm_ui);
					return false;
				}
				return true;														//Иначе продолжаем движение по цепочке обработчиков
			})
		.AddHandler(
			[loaded = &CompanyManagerUI::is_loaded, warning = &CompanyManagerUI::not_loaded_msg](CompanyManagerUI& cm_ui) {
				if (!std::invoke(loaded, cm_ui)) {
					std::invoke(warning, cm_ui);
					return false;
				}
				return true;
			})
		.AddHandler(
			[saved = &CompanyManagerUI::is_saved, warning = &CompanyManagerUI::already_saved_msg](CompanyManagerUI& cm_ui) {
				if (std::invoke(saved, cm_ui)) {
					std::invoke(warning, cm_ui);
					return false;
				}
				return true;
			})
		.AddHandler(
			[saver = &CompanyManagerUI::save_helper, 
			result_handler = &CompanyManagerUI::handle_save_result, &successfully_saved](CompanyManagerUI& cm_ui) {
				auto result{ std::invoke(saver, cm_ui) };
				if (result) {
					successfully_saved = std::invoke(result_handler, cm_ui, *result);
				}
				return false;														//Завершаем цепочку вручную, 
			})																		//чтобы не производить дополнительную проверку в pass_on Worker'a
		.Assemble()->Process(*this);
	return successfully_saved;
}


bool CompanyManagerUI::save_as() {
	QString path;
	bool successfully_saved{ false };

	PipelineBuilder()
		.AddHandler(
			[loaded = &CompanyManagerUI::is_loaded, warning = &CompanyManagerUI::not_loaded_msg](CompanyManagerUI& cm_ui) {
				if (!std::invoke(loaded, cm_ui)) {
					std::invoke(warning, cm_ui);
					return false;
				}
				return true;
			})
		.AddHandler(
			[saved = &CompanyManagerUI::is_saved, warning = &CompanyManagerUI::already_saved_msg](CompanyManagerUI& cm_ui) {
				if (std::invoke(saved, cm_ui)) {
					std::invoke(warning, cm_ui);
					return false;
				}
				return true;
			})
		.AddHandler(
			[request = &CompanyManagerUI::request_save_file_path, &path](CompanyManagerUI& cm_ui) {
				path = std::invoke(request, cm_ui);
				return !path.isEmpty();													//Если путь пустой - прекращаем обработку (операция отменена пользователем)
			})
		.AddHandler(
			[set_path = &CompanyManagerUI::set_file_path, &path](CompanyManagerUI& cm_ui) {
				std::invoke(set_path, cm_ui, path);
				return true;
			})
		.AddHandler(
			[saver = &CompanyManagerUI::save_helper,
			result_handler = &CompanyManagerUI::handle_save_result, &successfully_saved](CompanyManagerUI& cm_ui) {
				auto result{ std::invoke(saver, cm_ui) };
				if (result) {
					successfully_saved = std::invoke(result_handler, cm_ui, *result);
				}
				return false;													//Завершаем цепочку вручную, 
			})																	//чтобы не производить дополнительную проверку в pass_on Worker'a
		.Assemble()->Process(*this);
	return successfully_saved;
}

void CompanyManagerUI::close() {
	close_helper(WarningMode::Show);											//Предупреждение, если нет открытых документов
}

bool CompanyManagerUI::close_helper(WarningMode no_loaded) {
	bool not_cancelled{ true };

	PipelineBuilder()	
		.AddHandler(
			[loaded = &CompanyManagerUI::is_loaded, no_loaded,
			warning = &CompanyManagerUI::not_loaded_msg](CompanyManagerUI& cm_ui) {
				if (!std::invoke(loaded, cm_ui)) {
					if (no_loaded == WarningMode::Show) {
						std::invoke(warning, cm_ui);
					}		
					return false;
				}
				return true;
			})
		.AddHandler(															
			[saved = &CompanyManagerUI::is_saved, 
			save_request = &CompanyManagerUI::save_before_close_request,
			get_path = &CompanyManagerUI::get_current_file_path,
			name_extractor = &CompanyManagerUI::extract_file_name,
			saver = &CompanyManagerUI::save, &not_cancelled](CompanyManagerUI& cm_ui) {
				if (!std::invoke(saved, cm_ui)) {
					QString file_name{
						std::invoke(name_extractor, cm_ui, std::invoke(get_path, cm_ui))
					};
					auto user_answer{
						std::invoke(
							save_request, cm_ui, file_name
						)
					};
					switch (user_answer) {
						case QMessageBox::StandardButton::Save: not_cancelled = std::invoke(saver, cm_ui); break;		//В окне выбора пути операция может быть отменена
						case QMessageBox::StandardButton::Cancel: not_cancelled = false; break;
					}
				}
				return not_cancelled;
			})
		.AddHandler(
			[reset = &CompanyManagerUI::reset_helper](CompanyManagerUI& cm_ui) {
				std::invoke(reset, cm_ui);
				return false;
			})
		.Assemble()->Process(*this);
	return not_cancelled; 
}

void CompanyManagerUI::exit() {
	QWidget::close();
}

bool CompanyManagerUI::undo() {									//Порядок отмены операций един для команд различных типов
	bool wrapper_success{ false };
	PipelineBuilder()
		.AddHandler(
			[this, &wrapper_success](CompanyManagerUI&) {
				auto [_, result] {m_tasks->modify_xml.Cancel()};
				if (result != task::ResultType::Success) {
					handle_internal_fatal_error(m_tasks->modify_xml);
				}
				else {
					wrapper_success = true;
				}
				return true;									//Попытка изменить данные в TreeModel
			})
		.AddHandler(
			[this, &wrapper_success](CompanyManagerUI&) {
				auto [_, result] {m_tasks->tree_model.Cancel()};
				if (result != task::ResultType::Success) {
					handle_internal_fatal_error(m_tasks->modify_xml);
					if (wrapper_success) {
						wrapper_success = false;
						m_tasks->modify_xml.RepeatWithoutQueueing();		//Если обновление отбражения не удалось, но дерево данных обновлено успешно - отменяем изменения
					}
				}
				return false;									
			})
				.Assemble()->Process(*this);
	update_undo_redo_buttons();
	update_viewers();
	return wrapper_success;
}

bool CompanyManagerUI::redo() {									//Порядок повторного выполнения операций зависит от типа команды
	bool wrapper_success{ false };
	bool reversed_order{ *m_tasks->modify_xml.GetLastProcessedPurpose() == command::Purpose::Remove };

	auto redo_in_wrapper_tree{
		[this, &wrapper_success, reversed_order](CompanyManagerUI&) {
			auto [_, result] {m_tasks->modify_xml.Repeat()};
			if (result != task::ResultType::Success) {
				handle_internal_fatal_error(m_tasks->modify_xml);
				if (reversed_order) {
					m_tasks->tree_model.CancelWithoutQueueing();
				}
			}
			else {
				wrapper_success = true;
			}
			return wrapper_success;
		}
	};
	auto redo_in_tree_model{
		[this, &wrapper_success, reversed_order](CompanyManagerUI&) {
			auto [_, result] {m_tasks->tree_model.Repeat()};
			if (result != task::ResultType::Success) {
				handle_internal_fatal_error(m_tasks->modify_xml);
				if (wrapper_success) {
					wrapper_success = false;
				}
				if (!reversed_order) {
					m_tasks->modify_xml.CancelWithoutQueueing();
				}
			}
			else {
				wrapper_success = true;
			}
			return wrapper_success;
		}
	};		
	if (reversed_order) {
		redo_helper(std::ref(redo_in_tree_model), std::ref(redo_in_wrapper_tree)); //Передаем легкие объекты-"ссылки"
	}
	else {
		redo_helper(std::ref(redo_in_wrapper_tree), std::ref(redo_in_tree_model)); 
	}
	update_undo_redo_buttons();
	update_viewers();
	return wrapper_success;
}

QMessageBox::StandardButton CompanyManagerUI::show_message(
	message::Type type,
	QString title,
	std::optional<QString> text,
	QMessageBox::StandardButtons buttons
	) const {
	auto& factory{ *m_message_factories[static_cast<size_t>(type)] };
	factory
		.SetTitle(std::move(title))
		.SetButtons(buttons);
	if (text) {
		factory.SetText(std::move(*text));
	}
	return static_cast<QMessageBox::StandardButton>(
		factory.Create()->exec()
	);
}

void CompanyManagerUI::about_program() const {
	show_message(
		message::Type::Info,
		u8"О программе",
		u8"Company Manager - управление персоналом\n"
		u8"Обработка данных подразделений и сотрудников\n",
		QMessageBox::Button::Ok
	);
}

void CompanyManagerUI::selection_changed(const QItemSelection& selected, const QItemSelection& deselected) {
	select_viewer(selected.front().indexes().front());
}

bool CompanyManagerUI::new_department() {
	std::pair<wrapper::Company::department_view_it, bool> result_pair{ {}, false };

	QModelIndex current_selection_idx{ get_current_selection_index()};									//Индекс выделенного элемента
	bool append{																						//Добавление в конец
		!current_selection_idx.isValid()																//Нет выделенных элементов
		|| *m_tree_model->GetItemType(current_selection_idx) != CompanyTreeModel::ItemType::Department	//Валидность индекса уже проверена
		|| current_selection_idx.row() + 1 == m_tree_model->rowCount()									//Последний элемент в списке
	};

	wrapper::DepartmentBuilder builder;
	builder.SetAllocator(m_company_manager->GetAllocator());
	std::optional<wrapper::Department> department;														//Для проверки дубликатов перед вставкой

	PipelineBuilder()
		.AddHandler(
			[&builder, &msg_plantation = m_message_factories](CompanyManagerUI& cm_ui) {
				dialog::NewDepartment new_department_dialog(builder, msg_plantation);					//Диалоговое окно создания поздразделения
				return new_department_dialog.exec() == QDialog::Accepted;
			})
		.AddHandler(
			[&builder, &department, &company = m_company_manager->Read(),
			warning = &CompanyManagerUI::already_exists_msg](CompanyManagerUI& cm_ui) {
				department.emplace(builder.Assemble());
				if (company.Containts(department->GetName())) {
					std::invoke(
						warning, cm_ui,
						QString(u8"Подразделение \"") + QString::fromStdString(department->GetName()) + u8'\"' //уже сущесвует
					);
					return false;
				}
				return true;
			}
		)
		.AddHandler(
			[&result_pair, &department, &current_selection_idx, append,
			&tree_model = *m_tree_model,
			adder = &CompanyManagerUI::add_department_helper,
			inserter = &CompanyManagerUI::insert_department_helper](CompanyManagerUI& cm_ui) {	
				if (append) {
					result_pair = std::invoke(adder, cm_ui, std::move(*department));
				}
				else {																		
					result_pair = std::invoke(
						inserter, cm_ui, std::move(*department),
						*tree_model.GetDepartmentNameRef(tree_model.GetNextItem(current_selection_idx))
					);
				}
				return result_pair.second;																				//Если success - false, элемент будет удален
			})																									//из Company
		.AddHandler(
				[&result_pair, append, &current_selection_idx,
				&task_manager = m_tasks->tree_model,
				&tree_model = *m_tree_model,
				&company_manager = *m_company_manager,
				error_handler = &CompanyManagerUI::handle_internal_fatal_error<CompanyTreeModel>](CompanyManagerUI& cm_ui) {
				auto [value, result_type] {
					task_manager.Process(
						command::tree_model::InsertDepartment::make_instance(
							tree_model, company_manager, 
							result_pair.first->GetName(), 
							static_cast<size_t>(current_selection_idx.row()) + 1
						)
					)
				};
				if (result_type != task::ResultType::Success) {
					std::invoke(error_handler, cm_ui, task_manager);
				}
				else {
					result_pair.second = std::any_cast<bool>(value);
				}
				return !result_pair.second;														//Отмена операции и очистка в случае неудачи
			})
		.AddHandler(
			[&task_manager = m_tasks->modify_xml](CompanyManagerUI& cm_ui) {
				task_manager.CancelWithoutQueueing();
				return false;
			})
		.Assemble()->Process(*this);
	update_redo_undo_actions_after_editing();
	return result_pair.second;
}

bool CompanyManagerUI::rename_department(const DepartmentViewInfo& view_info, const QString& new_name) {
	if (new_name.isEmpty()) {
		empty_field_msg(u8"Наименование");
		return false;
	}
	if (!rename_department_helper(view_info, new_name)) {
		m_item_viewers.department->RestoreName();												//Возврат исходного значения
		return false;
	}
	reset_redo_queues();
	update_undo_redo_buttons();
	return true;
}

bool CompanyManagerUI::remove_department(const DepartmentViewInfo& view_info) {
	bool successfully_removed{ false };											//сначала - из TreeModel, потом - из CompanyManager
	wrapper::string_ref department_name{ extract_department_name(view_info) };

	PipelineBuilder()
		.AddHandler(
			[this, &view_info, &department_name](CompanyManagerUI& cm_ui) {
				auto [_, result_type] {
					m_tasks->tree_model.Process(
						command::tree_model::RemoveDepartment::make_instance(
							*m_tree_model,
							*m_company_manager,
							department_name,										//После удаления выделение сместится на другой элемент
							static_cast<size_t>(view_info.index.row())
						)
					)
				};
				if (result_type != task::ResultType::Success) {
					handle_internal_fatal_error(m_tasks->tree_model);
					return false;
				}
				return true;
			})
		.AddHandler(
			[this, &department_name, &successfully_removed](CompanyManagerUI& cm_ui) {
				auto [_, result_type] {
					m_tasks->modify_xml.Process(
						command::xml_wrapper::RemoveDepartment::make_instance(
							*m_company_manager,
							department_name
						)
					)
				};
				if (result_type != task::ResultType::Success) {
					handle_internal_fatal_error(m_tasks->modify_xml);
				}
				else {
					successfully_removed = true;
				}
				return false;													//Вручную останавливаем движение по цепочке обработчиков
			})
		.Assemble()->Process(*this);
	update_redo_undo_actions_after_editing();
	return successfully_removed;
}

bool CompanyManagerUI::new_employee(const DepartmentViewInfo& view_info) {
	bool successfully_inserted{ false };

	wrapper::EmployeeBuilder builder;
	builder.SetAllocator(m_company_manager->GetAllocator());
	std::variant<wrapper::Department::employee_view_it, wrapper::Employee> employee;						//Временное хранилище
																											//Команда вставки вовращает обычный итератор, а не view
	PipelineBuilder()
		.AddHandler(
			[&builder, &msg_plantation = m_message_factories](CompanyManagerUI& cm_ui) {
				dialog::NewEmployee new_employee_dialog(builder, msg_plantation);							//Диалоговое окно создания поздразделения
				return new_employee_dialog.exec() == QDialog::Accepted;
			})
		.AddHandler(
			[&builder, &employee, &view_info,
			warning = &CompanyManagerUI::already_exists_msg](CompanyManagerUI& cm_ui) {
				employee.emplace<wrapper::Employee>(builder.Assemble());
				wrapper::FullNameRef employee_name{ std::get<wrapper::Employee>(employee).GetFullName() };

				if (extract_department(view_info)->Containts(employee_name)) {
					std::invoke(
						warning, cm_ui,
						QString(u8"Сотрудник с ФИО \"") + CompanyTreeModel::FullNameRefToQString(employee_name) + u8'\"'
					);
					return false;
				}			
				return true;																				
			})																									//из Company
		.AddHandler(
			[&employee, &view_info,
			&task_manager = m_tasks->modify_xml,
			&company_manager = *m_company_manager,
			error_handler = &CompanyManagerUI::handle_internal_fatal_error<CompanyManager>](CompanyManagerUI& cm_ui) {
				auto [value, result_type] {
					task_manager.Process(
						command::xml_wrapper::InsertEmployee::make_instance(
							company_manager,
							extract_department_name(view_info),
							std::move(std::get<wrapper::Employee>(employee))								//Перемещаем сотрудника в команду
						)
					)
				};
				if (result_type != task::ResultType::Success) {
					std::invoke(error_handler, cm_ui, task_manager);
					return false;
				}
				employee = std::any_cast<wrapper::Department::employee_it>(value);							//Неявное преобразование в employee_view_it
				return true;														
			})
		.AddHandler(
			[&employee, &view_info,
			&task_manager = m_tasks->tree_model,
			&tree_model = *m_tree_model,
			&company_manager = *m_company_manager,
			error_handler = &CompanyManagerUI::handle_internal_fatal_error<CompanyTreeModel>](CompanyManagerUI& cm_ui) {
				auto [value, result_type] {
					task_manager.Process(
						command::tree_model::InsertEmployee::make_instance(
							tree_model,
							command::tree_model::WrapperInfo{
								std::addressof(company_manager.Read()),
								extract_department_name(view_info)
							},
							view_info.index.row(),												//Индекс подразделения
							std::get<wrapper::Department::employee_view_it>(employee)->second	//Передаем ссылку на вставленный элемент
						)
					)
				};
				if (result_type != task::ResultType::Success) {
					std::invoke(error_handler, cm_ui, task_manager);
					return false;
				}
				return !std::any_cast<bool>(value);
			})
		.AddHandler(
			[&task_manager = m_tasks->modify_xml](CompanyManagerUI& cm_ui) {					//Отмена операции и очистка в случае неудачи
				task_manager.CancelWithoutQueueing();
				return false;
			})
		.Assemble()->Process(*this);
	if (successfully_inserted) {
		m_item_viewers.department->UpdateStats();												//Т.к. операция добавления сотрудника выполняется из окна
	}																							//статистики подразделения, обновляем статистику
	update_redo_undo_actions_after_editing();
	return successfully_inserted;
}

bool CompanyManagerUI::remove_employee(const EmployeeViewInfo& view_info) {		//Удаление необходимо выполнять в обратном порядке:
	bool successfully_removed{ false };											//сначала - из TreeModel, потом - из CompanyManager
		
	PipelineBuilder()
		.AddHandler(
			[this, &view_info](CompanyManagerUI& cm_ui) {
				auto [_, result_type] {
					m_tasks->tree_model.Process(
						command::tree_model::RemoveEmployee::make_instance(
							*m_tree_model,
							command::tree_model::WrapperInfo{
								std::addressof(m_company_manager->Read()),
								extract_department_name(view_info)
							},
							view_info.index
						)
					)
				};
				if (result_type != task::ResultType::Success) {
					handle_internal_fatal_error(m_tasks->tree_model); 
					return false;
				}
				return true;
			})
		.AddHandler(
			[this, &view_info, &successfully_removed](CompanyManagerUI& cm_ui) {
				auto [_, result_type] {
						m_tasks->modify_xml.Process(
						command::xml_wrapper::RemoveEmployee::make_instance(
							*m_company_manager,
							extract_employee_personal_data(view_info)
						)
					)
				};
				if (result_type != task::ResultType::Success) {
					handle_internal_fatal_error(m_tasks->modify_xml);
				}
				else {
					successfully_removed = true;
				}
				return false;													//Вручную останавливаем движение по цепочке обработчиков
			})
		.Assemble()->Process(*this);
	update_redo_undo_actions_after_editing();
	return successfully_removed;
}

bool CompanyManagerUI::change_employee_surname(const EmployeeViewInfo& view_info, const QString& new_surname) {
	bool successfully_changed{ false };
	QString new_full_name;

	PipelineBuilder()
		.AddHandler(
			[&new_surname, warning = &CompanyManagerUI::empty_field_msg](CompanyManagerUI& cm_ui) {
				if (new_surname.isEmpty()) {
					std::invoke(warning, cm_ui, u8"Фамилия");
					return false;
				}
				return true;
			})
		.AddHandler(
			[&view_info, &new_surname, &new_full_name,
			&company_manager = *m_company_manager,
			&task_manager = m_tasks->modify_xml,
			warning = &CompanyManagerUI::warning_if_employee_already_exists,
			error_handler = &CompanyManagerUI::handle_internal_fatal_error<CompanyManager>,
			&successfully_changed](CompanyManagerUI& cm_ui) {
				auto [value, result_type] {
							task_manager.Process(
								command::xml_wrapper::ChangeEmployeeSurname::make_instance(
								company_manager,
								extract_employee_personal_data(view_info),
								new_surname.toStdString()
						)
					)
				};
				if (result_type != task::ResultType::Success) {
					std::invoke(error_handler, cm_ui, task_manager);
				}
				else {
					successfully_changed = std::invoke(
						warning, cm_ui, std::any_cast<wrapper::RenameResult>(value)
					);
				}
				return successfully_changed;
			})
		.AddHandler(
			[&successfully_changed, &new_full_name, &view_info,
			rename_helper = &CompanyManagerUI::rename_employee_in_tree_model](CompanyManagerUI& cm_ui) {
				new_full_name = CompanyTreeModel::FullNameRefToQString(
					extract_employee_personal_data(view_info).employee_name
				);																	 //Для обновления отображения после удачого редактирования дерева
				successfully_changed = std::invoke(rename_helper, cm_ui, view_info, new_full_name);
				return !successfully_changed;
			})
		.AddHandler(
			[&task_manager = m_tasks->modify_xml](CompanyManagerUI& cm_ui) {
				task_manager.CancelWithoutQueueing();
				return false;
			})
		.Assemble()->Process(*this);
	if (!successfully_changed) {
		m_item_viewers.employee->RestoreSurname();
	}
	update_redo_undo_actions_after_editing();
	return successfully_changed;
}

bool CompanyManagerUI::change_employee_name(const EmployeeViewInfo& view_info, const QString& new_name) {
	bool successfully_changed{ false };
	QString new_full_name;

	PipelineBuilder()
		.AddHandler(
			[&new_name, warning = &CompanyManagerUI::empty_field_msg](CompanyManagerUI& cm_ui) {
				if (new_name.isEmpty()) {
					std::invoke(warning, cm_ui, u8"Имя");
					return false;
				}
				return true;
			})
		.AddHandler(
			[&view_info, &new_name, &new_full_name,
			&company_manager = *m_company_manager,
			&task_manager = m_tasks->modify_xml,
			warning = &CompanyManagerUI::warning_if_employee_already_exists,
			error_handler = &CompanyManagerUI::handle_internal_fatal_error<CompanyManager>,
			&successfully_changed](CompanyManagerUI& cm_ui) {
				auto [value, result_type] {
							task_manager.Process(
								command::xml_wrapper::ChangeEmployeeName::make_instance(
								company_manager,
								extract_employee_personal_data(view_info),
								new_name.toStdString()
						)
					)	
				};
				if (result_type != task::ResultType::Success) {
				std::invoke(error_handler, cm_ui, task_manager);
				}
				else {
					successfully_changed = std::invoke(
						warning, cm_ui, std::any_cast<wrapper::RenameResult>(value)
					);
				}
				return successfully_changed;
			})
		.AddHandler(
			[&successfully_changed, &new_full_name, &view_info,
			rename_helper = &CompanyManagerUI::rename_employee_in_tree_model](CompanyManagerUI& cm_ui) {
				new_full_name = CompanyTreeModel::FullNameRefToQString(
					extract_employee_personal_data(view_info).employee_name
				);
				successfully_changed = std::invoke(rename_helper, cm_ui, view_info, new_full_name);
				return !successfully_changed;
			})
		.AddHandler(
			[&task_manager = m_tasks->modify_xml](CompanyManagerUI& cm_ui) {
				task_manager.CancelWithoutQueueing();
				return false;
			})
		.Assemble()->Process(*this);
	if (!successfully_changed) {
		m_item_viewers.employee->RestoreName();
	}
	update_redo_undo_actions_after_editing();
	return successfully_changed;
}

bool CompanyManagerUI::change_employee_middle_name(const EmployeeViewInfo& view_info, const QString& new_middle_name) {
	bool successfully_changed{ false };
	QString new_full_name;

	PipelineBuilder()
		.AddHandler(
			[&new_middle_name, empty_field_question = &CompanyManagerUI::empty_optional_field_msg_dlg](CompanyManagerUI& cm_ui) {
				if (new_middle_name.isEmpty()) {
					std::invoke(empty_field_question, cm_ui, u8"Отчество");
					return false;
			}
			return true;
			})
		.AddHandler(
			[&view_info, &new_middle_name, &new_full_name,
			&company_manager = *m_company_manager,
			&task_manager = m_tasks->modify_xml,
			warning = &CompanyManagerUI::warning_if_employee_already_exists,
			error_handler = &CompanyManagerUI::handle_internal_fatal_error<CompanyManager>,
			&successfully_changed](CompanyManagerUI& cm_ui) {
				auto [value, result_type] {
						task_manager.Process(
							command::xml_wrapper::ChangeEmployeeMiddleName::make_instance(
							company_manager,
							extract_employee_personal_data(view_info),
							new_middle_name.toStdString()
						)
					)
				};
				if (result_type != task::ResultType::Success) {
					std::invoke(error_handler, cm_ui, task_manager);
				}
				else {
					successfully_changed = std::invoke(
						warning, cm_ui, std::any_cast<wrapper::RenameResult>(value)
					);
				}
				return successfully_changed;
			})
		.AddHandler(
			[&successfully_changed, &new_full_name, &view_info,
			rename_helper = &CompanyManagerUI::rename_employee_in_tree_model](CompanyManagerUI& cm_ui) {
				new_full_name = CompanyTreeModel::FullNameRefToQString(
					extract_employee_personal_data(view_info).employee_name
				);
				successfully_changed = std::invoke(rename_helper, cm_ui, view_info, new_full_name);
				return !successfully_changed;
			})
		.AddHandler(
			[&task_manager = m_tasks->modify_xml](CompanyManagerUI& cm_ui) {
				task_manager.CancelWithoutQueueing();
				return false;
			})
		.Assemble()->Process(*this);
	if (!successfully_changed) {
			m_item_viewers.employee->RestoreMiddleName();
	}
	update_redo_undo_actions_after_editing();
	return successfully_changed;
}

bool CompanyManagerUI::change_employee_function(const EmployeeViewInfo& view_info, const QString& new_function) {
	bool successfully_changed{ false };

	PipelineBuilder()
		.AddHandler(
			[&new_function, empty_field_question = &CompanyManagerUI::empty_optional_field_msg_dlg](CompanyManagerUI& cm_ui) {
				if (new_function.isEmpty()) {
					return std::invoke(empty_field_question, cm_ui, u8"Должность") == QMessageBox::StandardButton::Ok;
				}
				return true;
			})
		.AddHandler(
			[&view_info, &new_function,
			&company_manager = *m_company_manager,
			&task_manager = m_tasks->modify_xml,
			error_handler = &CompanyManagerUI::handle_internal_fatal_error<CompanyManager>,
			&successfully_changed](CompanyManagerUI& cm_ui) {
				auto [_, result_type] {
							task_manager.Process(
							command::xml_wrapper::ChangeEmployeeFunction::make_instance(
							company_manager,
							extract_employee_personal_data(view_info),
							new_function.toStdString()
						)
					)
				};
				if (result_type != task::ResultType::Success) {
					std::invoke(error_handler, cm_ui, task_manager);
				}
				else {
					successfully_changed = true;
				}
				return false;
			})
		.Assemble()->Process(*this);
	if (!successfully_changed) {
		m_item_viewers.employee->RestoreFunction();														//Считывание из дерева и восстановление предыдущего значения
	}
	update_redo_undo_actions_after_editing();
	return successfully_changed;
}

bool CompanyManagerUI::update_employee_salary(const EmployeeViewInfo& view_info, wrapper::Employee::salary_t new_salary) {
	bool successfully_updated{ false };

	PipelineBuilder()
		.AddHandler(
			[new_salary, zero_salary_question = &CompanyManagerUI::zero_salary_msg_dlg](CompanyManagerUI& cm_ui) {
				if (!new_salary) {
					return std::invoke(zero_salary_question, cm_ui) == QMessageBox::StandardButton::Ok;
				}
				return true;
			})
		.AddHandler(
			[&view_info, new_salary,
			&company_manager = *m_company_manager,
			&task_manager = m_tasks->modify_xml,
			error_handler = &CompanyManagerUI::handle_internal_fatal_error<CompanyManager>,
			&successfully_updated](CompanyManagerUI& cm_ui) {
				auto [_, result_type] {
					task_manager.Process(
						command::xml_wrapper::UpdateEmployeeSalary::make_instance(
							company_manager,
							extract_employee_personal_data(view_info),
							new_salary
						)
					)
				};
				if (result_type != task::ResultType::Success) {
					std::invoke(error_handler, cm_ui, task_manager);
				}
				else {
					successfully_updated = true;
				}
				return false;
			})
		.Assemble()->Process(*this);
	if (!successfully_updated) {
		m_item_viewers.employee->RestoreSalary();														//Повторное считывание из дерева
	}
	update_redo_undo_actions_after_editing();
	return successfully_updated;
}


bool CompanyManagerUI::rename_department_helper(const DepartmentViewInfo& view_info, const QString& new_name) {
	bool successfully_renamed{ false };
	wrapper::string_ref old_name(extract_department_name(view_info));

	PipelineBuilder()
		.AddHandler(
			[&successfully_renamed, &old_name, &new_name,
			&task_manager = m_tasks->modify_xml,
			&company_manager = *m_company_manager,
			warning = &CompanyManagerUI::warning_if_department_already_exists,
			error_handler = &CompanyManagerUI::handle_internal_fatal_error<CompanyManager>](CompanyManagerUI& cm_ui) {
				auto [value, result_type] {
					task_manager.Process(
						command::xml_wrapper::RenameDepartment::make_instance(
							company_manager, old_name, new_name.toStdString()
						)
					)
				};
				if (result_type != task::ResultType::Success) {
					std::invoke(error_handler, cm_ui, task_manager);
					return false;
				}
				successfully_renamed = std::invoke(
					warning, cm_ui, std::any_cast<wrapper::RenameResult>(value)
				);
				return successfully_renamed;
			})
		.AddHandler(
			[&new_name, idx = view_info.index.row(),
			&tree_model = *m_tree_model,  &task_manager = m_tasks->tree_model,
			error_handler = &CompanyManagerUI::handle_internal_fatal_error<CompanyTreeModel>,
			&successfully_renamed](CompanyManagerUI& cm_ui) {
				auto [value, result_type] {
					task_manager.Process(
						command::tree_model::RenameDepartment::make_instance(tree_model, idx, new_name)
						)
				};
				if (result_type != task::ResultType::Success) {
					std::invoke(error_handler, cm_ui, task_manager);
				}
				else {
					successfully_renamed = std::any_cast<bool>(value);
				}
				return !successfully_renamed;
			})
		.AddHandler(
			[&task_manager = m_tasks->modify_xml](CompanyManagerUI& cm_ui) {
				task_manager.Cancel();
				return false;
			})
		.Assemble()->Process(*this);
	return successfully_renamed;
}

bool CompanyManagerUI::rename_employee_in_tree_model(const EmployeeViewInfo& view_info, const QString& new_full_name) {
	const auto& [q_idx, items] {view_info};
	auto [new_q_idx, result_type] {
		m_tasks->tree_model.Process(
			command::tree_model::ChangeEmployeeFullName::make_instance(
				*m_tree_model,
				command::tree_model::EmployeePersonalFile{
					static_cast<size_t>(q_idx.parent().row()),						//Позиция подразделения и сотрудника
					static_cast<size_t>(q_idx.row()) 
				},	
				new_full_name
			)
		)
	};
	if (result_type != task::ResultType::Success) {
		handle_internal_fatal_error(m_tasks->tree_model);
		return false;
	}																	//Обновляем индекс в Viewer'e, т.к. выделение сохранилось, а индекс - изменился!
	m_item_viewers.employee->UpdateModelIndex(
		std::any_cast<QModelIndex>(new_q_idx)
	);
	return true;
}

bool CompanyManagerUI::warning_if_employee_already_exists(wrapper::RenameResult result) const {
	if (result == wrapper::RenameResult::IsDuplicate) {
		already_exists_msg(u8"Сотрудник с таким ФИО");
		return false;
	}
	return true;
}
bool CompanyManagerUI::warning_if_department_already_exists(wrapper::RenameResult result) const {
	if (result == wrapper::RenameResult::IsDuplicate) {
		already_exists_msg(u8"Подразделение с таким наименованием");
		return false;
	}
	return true;
}

QModelIndex  CompanyManagerUI::get_current_selection_index() {
	return m_gui->tree_view->selectionModel()->currentIndex();
}

const wrapper::Department* CompanyManagerUI::extract_department(const DepartmentViewInfo& view_info) {
	return std::get<const wrapper::Department*>(view_info.items);
}


wrapper::string_ref CompanyManagerUI::extract_department_name(const DepartmentViewInfo& view_info) {
	return extract_department(view_info)->GetName();
}

wrapper::string_ref CompanyManagerUI::extract_department_name(const EmployeeViewInfo& view_info) {
	const auto& [department_name, _] {view_info.items};
	return department_name;
}

CompanyManagerUI::EmployeePersonalFile CompanyManagerUI::extract_employee_personal_data(const EmployeeViewInfo& view_info){
	const auto& [department_name, employee_it] {view_info.items};
	return {
		department_name,
		employee_it->GetFullName()
	};
}


void CompanyManagerUI::internal_fatal_error_msg(QString title, const std::string& error_definition) const noexcept {
	QString text(u8"Произошёл неисправимый сбой\n");
	text += u8"Системный лог: ";
	text += error_definition.empty() ? 
		u8"(пусто)" : error_definition.data();
	show_message(
		message::Type::Critical,
		std::move(title),
		text
	);
}

QString CompanyManagerUI::extract_file_name(const QString& filename) {
	m_file_handlers->info.setFile(filename);
	return m_file_handlers->info.baseName();
}

QMessageBox::StandardButton CompanyManagerUI::save_before_close_request(const QString& file_name) {
	QString question(u8"Сохранить изменения в файле");
	if (!file_name.isEmpty()) {
		question += ' ';
		question += file_name;
	}
	question += u8"?";
	return show_message(
		message::Type::Question,
		u8"Несохранённые изменения",
		question,
		QMessageBox::Button::Save | QMessageBox::Button::Discard | QMessageBox::Button::Cancel
	);
}

void CompanyManagerUI::empty_field_msg(const QString& field) const{
	QString text {u8"Поле \""};
	text += field;
	text += u8"\" должно быть заполнено";
	show_message(
		message::Type::Warning,
		u8"Некорректное редактирование",
		std::move(text),
		QMessageBox::Button::Ok
	);
}

void CompanyManagerUI::already_exists_msg(const QString& element_category) const {
	QString text(element_category);
	if (text.isEmpty()) {
		text = u8"Элемент";
	}
	text += u8" уже существует";
	show_message(
		message::Type::Warning,
		u8"Некорректное редактирование",
		std::move(text),
		QMessageBox::Button::Ok
	);
}

int CompanyManagerUI::empty_optional_field_msg_dlg(const QString& field) const {
	QString text(u8"Сохранить работника с незаполненным полем \"");
	text += field;
	text += "\"?";
	return show_message(
		message::Type::Question,
		u8"Ошибочное изменение",
		std::move(text),
		QMessageBox::Button::Yes | QMessageBox::Button::No
	);
}

int CompanyManagerUI::zero_salary_msg_dlg() const {
	return show_message(
		message::Type::Warning,
		u8"Ошибочное изменение",
		u8"Сохранить работника с нулевым значением зарплаты?",
		QMessageBox::Button::Yes | QMessageBox::Button::No
	);
}

QString CompanyManagerUI::request_load_file_path() {
	QString path;
	for (;;) {
		path = m_file_handlers->dialog.getOpenFileName(
			this,
			QObject::tr("Open file"),
			"",
			QObject::tr("XML files(*.xml)")
		);
		if (path.isEmpty() || is_readable(path)) {										//Пустой путь -> Пользователь нажал "Отмена"
			break;
		}
		else {
			invalid_load_path_msg();
		}
	}
	return path;
}

QString CompanyManagerUI::request_save_file_path() {
	QString path;
	for (;;) {
		path = m_file_handlers->dialog.getSaveFileName(
			this,
			QObject::tr("Open file"),
			"",
			QObject::tr("XML files(*.xml)")
		);							
		if (path.isEmpty() || is_writable(path)) {										//Пустой путь -> Пользователь нажал "Отмена"
			break;
		}
		else {
			invalid_save_path_msg();
		}
	}							
	return path;
}

void CompanyManagerUI::not_loaded_msg() const {
	show_message(
		message::Type::Warning,
		u8"Предупреждение",
		u8"Нет открытых документов",
		QMessageBox::Button::Ok
	);
}

void CompanyManagerUI::invalid_load_path_msg() const {
	show_message(
		message::Type::Warning,
		u8"Невозможно открыть файл",
		u8"Проверьте корректность пути и имени файла",
		QMessageBox::Button::Ok
	);
}

void CompanyManagerUI::invalid_save_path_msg() const {
	show_message(
		message::Type::Warning,
		u8"Невозможно сохранить файл",
		u8"Проверьте корректность пути и имени файла",
		QMessageBox::Button::Ok
	);
}

void CompanyManagerUI::unable_to_load_msg() const{
	show_message(
		message::Type::Critical,
		u8"Невозможно открыть файл",
		u8"Файл недоступен или поврёжден",
		QMessageBox::Button::Ok
	);
}

void CompanyManagerUI::unable_to_save_msg() const {
	show_message(
		message::Type::Critical,
		u8"Невозможно сохранить файл",
		u8"Файл недоступен или защищён от записи",
		QMessageBox::Button::Ok
	);
}

void CompanyManagerUI::already_saved_msg() {
	show_message(
		message::Type::Info,
		u8"Сохранение не требуется",
		u8"Файл не претерпел изменений с последнего сохранения",
		QMessageBox::Button::Ok
	);
}

bool CompanyManagerUI::is_readable(const QString& path) const {
	m_file_handlers->info.setFile(path);
	return m_file_handlers->info.isReadable();
}

bool CompanyManagerUI::is_writable(const QString& path) const {
	m_file_handlers->info.setFile(path);																//Если файла не существует - он будет создан
	return !m_file_handlers->info.exists() || m_file_handlers->info.isWritable();						//Иначе - проверка на возможность записи
}

bool CompanyManagerUI::is_loaded() const {
	return extract_value<bool>(
		m_tasks->service.Process(command::file_io::CheckLoaded::make_instance(*m_company_manager)).value
	);
}

bool CompanyManagerUI::is_saved() const {
	return extract_value<bool>(
		m_tasks->service.Process(command::file_io::CheckSaved::make_instance(*m_company_manager)).value
	);
}

QString CompanyManagerUI::get_current_file_path() {
	return extract_value<std::string_view>(
			m_tasks->service.Process(command::file_io::GetPath::make_instance(*m_company_manager)).value
	).data();
}

void CompanyManagerUI::set_file_path(const QString& path) {
	m_tasks->service.Process(
		command::file_io::SetPath::make_instance(*m_company_manager, path.toStdString())
	);
}

std::optional<worker::file_operation::Result> CompanyManagerUI::save_helper() {
	auto [value, result_type] {
			m_tasks->service.Process(
				command::file_io::Save::make_instance(*m_company_manager)
		)
	};
	if (result_type != task::ResultType::Success) {
		handle_internal_fatal_error(m_tasks->service);
		return std::nullopt;
	}
	return extract_value<worker::file_operation::Result>(value);
}

bool CompanyManagerUI::create_helper() {
	auto [_, result_type] {
		m_tasks->service.Process(
			command::file_io::CreateDocument::make_instance(*m_company_manager)
		)
	};
	if (result_type != task::ResultType::Success) {
		handle_internal_fatal_error(m_tasks->service);
		return false;
	}
	return true;
}

std::optional<worker::file_operation::Result> CompanyManagerUI::load_helper() {
	auto [value, result_type] {
		m_tasks->service.Process(
			command::file_io::Load::make_instance(*m_company_manager)
		)
	};
	if (result_type != task::ResultType::Success) {
		handle_internal_fatal_error(m_tasks->service);
		return std::nullopt;
	}
	return extract_value<worker::file_operation::Result>(value);
}

bool CompanyManagerUI::handle_load_result(worker::file_operation::Result result) {
	using worker::file_operation::Result;
	switch (result) {
	case Result::EmptyPath: case Result::FileOpenError: invalid_load_path_msg(); return false;
	case Result::NoData: case Result::FileIOError: unable_to_load_msg(); return false;
	default: return true;
	};
}

bool CompanyManagerUI::handle_save_result(worker::file_operation::Result result) {
	using worker::file_operation::Result;
	switch (result) {
	case Result::EmptyPath: case Result::FileOpenError: invalid_save_path_msg(); return false;
	case Result::NoData: case Result::FileIOError: unable_to_save_msg(); return false;
	default: return true;
	};
}

std::pair<wrapper::Company::department_view_it, bool> CompanyManagerUI::add_department_helper(wrapper::Department&& department) {
	bool successfully_added{ false };
	wrapper::Company::department_it added;

	PipelineBuilder()
		.AddHandler(
			[&department, &task_manager = m_tasks->modify_xml,
			&company_manager = *m_company_manager,
			error_handler = &CompanyManagerUI::handle_internal_fatal_error<CompanyManager>,
			&added , &successfully_added](CompanyManagerUI& cm_ui) {
				auto [value, result_type] {
					task_manager.Process(
						command::xml_wrapper::AddDepartment::make_instance(company_manager, std::move(department))
					)
				};
				successfully_added = (result_type == task::ResultType::Success);						//Вычисление логического выражения
				if (!successfully_added) {
					std::invoke(error_handler, cm_ui, task_manager);
				}
				else {
					added = std::any_cast<wrapper::Company::department_it>(value);
				}
				return successfully_added;
			})
		.Assemble()->Process(*this);
	return { added, successfully_added };
}

std::pair<wrapper::Company::department_view_it, bool> CompanyManagerUI::insert_department_helper(wrapper::Department&& department, wrapper::string_ref before) {
	bool successfully_inserted{ false };
	wrapper::Company::department_it inserted;

	PipelineBuilder()
		.AddHandler(
			[&department, &before, &task_manager = m_tasks->modify_xml,
			&company_manager = *m_company_manager,
			error_handler = &CompanyManagerUI::handle_internal_fatal_error<CompanyManager>,
			&successfully_inserted, &inserted](CompanyManagerUI& cm_ui) {
				auto [value, result_type] {
					task_manager.Process(
						command::xml_wrapper::InsertDepartment::make_instance(company_manager, before, std::move(department))
					)
				};
				successfully_inserted = (result_type == task::ResultType::Success);						//Вычисление логического выражения
				if (!successfully_inserted) {
					std::invoke(error_handler, cm_ui, task_manager);
				}
				else {
					inserted = std::any_cast<wrapper::Company::department_it>(value);
				}
				return successfully_inserted;
			})
		.Assemble()->Process(*this);
	return { inserted, successfully_inserted };
}

void CompanyManagerUI::closeEvent(QCloseEvent* event) {
	PipelineBuilder()
		.AddHandler(
			[close = &CompanyManagerUI::close_helper, event](CompanyManagerUI& cm_ui) {
				if (!std::invoke(close, cm_ui, WarningMode::Hide)) {
					event->ignore();														//Файл не был сохранён - пользователь отменил операцию
					return false;
				}
				return true;
			})
		.AddHandler(
			[event](CompanyManagerUI& cm_ui) {
				event->accept();
				return false;
			})
		.Assemble()->Process(*this);
}
