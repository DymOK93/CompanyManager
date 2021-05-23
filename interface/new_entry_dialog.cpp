#include "new_entry_dialog.h"

/*GUI forms*/
#include "ui_new_department_dialog.h"
#include "ui_new_employee_dialog.h"

namespace dialog {
NewDepartment::NewDepartment(wrapper::DepartmentBuilder& builder,
                             message::Plantation& plantation,
                             QDialog* parent)
    : QDialog(parent),
      DialogBase(plantation),
      m_builder(std::addressof(builder)),
      m_gui{std::make_unique<Ui::AddDepartmentDialogGUI>()} {
  m_gui->setupUi(this);
  connect(m_gui->button_box, &QDialogButtonBox::accepted, this,
          &NewDepartment::accept);
  connect(m_gui->button_box, &QDialogButtonBox::rejected, this,
          &QDialog::reject);
}

NewDepartment::~NewDepartment() noexcept = default;

void NewDepartment::accept() {
  if (set_checked_name()) {
    QDialog::accept();
  }
}

bool NewDepartment::set_checked_name() {
  if (m_gui->department_name_lnedit->text().isEmpty()) {
    return false;
  }
  m_builder->SetName(m_gui->department_name_lnedit->text().toStdString());
  return true;
}

void NewDepartment::empty_name_msg() {
  auto& factory{get_factory(message::Type::Warning)};
  factory.SetTitle(u8"Ошибка")
      .SetText(u8"Имя подразделения не может быть пустым")
      .Create()
      ->exec();
}

NewEmployee::NewEmployee(wrapper::EmployeeBuilder& builder,
                         message::Plantation& plantation,
                         QDialog* parent)
    : QDialog(parent),
      DialogBase(plantation),
      m_builder(std::addressof(builder)),
      m_gui{std::make_unique<Ui::AddEmployeeDialogGUI>()} {
  m_gui->setupUi(this);
  connect(m_gui->button_box, &QDialogButtonBox::accepted, this,
          &NewEmployee::set_checked_data);
  connect(m_gui->button_box, &QDialogButtonBox::rejected, this,
          &QDialog::reject);
}

NewEmployee::~NewEmployee() noexcept = default;

void NewEmployee::set_checked_data() {
  PipelineBuilder()
      .AddHandler(
          [builder = m_builder, surname_lnedit = m_gui->surname_lnedit,
           warning = &NewEmployee::empty_required_field_msg](NewEmployee& emp) {
            QString surname{surname_lnedit->text()};
            if (surname.isEmpty()) {
              std::invoke(warning, emp,
                          u8"Фамилия");  //Предупреждение о незаполненном поле
              return false;
            }
            builder->SetSurname(surname.toStdString());
            return true;
          })
      .AddHandler(
          [builder = m_builder, name_lnedit = m_gui->name_lnedit,
           warning = &NewEmployee::empty_required_field_msg](NewEmployee& emp) {
            QString name{name_lnedit->text()};
            if (name.isEmpty()) {
              std::invoke(warning, emp, u8"Имя");
              return false;
            }
            builder->SetName(name.toStdString());
            return true;
          })
      .AddHandler(
          [builder = m_builder, middle_name_lnedit = m_gui->middle_name_lnedit,
           question =
               &NewEmployee::empty_optional_field_msg](NewEmployee& emp) {
            QString middle_name{middle_name_lnedit->text()};
            if (middle_name.isEmpty()) {
              if (!std::invoke(question, emp, u8"Отчество")) {
                return false;  //Операция будет отменена только в случае нажатия
                               //"Отмена" пользователем
              }
            }
            builder->SetMiddleName(middle_name.toStdString());
            return true;
          })
      .AddHandler(
          [builder = m_builder, function_lnedit = m_gui->function_lnedit,
           question =
               &NewEmployee::empty_optional_field_msg](NewEmployee& emp) {
            QString function{function_lnedit->text()};
            if (function.isEmpty()) {
              if (!std::invoke(question, emp, u8"Должность")) {
                return false;
              }
            }
            builder->SetFunction(function.toStdString());
            return true;
          })
      .AddHandler(
          [builder = m_builder, salary_spbox = m_gui->salary_spbox,
           question = &NewEmployee::invalid_salary_msg](NewEmployee& emp) {
            int salary{salary_spbox->value()};
            if (salary == 0) {
              if (!std::invoke(question, emp)) {
                return false;
              }
            }
            builder->SetSalary(static_cast<size_t>(salary));
            return true;
          })
      .AddHandler([accept = &QDialog::accept](NewEmployee& emp) {
        std::invoke(accept, emp);  //Закрытие окна
        return false;
      })
      .Assemble()
      ->Process(*this);
}

void NewEmployee::empty_required_field_msg(const QString& field) const {
  auto& factory{get_factory(message::Type::Warning)};
  QString text(u8"Поле \"");
  text += field;
  text += u8" \" не может быть пустым";
  factory.SetTitle(u8"Ошибка")
      .SetText(std::move(text))  //Экранирование кавычек, чтобы не использовать
                                 //сырой литерал R("")
      .Create()
      ->exec();
}

bool NewEmployee::empty_optional_field_msg(
    const QString& field) const {  //Запрос пользовательского действия
  auto& factory{get_factory(message::Type::Warning)};
  QString text(u8"Поле \"");
  text += field;
  text += u8" \" не заполнено. Продолжить?";
  factory.SetTitle(u8"Предупреждение")
      .SetText(std::move(text))
      .SetButtons(QMessageBox::StandardButton::Ok |
                  QMessageBox::StandardButton::Cancel);
  return factory.Create()->exec() == QMessageBox::StandardButton::Ok;
}

bool NewEmployee::invalid_salary_msg() const {
  auto& factory{get_factory(message::Type::Warning)};
  factory.SetTitle(u8"Предупреждение")
      .SetText(u8"Указана нулевая заработная плата. Вы уверены?")
      .SetButtons(QMessageBox::StandardButton::Ok |
                  QMessageBox::StandardButton::Cancel);
  return factory.Create()->exec() == QMessageBox::StandardButton::Ok;
}
}  // namespace dialog