#include "main_window.h"

/*GUI forms*/
#include "ui_main_window_ui.h"

struct CompanyManagerUI::Impl {
  /*������� ���������*/
  message::Plantation message_factories{
      message::MakeFactory<message::Question>(),
      message::MakeFactory<message::Info>(),
      message::MakeFactory<message::Warning>(),
      message::MakeFactory<message::Critical>()};

  /*GUI*/
  Ui::CompanyManagerGUI gui;
  CompanyTreeModel tree_model;
  ItemViewers item_viewers;

  /*�������� ������*/
  CompanyManager company_manager;
  TaskManagement tasks;
  FileHandlers file_handlers;
};

CompanyManagerUI::CompanyManagerUI(QWidget* parent)
    : QMainWindow(parent), m_impl{std::make_unique<Impl>()} {
  m_impl->gui.setupUi(this);

  /*����������� ������ ����*/
  connect_menu_bar();
  update_undo_redo_buttons();

  /*��������� ������������ �����������*/
  auto& tree_view{*m_impl->gui.tree_view};
  tree_view.setModel(std::addressof(m_impl->tree_model));
  connect(tree_view.selectionModel(), &QItemSelectionModel::selectionChanged,
          this, &CompanyManagerUI::selection_changed);
  show_viewers(false);

  /*������������� � ����������� ��������������� ��������*/
  initialize_stacked_viewers();
  connect_item_viewers();
}

CompanyManagerUI::~CompanyManagerUI() noexcept = default;

/*******************************************************************************************************************************
�� std::function � Small Object Optimization

��������, ��� std::function ���������� ��������� � heap ��� ��������
callable-��������. ��������� ��������� ������ - �������� ������ ���������, ���
������������ ���������� ����� Small Object Optimization ��� �������� ����������
�������. ������ ������ SSO-������ �� ���������������� (���� �������������, ���
�� �� ����� 8 ���� -
https://en.cppreference.com/w/cpp/utility/functional/function/function) � ��
����������� �������������� ���������. ������� ���� ��������, ��� �� GCC8.1 ��
���������� 16 ����, MSVC2019 ��������� ������ ����� �������������� �� 40 ����.
��������� ��� �������� ����� Worker'�� ������������ ��������� ���������,
�������� �� �������� �������������� ��������� ������. ������������ �� �������
�������� ������ (16 ����), ����� ������������ ������ this (�.�. ���� �����
������ ������ ������� �� ���������� ������ ����� ����� ������� ������������) �
������ �� ������ ���� ������������ ��������� ����������, ����
std::tuple<Types&...>, ����� �������������� ��������������� �������������� SOO.
********************************************************************************************************************************/

bool CompanyManagerUI::create() {
  bool successfully_created{false};

  PipelineBuilder()
      .AddHandler([this](bool& _) {
        return close_helper(
            WarningMode::Hide);  //��������� ����� �������� ���������. false,
                                 //���� ������������ ������� ��������
      })
      .AddHandler([this](bool& successfully_created) {
        successfully_created = create_helper();
        return successfully_created;
      })
      .AddHandler([this](bool& _) {
        setup_viewers();
        return false;
      })
      .Assemble()
      ->Process(successfully_created);
  return successfully_created;
}

bool CompanyManagerUI::load() {
  bool successfully_loaded{false};
  QString path;

  PipelineBuilder()
      .AddHandler([this, &path](bool& _) {
        path = request_load_file_path();
        return !path.isEmpty();  //� ������ ������ �������� ������ ��������
                                 //��������� ��������
      })
      .AddHandler([this](bool& _) {
        return close_helper(
            WarningMode::Hide);  //��������� ����� �������� ���������. false,
                                 //���� ������������ ������ ��������
      })
      .AddHandler([this, &path](bool& _) {
        set_file_path(path);
        return true;
      })
      .AddHandler([this](bool& successfully_loaded) {
        auto result{load_helper()};
        if (result) {
          successfully_loaded = handle_load_result(*result);
        } else {
          unable_to_load_msg();
        }
        return successfully_loaded;
      })
      .AddHandler([this](bool& _) {
        setup_viewers();
        return false;
      })
      .Assemble()
      ->Process(successfully_loaded);
  return successfully_loaded;
}

bool CompanyManagerUI::save() {
  bool successfully_saved{false};

  PipelineBuilder()
      .AddHandler([this](bool& successfully_saved) {
        QString path{get_current_file_path()};
        if (path.isEmpty() ||
            !is_writable(path)) {  //���� ���� ���� ��� ����������� - ���������
                                   //� "��������� ���"
          successfully_saved = save_as();
          return false;
        }
        return true;  //����� ���������� �������� �� ������� ������������
      })
      .AddHandler([this](bool& _) {
        if (!is_loaded()) {
          not_loaded_msg();
          return false;
        }
        return true;
      })
      .AddHandler([this](bool& _) {
        if (is_saved()) {
          already_saved_msg();
          return false;
        }
        return true;
      })
      .AddHandler([this](bool& successfully_saved) {
        auto result{save_helper()};
        if (result) {
          successfully_saved = handle_save_result(*result);
        } else {
          unable_to_save_msg();
        }
        return false;  //��������� ������� �������,
      })  //����� �� ����������� �������������� �������� � pass_on Worker'a
      .Assemble()
      ->Process(successfully_saved);
  return successfully_saved;
}

bool CompanyManagerUI::save_as() {
  QString path;
  bool successfully_saved{false};

  PipelineBuilder()
      .AddHandler([this](bool& _) {
        if (!is_loaded()) {
          not_loaded_msg();
          return false;
        }
        return true;
      })
      .AddHandler([this](bool& _) {
        if (is_saved()) {
          already_saved_msg();
          return false;
        }
        return true;
      })
      .AddHandler([this, &path](bool& _) {
        path = request_save_file_path();
        return !path.isEmpty();  //���� ���� ������ - ���������� ���������
                                 //(�������� �������� �������������)
      })
      .AddHandler([this, &path](bool& _) {
        set_file_path(path);
        return true;
      })
      .AddHandler([this](bool& successfully_saved) {
        auto result{save_helper()};
        if (result) {
          successfully_saved = handle_save_result(*result);
        }
        return false;  //��������� ������� �������,
      })  //����� �� ����������� �������������� �������� � pass_on Worker'a
      .Assemble()
      ->Process(successfully_saved);
  return successfully_saved;
}

void CompanyManagerUI::close() {
  close_helper(
      WarningMode::Show);  //��������������, ���� ��� �������� ����������
}

