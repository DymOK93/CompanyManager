#pragma once
#include "dialog_base.h"
#include "internal_workers.h"
#include "xml_wrappers.h"
#include "xml_wrappers_builders.h"

#include <memory>
#include <utility>

#include <QString>
#include <QtWidgets>

namespace Ui {
class AddDepartmentDialogGUI;
class AddEmployeeDialogGUI;
}

namespace dialog {
class NewDepartment : public QDialog, public DialogBase {
  Q_OBJECT
 public:
  NewDepartment(wrapper::DepartmentBuilder& builder,
                message::Plantation& plantation,
                QDialog* parent = nullptr);
  NewDepartment(const NewDepartment&) = delete;
  NewDepartment(NewDepartment&&) noexcept = default;
  NewDepartment& operator=(const NewDepartment&) = delete;
  NewDepartment& operator=(NewDepartment&&) noexcept = default;
  ~NewDepartment() noexcept;
 private slots:
  void accept();  //Проверяет корретность заполнения полей
 private:
  bool set_checked_name();
  void empty_name_msg();

 private:
  wrapper::DepartmentBuilder* m_builder;
  std::unique_ptr<Ui::AddDepartmentDialogGUI> m_gui;
};

class NewEmployee : public QDialog, public DialogBase {
  Q_OBJECT
 public:
  using PipelineBuilder = worker::ui_internal::PipelineBuilder<NewEmployee>;

 public:
  NewEmployee(wrapper::EmployeeBuilder& builder,
              message::Plantation& plantation,
              QDialog* parent = nullptr);
  NewEmployee(const NewEmployee&) = delete;
  NewEmployee(NewEmployee&&) noexcept = default;
  NewEmployee& operator=(const NewEmployee&) = delete;
  NewEmployee& operator=(NewEmployee&&) noexcept = default;
  ~NewEmployee() noexcept;
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
  std::unique_ptr<Ui::AddEmployeeDialogGUI> m_gui;
};
}  // namespace dialog
