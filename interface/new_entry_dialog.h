#pragma once
#include "dialog_base.h"
#include "internal_workers.h"
#include "xml_wrappers.h"
#include "xml_wrappers_builders.h"

/*GUI forms*/
#include "ui_new_department_dialog.h"
#include "ui_new_employee_dialog.h"

#include <memory>
#include <utility>

#include <QString>
#include <QtWidgets>

namespace dialog {
class NewDepartment : public QDialog, public DialogBase {
  Q_OBJECT
 public:
  NewDepartment(wrapper::DepartmentBuilder& builder,
                message::Plantation& plantation,
                QDialog* parent = nullptr);
 private slots:
  void accept();  //Проверяет корретность заполнения полей
 private:
  bool set_checked_name();
  void empty_name_msg();

 private:
  wrapper::DepartmentBuilder* m_builder;
  std::unique_ptr<Ui::AddDepartmentDialogGUI> m_gui{
      std::make_unique<Ui::AddDepartmentDialogGUI>()};
};

class NewEmployee : public QDialog, public DialogBase {
  Q_OBJECT
 public:
  using PipelineBuilder = worker::ui_internal::PipelineBuilder<NewEmployee>;

 public:
  NewEmployee(wrapper::EmployeeBuilder& builder,
              message::Plantation& plantation,
              QDialog* parent = nullptr);
 private slots:
  void set_checked_data();  //Проверяет корретность заполнения полей
 private:
  void empty_required_field_msg(const QString& field)
      const;  //Только поля "Фамилия" и "Имя" обязательны к заполнению
  bool empty_optional_field_msg(
      const QString& field) const;  // true, если пользователь нажал согласился
                                    // с незаполненными полями
  bool invalid_salary_msg() const;

 private:
  wrapper::EmployeeBuilder* m_builder;
  std::unique_ptr<Ui::AddEmployeeDialogGUI> m_gui{
      std::make_unique<Ui::AddEmployeeDialogGUI>()};
};
}  // namespace dialog