void CompanyManagerUI::exit() {
  QWidget::close();
}

bool CompanyManagerUI::undo() {  //������� ������ �������� ���� ��� ������
                                 //��������� �����
  auto& task_manager{m_impl->tasks};
  bool wrapper_success{false};
  PipelineBuilder()
      .AddHandler([this, &task_manager](bool& wrapper_success) {
        auto [_, result]{task_manager.modify_xml.Cancel()};
        if (result != task::ResultType::Success) {
          handle_internal_fatal_error(task_manager.modify_xml);
        } else {
          wrapper_success = true;
        }
        return true;  //������� �������� ������ � TreeModel
      })
      .AddHandler([this, &task_manager](bool& wrapper_success) {
        auto [_, result]{task_manager.tree_model.Cancel()};
        if (result != task::ResultType::Success) {
          handle_internal_fatal_error(task_manager.modify_xml);
          if (wrapper_success) {
            wrapper_success = false;
            task_manager.modify_xml
                .RepeatWithoutQueueing();  //���� ���������� ���������� ��
                                           //�������, �� ������ ������ ���������
                                           //������� - �������� ���������
          }
        }
        return false;
      })
      .Assemble()
      ->Process(wrapper_success);
  update_undo_redo_buttons();
  update_viewers();
  return wrapper_success;
}

bool CompanyManagerUI::redo() {  //������� ���������� ���������� ��������
                                 //������� �� ���� �������
  bool wrapper_success{false};
  bool reversed_order{
      m_impl->tasks.modify_xml.GetLastProcessedPurpose() ==
      command::Purpose::Remove  //��������� ����������� (�.�. ���������� ������)
                                //������� - �������� ��������
  };

  auto redo_in_wrapper_tree{[this, reversed_order](bool& wrapper_success) {
    auto [_, result]{m_impl->tasks.modify_xml.Repeat()};
    if (result != task::ResultType::Success) {
      handle_internal_fatal_error(m_impl->tasks.modify_xml);
      if (reversed_order) {
        m_impl->tasks.tree_model.CancelWithoutQueueing();
      }
    } else {
      wrapper_success = true;
    }
    return wrapper_success;
  }};
  auto redo_in_tree_model{[this, reversed_order](bool& wrapper_success) {
    auto [_, result]{m_impl->tasks.tree_model.Repeat()};
    if (result != task::ResultType::Success) {
      handle_internal_fatal_error(m_impl->tasks.modify_xml);
      if (wrapper_success) {
        wrapper_success = false;
      }
      if (!reversed_order) {
        m_impl->tasks.modify_xml.CancelWithoutQueueing();
      }
    } else {
      wrapper_success = true;
    }
    return wrapper_success;
  }};
  if (reversed_order) {
    wrapper_success = redo_helper(
        std::ref(redo_in_tree_model),
        std::ref(redo_in_wrapper_tree));  //�.�. ����������� - ���������
                                          //����������, ����� �������� ��
  }  //� ������� std::reference_wrapper
  else {
    wrapper_success = redo_helper(std::ref(redo_in_wrapper_tree),
                                  std::ref(redo_in_tree_model));
  }
  update_undo_redo_buttons();
  update_viewers();
  return wrapper_success;
}

void CompanyManagerUI::about_program() const {
  show_message(message::Type::Info, u8"� ���������",
               u8"Company Manager - ���������� ����������\n"
               u8"��������� ������ ������������� � �����������\n",
               QMessageBox::Button::Ok);
}

void CompanyManagerUI::selection_changed(const QItemSelection& selected,
                                         const QItemSelection& deselected) {
  const auto& index_list{selected.indexes()};
  if (index_list.isEmpty()) {  //�������� ��� ����������� �����������
    select_viewer(
        QModelIndex());  //��� ��������� �������-�������� � �������� �����������
  } else {
    select_viewer(index_list.front());  //�������� ����� ������ ������ ��������
  }
}

bool CompanyManagerUI::new_department() {
  bool successfully_inserted{false};

  QModelIndex current_selection_idx{
      get_current_selection_index()};  //������ ����������� ��������
  bool append{
      //���������� � �����
      !current_selection_idx.isValid()  //��� ���������� ���������
      || *m_impl->tree_model.GetItemType(current_selection_idx) !=
             CompanyTreeModel::ItemType::Department  //���������� ������� ���
                                                     //���������
      || current_selection_idx.row() + 1 ==
             m_impl->tree_model.rowCount()  //��������� ������� � ������
  };

  wrapper::DepartmentBuilder builder;
  builder.SetAllocator(m_impl->company_manager.GetAllocator());
  std::variant<std::monostate, Company::department_view_it, Department>
      department;  //��� �������� ���������� ����� ��������

  auto arg_tuple{std::tie(builder, department, current_selection_idx, append)};

  PipelineBuilder()
      .AddHandler([&builder,
                   &msg_factories = m_impl->message_factories](bool& _) {
        dialog::NewDepartment new_department_dialog(
            builder, msg_factories);  //���������� ���� �������� ��������������
        return new_department_dialog.exec() == QDialog::Accepted;
      })
      .AddHandler([this, &arg_tuple](bool& _) {
        auto& [builder, department_holder, _1,
               _2]{arg_tuple};  //������������� ������ �������.
        department_holder.emplace<Department>(
            builder.Assemble());  //������������ const � ������ auto& �����
                                  //�������� ������������� ��� �������������
        auto& department{std::get<Department>(department_holder)};

        if (m_impl->company_manager.Read().Containts(department.GetName())) {
          already_exists_msg(QString(u8"������������� \"") +
                             QString::fromStdString(department.GetName()) +
                             u8'\"'  //��� ���������
          );
          return false;
        }
        return true;
      })
      .AddHandler([this, &arg_tuple](bool& successfully_inserted) {
        auto& [builder, department_holder, current_selection_idx,
               append]{arg_tuple};
        if (append) {
          if (auto it = add_department_helper(
                  std::move(std::get<Department>(department_holder)));
              it.has_value()) {
            department_holder = *it;
            successfully_inserted = true;
          }
        } else {
          if (auto it = insert_department_helper(
                  std::move(std::get<Department>(department_holder)),
                  *m_impl->tree_model.GetDepartmentNameRef(
                      m_impl->tree_model.GetNextItem(
                          current_selection_idx)  //�������� ��� ��������,
                                                  //���������� �� ����������
                      ));
              it.has_value()) {
            department_holder = *it;
            successfully_inserted = true;
          }
        };
        return successfully_inserted;
      })
      .AddHandler([this, &arg_tuple](bool& successfully_inserted) {
        auto& [_, department_holder, current_selection_idx, append]{arg_tuple};
        auto [value, result_type]{m_impl->tasks.tree_model.Process(
            command::tree_model::InsertDepartment::make_instance(
                m_impl->tree_model, m_impl->company_manager,
                std::get<Company::department_view_it>(department_holder)
                    ->GetName(),
                static_cast<size_t>(current_selection_idx.row()) + 1))};
        if (result_type != task::ResultType::Success) {
          handle_internal_fatal_error(m_impl->tasks.tree_model);
        } else {
          successfully_inserted = std::any_cast<bool>(value);
        }
        return !successfully_inserted;  //������ �������� � ������� � ������
                                        //�������
      })
      .AddHandler([&task_manager = m_impl->tasks.modify_xml](bool& _) {
        task_manager.CancelWithoutQueueing();
        return false;
      })
      .Assemble()
      ->Process(successfully_inserted);
  update_redo_undo_actions_after_editing();
  return successfully_inserted;
}

bool CompanyManagerUI::remove_department(const DepartmentViewInfo& view_info) {
  bool successfully_removed{
      false};  //������� - �� TreeModel, ����� - �� CompanyManager
  wrapper::string_ref department_name{extract_department_name(view_info)};
  auto arg_tuple{std::tie(view_info, department_name)};

  PipelineBuilder()
      .AddHandler([this, &arg_tuple](bool& _0) {
        const auto& [view_info, department_name]{arg_tuple};
        auto [_1, result_type]{m_impl->tasks.tree_model.Process(
            command::tree_model::RemoveDepartment::make_instance(
                m_impl->tree_model, m_impl->company_manager,
                department_name,  //����� �������� ��������� ��������� �� ������
                                  //�������
                static_cast<size_t>(view_info.index.row())))};
        if (result_type != task::ResultType::Success) {
          handle_internal_fatal_error(m_impl->tasks.tree_model);
          return false;
        }
        return true;
      })
      .AddHandler([this, &arg_tuple](bool& successfully_removed) {
        const auto& [_0, department_name]{arg_tuple};
        auto [_1, result_type]{m_impl->tasks.modify_xml.Process(
            command::xml_wrapper::RemoveDepartment::make_instance(
                m_impl->company_manager, department_name))};
        if (result_type != task::ResultType::Success) {
          handle_internal_fatal_error(m_impl->tasks.modify_xml);
        } else {
          successfully_removed = true;
        }
        return false;  //������� ������������� �������� �� ������� ������������
      })
      .Assemble()
      ->Process(successfully_removed);
  update_redo_undo_actions_after_editing();
  return successfully_removed;
}

bool CompanyManagerUI::rename_department(const DepartmentViewInfo& view_info,
                                         const QString& new_name) {
  if (new_name.isEmpty()) {
    empty_field_msg(u8"������������");
    return false;
  }
  if (!rename_department_helper(view_info, new_name)) {
    m_impl->item_viewers.department
        ->RestoreName();  //������� ��������� ��������
    return false;
  }
  reset_redo_queues();
  update_undo_redo_buttons();
  return true;
}

bool CompanyManagerUI::new_employee(const DepartmentViewInfo& view_info) {
  bool successfully_inserted{false};

  wrapper::EmployeeBuilder builder;
  builder.SetAllocator(m_impl->company_manager.GetAllocator());
  std::variant<std::monostate, Department::employee_view_it, Employee>
      employee;  //��������� ���������

  auto arg_tuple{std::tie(builder, employee, view_info)};

  PipelineBuilder()
      .AddHandler([&builder,
                   &msg_factories = m_impl->message_factories](bool& _) {
        dialog::NewEmployee new_employee_dialog(
            builder, msg_factories);  //���������� ���� �������� ��������������
        return new_employee_dialog.exec() == QDialog::Accepted;
      })
      .AddHandler([this, &arg_tuple](bool& _) {
        auto& [builder, employee_holder, view_info]{arg_tuple};
        employee_holder.emplace<Employee>(builder.Assemble());
        const auto& employee_name{
            std::get<Employee>(employee_holder).GetFullName()};

        if (extract_department_ptr(view_info)->Containts(employee_name)) {
          already_exists_msg(
              QString(u8"��������� � ��� \"") +
              CompanyTreeModel::FullNameRefToQString(employee_name) + u8'\"');
          return false;
        }
        return true;
      })  //�� Company
      .AddHandler([this, &arg_tuple](bool& _0) {
        auto& [_1, employee_holder, view_info]{arg_tuple};
        auto [value, result_type]{m_impl->tasks.modify_xml.Process(
            command::xml_wrapper::InsertEmployee::make_instance(
                m_impl->company_manager, extract_department_name(view_info),
                std::move(std::get<Employee>(
                    employee_holder))  //���������� ���������� � �������
                ))};
        if (result_type != task::ResultType::Success) {
          handle_internal_fatal_error(m_impl->tasks.modify_xml);
          return false;
        }
        employee_holder = std::any_cast<Department::employee_it>(
            value);  //������� �������������� � employee_view_it
        return true;
      })
      .AddHandler([this, &arg_tuple](bool& successfully_inserted) {
        auto& [_, employee_holder, view_info]{arg_tuple};
        auto [value, result_type]{m_impl->tasks.tree_model.Process(
            command::tree_model::InsertEmployee::make_instance(
                m_impl->tree_model,
                command::tree_model::WrapperInfo{
                    std::addressof(m_impl->company_manager.Read()),
                    extract_department_name(view_info)},
                view_info.index.row(),  //������ �������������
                std::get<Department::employee_view_it>(employee_holder)
                    ->second  //�������� ������ �� ����������� �������
                ))};
        if (result_type != task::ResultType::Success) {
          handle_internal_fatal_error(m_impl->tasks.tree_model);
          return false;
        }
        successfully_inserted = std::any_cast<bool>(value);
        return !successfully_inserted;
      })
      .AddHandler([&task_manager = m_impl->tasks.modify_xml](
                      bool& _) {  //������ �������� � ������� � ������ �������
        task_manager.CancelWithoutQueueing();
        return false;
      })
      .Assemble()
      ->Process(successfully_inserted);
  if (successfully_inserted) {
    m_impl->item_viewers.department
        ->UpdateStats();  //�.�. �������� ����������
                          //���������� ����������� �� ����
  }  //���������� �������������, ��������� ����������
  update_redo_undo_actions_after_editing();
  return successfully_inserted;
}

bool CompanyManagerUI::remove_employee(
    const EmployeeViewInfo&
        view_info) {  //�������� ���������� ��������� � �������� �������:
  bool successfully_removed{
      false};  //������� - �� TreeModel, ����� - �� CompanyManager
  EmployeePersonalFile personal_data{extract_employee_personal_data(
      view_info)};  //���� �� ��������� ��������������, ����� �������� ��
                    // TreeModel ������ ��������� �
                    // extract_employee_personal_data() ����� ������ ��������,
  PipelineBuilder()  //���������� �� ��������!
      .AddHandler([this, &view_info](bool& _0) {
        auto [_1, result_type]{m_impl->tasks.tree_model.Process(
            command::tree_model::RemoveEmployee::make_instance(
                m_impl->tree_model,
                command::tree_model::WrapperInfo{
                    std::addressof(m_impl->company_manager.Read()),
                    extract_department_name(view_info)},
                view_info.index  // QModelIndex ���� ��� �������
                ))};
        if (result_type != task::ResultType::Success) {
          handle_internal_fatal_error(m_impl->tasks.tree_model);
          return false;
        }
        return true;
      })
      .AddHandler([this, &personal_data](bool& successfully_removed) {
        auto [_, result_type]{m_impl->tasks.modify_xml.Process(
            command::xml_wrapper::RemoveEmployee::make_instance(
                m_impl->company_manager, personal_data))};
        if (result_type != task::ResultType::Success) {
          handle_internal_fatal_error(m_impl->tasks.modify_xml);
        } else {
          successfully_removed = true;
        }
        return false;  //������� ������������� �������� �� ������� ������������
      })
      .Assemble()
      ->Process(successfully_removed);
  update_redo_undo_actions_after_editing();
  return successfully_removed;
}

bool CompanyManagerUI::change_employee_surname(
    const EmployeeViewInfo& view_info,
    const QString& new_surname) {
  bool successfully_changed{false};
  QString new_full_name;

  auto arg_tuple{std::tie(view_info, new_surname, new_full_name)};

  PipelineBuilder()
      .AddHandler([this, &new_surname](bool& _) {
        if (new_surname.isEmpty()) {
          empty_field_msg(u8"�������");
          return false;
        }
        return true;
      })
      .AddHandler([this, &arg_tuple](bool& successfully_changed) {
        auto& [view_info, new_surname, new_full_name]{arg_tuple};
        auto [value, result_type]{m_impl->tasks.modify_xml.Process(
            command::xml_wrapper::ChangeEmployeeSurname::make_instance(
                m_impl->company_manager,
                extract_employee_personal_data(view_info),
                new_surname.toStdString()))};
        if (result_type != task::ResultType::Success) {
          handle_internal_fatal_error(m_impl->tasks.modify_xml);
        } else {
          successfully_changed = warning_if_employee_already_exists(
              std::any_cast<wrapper::RenameResult>(value));
        }
        return successfully_changed;
      })
      .AddHandler([this, &arg_tuple](bool& successfully_changed) {
        auto& [view_info, new_surname, new_full_name]{arg_tuple};
        new_full_name = CompanyTreeModel::FullNameRefToQString(
            extract_employee_personal_data(view_info)
                .employee_name);  //��� ���������� ����������� ����� �������
                                  //�������������� ������
        successfully_changed =
            rename_employee_in_tree_model(view_info, new_full_name);
        return !successfully_changed;
      })
      .AddHandler([&task_manager = m_impl->tasks.modify_xml](bool& _) {
        task_manager.CancelWithoutQueueing();
        return false;
      })
      .Assemble()
      ->Process(successfully_changed);
  if (!successfully_changed) {
    m_impl->item_viewers.employee->RestoreSurname();
  }
  update_redo_undo_actions_after_editing();
  return successfully_changed;
}

bool CompanyManagerUI::change_employee_name(const EmployeeViewInfo& view_info,
                                            const QString& new_name) {
  bool successfully_changed{false};
  QString new_full_name;

  auto arg_tuple{std::tie(view_info, new_name, new_full_name)};

  PipelineBuilder()
      .AddHandler([this, &new_name](bool& _) {
        if (new_name.isEmpty()) {
          empty_field_msg(u8"���");
          return false;
        }
        return true;
      })
      .AddHandler([this, &arg_tuple](bool& successfully_changed) {
        auto& [view_info, new_name, new_full_name]{arg_tuple};
        auto [value, result_type]{m_impl->tasks.modify_xml.Process(
            command::xml_wrapper::ChangeEmployeeName::make_instance(
                m_impl->company_manager,
                extract_employee_personal_data(view_info),
                new_name.toStdString()))};
        if (result_type != task::ResultType::Success) {
          handle_internal_fatal_error(m_impl->tasks.modify_xml);
        } else {
          successfully_changed = warning_if_employee_already_exists(
              std::any_cast<wrapper::RenameResult>(value));
        }
        return successfully_changed;
      })
      .AddHandler([this, &arg_tuple](bool& successfully_changed) {
        auto& [view_info, _, new_full_name]{arg_tuple};
        new_full_name = CompanyTreeModel::FullNameRefToQString(
            extract_employee_personal_data(view_info).employee_name);
        successfully_changed =
            rename_employee_in_tree_model(view_info, new_full_name);
        return !successfully_changed;
      })
      .AddHandler([&task_manager = m_impl->tasks.modify_xml](bool& _) {
        task_manager.CancelWithoutQueueing();
        return false;
      })
      .Assemble()
      ->Process(successfully_changed);
  if (!successfully_changed) {
    m_impl->item_viewers.employee->RestoreName();
  }
  update_redo_undo_actions_after_editing();
  return successfully_changed;
}

bool CompanyManagerUI::change_employee_middle_name(
    const EmployeeViewInfo& view_info,
    const QString& new_middle_name) {
  bool successfully_changed{false};
  QString new_full_name;

  auto arg_tuple{std::tie(view_info, new_middle_name, new_full_name)};

  PipelineBuilder()
      .AddHandler([this, &new_middle_name](bool& _) {
        if (new_middle_name.isEmpty()) {
          empty_optional_field_msg_dlg(u8"��������");
          return false;
        }
        return true;
      })
      .AddHandler([this, &arg_tuple](bool& successfully_changed) {
        auto& [view_info, new_middle_name, new_full_name]{arg_tuple};
        auto [value, result_type]{m_impl->tasks.modify_xml.Process(
            command::xml_wrapper::ChangeEmployeeMiddleName::make_instance(
                m_impl->company_manager,
                extract_employee_personal_data(view_info),
                new_middle_name.toStdString()))};
        if (result_type != task::ResultType::Success) {
          handle_internal_fatal_error(m_impl->tasks.modify_xml);
        } else {
          successfully_changed = warning_if_employee_already_exists(
              std::any_cast<wrapper::RenameResult>(value));
        }
        return successfully_changed;
      })
      .AddHandler([this, &arg_tuple](bool& successfully_changed) {
        auto& [view_info, new_middle_name, new_full_name]{arg_tuple};
        new_full_name = CompanyTreeModel::FullNameRefToQString(
            extract_employee_personal_data(view_info).employee_name);
        successfully_changed =
            rename_employee_in_tree_model(view_info, new_full_name);
        return !successfully_changed;
      })
      .AddHandler([&task_manager = m_impl->tasks.modify_xml](bool& _) {
        task_manager.CancelWithoutQueueing();
        return false;
      })
      .Assemble()
      ->Process(successfully_changed);
  if (!successfully_changed) {
    m_impl->item_viewers.employee->RestoreMiddleName();
  }
  update_redo_undo_actions_after_editing();
  return successfully_changed;
}

bool CompanyManagerUI::change_employee_function(
    const EmployeeViewInfo& view_info,
    const QString& new_function) {
  bool successfully_changed{false};

  auto arg_tuple{std::tie(view_info, new_function)};

  PipelineBuilder()
      .AddHandler([this, &new_function](bool& _) {
        if (new_function.isEmpty()) {
          return empty_optional_field_msg_dlg(u8"���������") ==
                 QMessageBox::StandardButton::Ok;
        }
        return true;
      })
      .AddHandler([this, &arg_tuple](bool& successfully_changed) {
        auto& [view_info, new_function]{arg_tuple};
        auto [_, result_type]{m_impl->tasks.modify_xml.Process(
            command::xml_wrapper::ChangeEmployeeFunction::make_instance(
                m_impl->company_manager,
                extract_employee_personal_data(view_info),
                new_function.toStdString()))};
        if (result_type != task::ResultType::Success) {
          handle_internal_fatal_error(m_impl->tasks.modify_xml);
        } else {
          successfully_changed = true;
        }
        return false;
      })
      .Assemble()
      ->Process(successfully_changed);
  if (!successfully_changed) {
    m_impl->item_viewers.employee
        ->RestoreFunction();  //���������� �� ������ � ��������������
                              //����������� ��������
  }
  update_redo_undo_actions_after_editing();
  return successfully_changed;
}

bool CompanyManagerUI::update_employee_salary(const EmployeeViewInfo& view_info,
                                              Employee::salary_t new_salary) {
  bool successfully_updated{false};

  auto arg_tuple{std::tie(view_info, new_salary)};

  PipelineBuilder()
      .AddHandler([this, &new_salary](bool& _) {
        if (!new_salary) {
          return zero_salary_msg_dlg() == QMessageBox::StandardButton::Ok;
        }
        return true;
      })
      .AddHandler([this, &arg_tuple](bool& successfully_updated) {
        auto& [view_info, new_salary]{arg_tuple};
        auto [_, result_type]{m_impl->tasks.modify_xml.Process(
            command::xml_wrapper::UpdateEmployeeSalary::make_instance(
                m_impl->company_manager,
                extract_employee_personal_data(view_info), new_salary))};
        if (result_type != task::ResultType::Success) {
          handle_internal_fatal_error(m_impl->tasks.modify_xml);
        } else {
          successfully_updated = true;
        }
        return false;
      })
      .Assemble()
      ->Process(successfully_updated);
  if (!successfully_updated) {
    m_impl->item_viewers.employee
        ->RestoreSalary();  //��������� ���������� �� ������
  }
  update_redo_undo_actions_after_editing();
  return successfully_updated;
}

void CompanyManagerUI::connect_menu_bar() {
  connect(m_impl->gui.new_btn, &QAction::triggered, this,
          &CompanyManagerUI::create);
  connect(m_impl->gui.open_btn, &QAction::triggered, this,
          &CompanyManagerUI::load);
  connect(m_impl->gui.save_btn, &QAction::triggered, this,
          &CompanyManagerUI::save);
  connect(m_impl->gui.save_as_btn, &QAction::triggered, this,
          &CompanyManagerUI::save_as);
  connect(m_impl->gui.close_btn, &QAction::triggered, this,
          &CompanyManagerUI::close);
  connect(m_impl->gui.exit_btn, &QAction::triggered, this,
          &CompanyManagerUI::exit);
  connect(m_impl->gui.undo_btn, &QAction::triggered, this,
          &CompanyManagerUI::undo);
  connect(m_impl->gui.redo_btn, &QAction::triggered, this,
          &CompanyManagerUI::redo);
  connect(m_impl->gui.about_program, &QAction::triggered, this,
          &CompanyManagerUI::about_program);
}

void CompanyManagerUI::initialize_stacked_viewers() {
  auto& stacked_viewer{*m_impl->gui.stacked_viewer};
  m_impl->item_viewers = {
      //����� ������������� ����� ������������ ������ �� ������ ������
      new QWidget(this),  //��� ������� ���������� � ����� �� ������� new,
      new DepartmentView(
          this),  //�.�. � ���� ������� ��������� ������� �������������� �����
      new EmployeeView(this)  //������� �� �������� ������� ����� ����������
  };
  stacked_viewer.addWidget(m_impl->item_viewers.dummy);
  stacked_viewer.addWidget(m_impl->item_viewers.department);
  stacked_viewer.addWidget(m_impl->item_viewers.employee);
}

void CompanyManagerUI::connect_item_viewers() {
  connect(m_impl->gui.new_department_btn, &QPushButton::clicked, this,
          &CompanyManagerUI::new_department);
  connect(m_impl->item_viewers.department, &DepartmentView::name_changed, this,
          &CompanyManagerUI::rename_department);
  connect(m_impl->item_viewers.department, &DepartmentView::erased, this,
          &CompanyManagerUI::remove_department);
  connect(m_impl->item_viewers.department, &DepartmentView::employee_added,
          this, &CompanyManagerUI::new_employee);
  connect(m_impl->item_viewers.employee, &EmployeeView::surname_changed, this,
          &CompanyManagerUI::change_employee_surname);
  connect(m_impl->item_viewers.employee, &EmployeeView::name_changed, this,
          &CompanyManagerUI::change_employee_name);
  connect(m_impl->item_viewers.employee, &EmployeeView::middle_name_changed,
          this, &CompanyManagerUI::change_employee_middle_name);
  connect(m_impl->item_viewers.employee, &EmployeeView::function_changed, this,
          &CompanyManagerUI::change_employee_function);
  connect(m_impl->item_viewers.employee, &EmployeeView::salary_updated, this,
          &CompanyManagerUI::update_employee_salary);
  connect(m_impl->item_viewers.employee, &EmployeeView::erased, this,
          &CompanyManagerUI::remove_employee);
}

void CompanyManagerUI::reset_redo_queues() {
  m_impl->tasks.modify_xml.ResetRepeatQueue();
  m_impl->tasks.tree_model.ResetRepeatQueue();
}

void CompanyManagerUI::update_undo_redo_buttons() {
  m_impl->gui.undo_btn->setEnabled(
      static_cast<bool>(m_impl->tasks.modify_xml.CanBeCanceled()));
  m_impl->gui.redo_btn->setEnabled(
      static_cast<bool>(m_impl->tasks.modify_xml.CanBeRepeated()));
}

void CompanyManagerUI::update_redo_undo_actions_after_editing() {
  reset_redo_queues();
  update_undo_redo_buttons();
}

void CompanyManagerUI::update_viewers() {
  select_viewer(  //��������� ������ � ���� ���������
      get_current_selection_index());
}

void CompanyManagerUI::setup_viewers() {
  m_impl->tree_model.SetCompany(m_impl->company_manager);
  m_impl->gui.stacked_viewer->setCurrentWidget(
      m_impl->item_viewers.dummy);  //������������� ������ ���� ���������
  show_viewers(true);
}

void CompanyManagerUI::select_viewer(const QModelIndex& selected_item) {
  using ItemType = CompanyTreeModel::ItemType;
  auto item_type{m_impl->tree_model.GetItemType(selected_item)};
  if (!item_type ||
      *item_type ==
          ItemType::Company) {  //�� �������� �������������� ��� ���������
    m_impl->gui.stacked_viewer->setCurrentWidget(m_impl->item_viewers.dummy);
  }
  switch (*item_type) {
    case ItemType::Department: {
      update_department_viewer(selected_item);
      m_impl->gui.stacked_viewer->setCurrentWidget(
          m_impl->item_viewers.department);
    } break;
    case ItemType::Employee: {
      update_employee_viewer(selected_item);
      m_impl->gui.stacked_viewer->setCurrentWidget(
          m_impl->item_viewers.employee);
    } break;
  };
}

void CompanyManagerUI::update_department_viewer(
    const QModelIndex& selected_item) {
  m_impl->item_viewers.department->SetDepartment(DepartmentView::view_info{
      selected_item, std::get<const Department*>(
                         m_impl->tree_model.GetItemNode(selected_item))});
}

void CompanyManagerUI::update_employee_viewer(
    const QModelIndex& selected_item) {
  m_impl->item_viewers.employee->SetEmployee(EmployeeView::view_info{
      selected_item,
      std::tuple{
          *m_impl->tree_model.GetDepartmentNameRef(
              selected_item),  //��� ������ ������ - �� ����� ���������� �
                               //�������� �������������� ��� �����������
          std::get<const Employee*>(
              m_impl->tree_model.GetItemNode(selected_item))}});
}

void CompanyManagerUI::show_viewers(bool on) {
  if (on) {
    m_impl->gui.company_viewer->show();
    m_impl->gui.stacked_viewer->show();
  } else {
    m_impl->gui.company_viewer->hide();
    m_impl->gui.stacked_viewer->hide();
  }
}

void CompanyManagerUI::reset_helper() {  //����� ��� �������� ���������
  m_impl->tasks.service.Process(
      command::file_io::Reset::make_instance(m_impl->company_manager));
  m_impl->tasks.service
      .ResetQueues();  //����� ������� ������ ��� ������� ���� ������
  m_impl->tasks.modify_xml.ResetQueues();
  m_impl->tasks.tree_model.ResetQueues();
  update_undo_redo_buttons();
  m_impl->tree_model.Reset();
  m_impl->item_viewers.department->Reset();
  m_impl->item_viewers.employee->Reset();
  show_viewers(false);
}

bool CompanyManagerUI::close_helper(WarningMode no_loaded) {
  bool not_cancelled{true};

  PipelineBuilder()
      .AddHandler([this, no_loaded](bool& _) {
        if (!is_loaded()) {
          if (no_loaded == WarningMode::Show) {
            not_loaded_msg();
          }
          return false;
        }
        return true;
      })
      .AddHandler([this](bool& not_cancelled) {
        if (!is_saved()) {
          QString file_name{extract_file_name(get_current_file_path())};
          auto user_answer{save_before_close_request(file_name)};
          switch (user_answer) {
            case QMessageBox::StandardButton::Save:
              not_cancelled = save();
              break;  //� ���� ������ ���� �������� ����� ���� ��������
            case QMessageBox::StandardButton::Cancel:
              not_cancelled = false;
              break;
          }
        }
        return not_cancelled;
      })
      .AddHandler([this](bool& _) {
        reset_helper();
        return false;
      })
      .Assemble()
      ->Process(not_cancelled);
  return not_cancelled;
}

void CompanyManagerUI::closeEvent(QCloseEvent* event) {
  if (!close_helper(WarningMode::Hide)) {
    event->ignore();  //���� �� ��� ������� - ������������ ������� ��������
  } else {
    event->accept();
  }
}

QMessageBox::StandardButton CompanyManagerUI::show_message(
    message::Type type,
    QString title,
    std::optional<QString> text,
    QMessageBox::StandardButtons buttons) const {
  auto& factory{*m_impl->message_factories[static_cast<size_t>(type)]};
  factory.SetTitle(std::move(title)).SetButtons(buttons);
  if (text) {
    factory.SetText(std::move(*text));
  }
  return static_cast<QMessageBox::StandardButton>(factory.Create()->exec());
}

void CompanyManagerUI::internal_fatal_error_msg(
    QString title,
    const std::string& error_definition) const noexcept {
  QString text(u8"��������� ������������ ����\n");
  text += u8"��������� ���: ";
  text += error_definition.empty() ? u8"(�����)" : error_definition.data();
  show_message(message::Type::Critical, std::move(title), text);
}

QModelIndex CompanyManagerUI::get_current_selection_index() {
  return m_impl->gui.tree_view->selectionModel()->currentIndex();
}

QMessageBox::StandardButton CompanyManagerUI::save_before_close_request(
    const QString& file_name) {
  QString question(u8"��������� ��������� � �����");
  if (!file_name.isEmpty()) {
    question += ' ';
    question += file_name;
  }
  question += u8"?";
  return show_message(message::Type::Question, u8"������������ ���������",
                      question,
                      QMessageBox::Button::Save | QMessageBox::Button::Discard |
                          QMessageBox::Button::Cancel);
}

void CompanyManagerUI::empty_field_msg(const QString& field) const {
  QString text{u8"���� \""};
  text += field;
  text += u8"\" ������ ���� ���������";
  show_message(message::Type::Warning, u8"������������ ��������������",
               std::move(text), QMessageBox::Button::Ok);
}

void CompanyManagerUI::already_exists_msg(
    const QString& element_category) const {
  QString text(element_category);
  if (text.isEmpty()) {
    text = u8"�������";
  }
  text += u8" ��� ����������";
  show_message(message::Type::Warning, u8"������������ ��������������",
               std::move(text), QMessageBox::Button::Ok);
}

int CompanyManagerUI::empty_optional_field_msg_dlg(const QString& field) const {
  QString text(u8"��������� ��������� � ������������� ����� \"");
  text += field;
  text += "\"?";
  return show_message(message::Type::Question, u8"��������� ���������",
                      std::move(text),
                      QMessageBox::Button::Yes | QMessageBox::Button::No);
}

int CompanyManagerUI::zero_salary_msg_dlg() const {
  return show_message(message::Type::Warning, u8"��������� ���������",
                      u8"��������� ��������� � ������� ��������� ��������?",
                      QMessageBox::Button::Yes | QMessageBox::Button::No);
}

void CompanyManagerUI::not_loaded_msg() const {
  show_message(message::Type::Warning, u8"��������������",
               u8"��� �������� ����������", QMessageBox::Button::Ok);
}

void CompanyManagerUI::invalid_load_path_msg() const {
  show_message(message::Type::Warning, u8"���������� ������� ����",
               u8"��������� ������������ ���� � ����� �����",
               QMessageBox::Button::Ok);
}

void CompanyManagerUI::invalid_save_path_msg() const {
  show_message(message::Type::Warning, u8"���������� ��������� ����",
               u8"��������� ������������ ���� � ����� �����",
               QMessageBox::Button::Ok);
}

void CompanyManagerUI::unable_to_load_msg() const {
  show_message(
      message::Type::Critical, u8"���������� ������� ����",
      u8"���� ���������� ��� ������, �������� ��� ����� �������� ������",
      QMessageBox::Button::Ok);
}

void CompanyManagerUI::unable_to_save_msg() const {
  show_message(message::Type::Critical, u8"���������� ��������� ����",
               u8"���� ������� �� ������", QMessageBox::Button::Ok);
}

void CompanyManagerUI::already_saved_msg() {
  show_message(message::Type::Info, u8"���������� �� ���������",
               u8"���� �� ��������� ��������� � ���������� ����������",
               QMessageBox::Button::Ok);
}

bool CompanyManagerUI::warning_if_employee_already_exists(
    wrapper::RenameResult result) const {
  if (result == wrapper::RenameResult::IsDuplicate) {
    already_exists_msg(u8"��������� � ����� ���");
    return false;
  }
  return true;
}

bool CompanyManagerUI::warning_if_department_already_exists(
    wrapper::RenameResult result) const {
  if (result == wrapper::RenameResult::IsDuplicate) {
    already_exists_msg(u8"������������� � ����� �������������");
    return false;
  }
  return true;
}

QString CompanyManagerUI::request_load_file_path() {
  QString path;
  for (;;) {
    path = m_impl->file_handlers.dialog.getOpenFileName(
        this, QObject::tr("Open file"), "",
        QObject::tr("XML files(*.xml) ;; All files(*.*) "));
    if (path.isEmpty() ||
        is_readable(path)) {  //������ ���� -> ������������ ����� "������"
      break;
    } else {
      invalid_load_path_msg();
    }
  }
  return path;
}

QString CompanyManagerUI::request_save_file_path() {
  QString path;
  for (;;) {
    path = m_impl->file_handlers.dialog.getSaveFileName(
        this, QObject::tr("Open file"), "",
        QObject::tr("XML files(*.xml) ;; All files(*.*) "));
    if (path.isEmpty() ||
        is_writable(path)) {  //������ ���� -> ������������ ����� "������"
      break;
    } else {
      invalid_save_path_msg();
    }
  }
  return path;
}

QString CompanyManagerUI::get_current_file_path() {
  return std::any_cast<std::string_view>(
             m_impl->tasks.service
                 .Process(command::file_io::GetPath::make_instance(
                     m_impl->company_manager))
                 .value)
      .data();
}

QString CompanyManagerUI::extract_file_name(const QString& path) {
  m_impl->file_handlers.info.setFile(path);
  return m_impl->file_handlers.info.baseName();
}

bool CompanyManagerUI::is_loaded() const {
  return std::any_cast<bool>(
      m_impl->tasks.service
          .Process(command::file_io::CheckLoaded::make_instance(
              m_impl->company_manager))
          .value);
}

bool CompanyManagerUI::is_saved() const {
  return std::any_cast<bool>(
      m_impl->tasks.service
          .Process(command::file_io::CheckSaved::make_instance(
              m_impl->company_manager))
          .value);
}

void CompanyManagerUI::set_file_path(const QString& path) {
  m_impl->tasks.service.Process(command::file_io::SetPath::make_instance(
      m_impl->company_manager, path.toStdString()));
}

bool CompanyManagerUI::is_readable(const QString& path) const {
  m_impl->file_handlers.info.setFile(path);
  return m_impl->file_handlers.info.isReadable();
}

bool CompanyManagerUI::is_writable(const QString& path) const {
  m_impl->file_handlers.info.setFile(
      path);  //���� ����� �� ���������� - �� ����� ������
  return !m_impl->file_handlers.info.exists() ||
         m_impl->file_handlers.info
             .isWritable();  //����� - �������� �� ����������� ������
}

bool CompanyManagerUI::create_helper() {
  auto [_, result_type]{m_impl->tasks.service.Process(
      command::file_io::CreateDocument::make_instance(
          m_impl->company_manager))};
  if (result_type != task::ResultType::Success) {
    handle_internal_fatal_error(m_impl->tasks.service);
    return false;
  }
  return true;
}

std::optional<worker::file_operation::Result> CompanyManagerUI::load_helper() {
  auto [value, result_type]{m_impl->tasks.service.Process(
      command::file_io::Load::make_instance(m_impl->company_manager))};
  if (result_type != task::ResultType::Success) {
    return std::nullopt;  //��������� �� ������ ������� ���������� ������
  }
  return std::any_cast<worker::file_operation::Result>(value);
}

bool CompanyManagerUI::handle_load_result(
    worker::file_operation::Result result) {
  using worker::file_operation::Result;
  switch (result) {
    case Result::EmptyPath:
    case Result::FileOpenError:
      invalid_load_path_msg();
      return false;
    case Result::NoData:
    case Result::FileIOError:
      unable_to_load_msg();
      return false;
    default:
      return true;
  };
}

std::optional<worker::file_operation::Result> CompanyManagerUI::save_helper() {
  auto [value, result_type]{m_impl->tasks.service.Process(
      command::file_io::Save::make_instance(m_impl->company_manager))};
  if (result_type != task::ResultType::Success) {
    return std::nullopt;  //��������� �� ������ ������� ���������� ������
  }
  return std::any_cast<worker::file_operation::Result>(value);
}

bool CompanyManagerUI::handle_save_result(
    worker::file_operation::Result result) {
  using worker::file_operation::Result;
  switch (result) {
    case Result::EmptyPath:
    case Result::FileOpenError:
      invalid_save_path_msg();
      return false;
    case Result::NoData:
    case Result::FileIOError:
      unable_to_save_msg();
      return false;
    default:
      return true;
  };
}

std::optional<CompanyManagerUI::Company::department_view_it>
CompanyManagerUI::add_department_helper(Department&& department) {
  auto [value, result_type]{m_impl->tasks.modify_xml.Process(
      command::xml_wrapper::AddDepartment::make_instance(
          m_impl->company_manager, std::move(department)))};
  if (result_type != task::ResultType::Success) {
    handle_internal_fatal_error(m_impl->tasks.modify_xml);
    return std::nullopt;
  }
  return std::any_cast<Company::department_it>(value);
}

std::optional<CompanyManagerUI::Company::department_view_it>
CompanyManagerUI::insert_department_helper(Department&& department,
                                           wrapper::string_ref before) {
  auto [value, result_type]{m_impl->tasks.modify_xml.Process(
      command::xml_wrapper::InsertDepartment::make_instance(
          m_impl->company_manager, before, std::move(department)))};
  if (result_type != task::ResultType::Success) {
    handle_internal_fatal_error(m_impl->tasks.modify_xml);
    return std::nullopt;
  }
  return std::any_cast<Company::department_it>(value);
}

bool CompanyManagerUI::rename_department_helper(
    const DepartmentViewInfo& view_info,
    const QString& new_name) {
  bool successfully_renamed{false};
  wrapper::string_ref old_name(extract_department_name(view_info));

  auto arg_tuple{std::tie(view_info, old_name, new_name)};

  PipelineBuilder()
      .AddHandler([this, &arg_tuple](bool& successfully_renamed) {
        auto& [view_info, old_name, new_name]{arg_tuple};
        auto [value, result_type]{m_impl->tasks.modify_xml.Process(
            command::xml_wrapper::RenameDepartment::make_instance(
                m_impl->company_manager, old_name, new_name.toStdString()))};
        if (result_type != task::ResultType::Success) {
          handle_internal_fatal_error(m_impl->tasks.modify_xml);
          return false;
        }
        successfully_renamed = warning_if_department_already_exists(
            std::any_cast<wrapper::RenameResult>(value));
        return successfully_renamed;
      })
      .AddHandler([this, &arg_tuple](bool& successfully_renamed) {
        auto& [view_info, old_name, new_name]{arg_tuple};
        auto [value, result_type]{m_impl->tasks.tree_model.Process(
            command::tree_model::RenameDepartment::make_instance(
                m_impl->tree_model, view_info.index.row(), new_name))};
        if (result_type != task::ResultType::Success) {
          handle_internal_fatal_error(m_impl->tasks.tree_model);
        } else {
          successfully_renamed = std::any_cast<bool>(value);
        }
        return !successfully_renamed;
      })
      .AddHandler([&task_manager = m_impl->tasks.modify_xml](bool& _) {
        task_manager.Cancel();
        return false;
      })
      .Assemble()
      ->Process(successfully_renamed);
  return successfully_renamed;
}

bool CompanyManagerUI::rename_employee_in_tree_model(
    const EmployeeViewInfo& view_info,
    const QString& new_full_name) {
  const auto& [q_idx, items]{view_info};
  auto [new_q_idx, result_type]{m_impl->tasks.tree_model.Process(
      command::tree_model::ChangeEmployeeFullName::make_instance(
          m_impl->tree_model,
          command::tree_model::EmployeePersonalFile{
              static_cast<size_t>(
                  q_idx.parent().row()),  //������� ������������� � ����������
              static_cast<size_t>(q_idx.row())},
          new_full_name))};
  if (result_type != task::ResultType::Success) {
    handle_internal_fatal_error(m_impl->tasks.tree_model);
    return false;
  }  //��������� ������ � Viewer'e, �.�. ��������� �����������, � ������ -
     //���������!
  m_impl->item_viewers.employee->UpdateModelIndex(
      std::any_cast<QModelIndex>(new_q_idx));
  return true;
}

const CompanyManagerUI::Department* CompanyManagerUI::extract_department_ptr(
    const DepartmentViewInfo& view_info) {
  return std::get<const Department*>(view_info.items);
}

wrapper::string_ref CompanyManagerUI::extract_department_name(
    const DepartmentViewInfo& view_info) {
  return extract_department_ptr(view_info)->GetName();
}

wrapper::string_ref CompanyManagerUI::extract_department_name(
    const EmployeeViewInfo& view_info) {
  const auto& [department_name, _]{view_info.items};
  return department_name;
}

CompanyManagerUI::EmployeePersonalFile
CompanyManagerUI::extract_employee_personal_data(
    const EmployeeViewInfo& view_info) {
  const auto& [department_name, employee_it]{view_info.items};
  return {department_name, employee_it->GetFullName()};
}