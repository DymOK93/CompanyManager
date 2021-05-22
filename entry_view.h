#pragma once
#include "dialog_base.h"
#include "xml_wrappers.h"

/*GUI Forms*/
#include "ui_department_view.h"
#include "ui_employee_view.h"

#include <limits>
#include <memory>
#include <optional>
#include <tuple>

#include <QRegExp>
#include <QString>
#include <QtWidgets>

class IViewer : public QWidget {
  Q_OBJECT
 public:
  IViewer(QWidget* parent = nullptr) : QWidget(parent) {}
  virtual void UpdateView() = 0;
  ~IViewer() = default;
};

template <class... Items>
class ViewBase : public IViewer {
 public:
  struct ViewInfo {
    QModelIndex index;
    std::tuple<Items...> items;
  };

 public:
  ViewBase(QWidget* parent = nullptr) : IViewer(parent) {}
  void SetViewInfo(const ViewInfo& view_info) { m_view_info = view_info; }
  void Reset() noexcept { m_view_info.reset(); }

 protected:
  const QModelIndex& get_index() const {
    return m_view_info.value().index;  //Проверка на случай попытки обращения к
                                       //невалидному содержимому
  }
  void update_model_index(const QModelIndex& new_q_idx) {
    m_view_info.value().index = new_q_idx;
  }
  template <class Item>
  const Item& get_item() const {
    return std::get<Item>(m_view_info.value().items);
  }
  const ViewInfo& get_view() const { return m_view_info.value(); }

 private:
  std::optional<ViewInfo> m_view_info;
};

class DepartmentView : public ViewBase<const wrapper::Department*> {
  Q_OBJECT
 public:
  using MyBase = ViewBase<const wrapper::Department*>;
  using view_info = typename MyBase::ViewInfo;

 public:
  DepartmentView(QWidget* parent = nullptr);
  DepartmentView& SetDepartment(const view_info& department);
  DepartmentView& Reset() noexcept;

  void UpdateView() override;  //Обновление всех полей
  void RestoreName();          //Заново считывает имя
  void UpdateStats();  //Загружает новые данные статистики (например, после
                       //добавления работника)
 signals:
  void name_changed(const view_info&, const QString& new_name);
  void erased(const view_info&);
  void employee_added(const view_info&);
 private slots:
  void view_mode(int mode);  //Переключает режимы чтения и редактирования
  void name_changed_transmitter();
  void erased_transmitter();
  void employee_added_transmitter();

 private:
  void set_read_only_mode(bool on);
  void uncheck_enable_editing_chbox();

 private:
  std::unique_ptr<Ui::DepartmentViewGUI> m_gui{
      std::make_unique<Ui::DepartmentViewGUI>()};
};

class EmployeeView
    : public ViewBase<wrapper::string_ref,
                      const wrapper::Employee*> {  //Имя подразделения и
                                                   //указатель на сотрудника
  Q_OBJECT  //Итераторы не используются из-за их инвалидации
      public : using MyBase =
                   ViewBase<wrapper::string_ref, const wrapper::Employee*>;
  using view_info = typename MyBase::ViewInfo;

 public:
  EmployeeView(QWidget* parent = nullptr);
  EmployeeView& SetEmployee(const view_info& employee);
  EmployeeView& Reset() noexcept;
  void UpdateModelIndex(
      const QModelIndex& new_q_idx);  //Обновляет индекс элемента

  void UpdateView() override;  //Заново считывает все данные из узла Employee
  void RestoreSurname();  //Сверка отдельных полей
  void RestoreName();
  void RestoreMiddleName();
  void RestoreFunction();
  void RestoreSalary();
 signals:
  void erased(const view_info&);
  void surname_changed(const view_info&, const QString& new_surname);
  void name_changed(const view_info&, const QString& new_name);
  void middle_name_changed(const view_info&, const QString& new_middle_name);
  void function_changed(const view_info&, const QString& new_function);
  void salary_updated(const view_info&, wrapper::Employee::salary_t new_salary);
 private slots:
  void view_mode(int mode);  //Переключает режимы чтения и редактирования
  void surname_changed_transmitter();
  void name_changed_transmitter();
  void middle_name_changed_transmitter();
  void function_changed_transmitter();
  void salary_updated_transmitter();
  void erased_transmitter();

 private:
  void set_read_only_mode(bool on);
  void uncheck_enable_editing_chbox();

 private:
  std::unique_ptr<Ui::EmployeeViewGUI> m_gui{
      std::make_unique<Ui::EmployeeViewGUI>()};
  std::unique_ptr<QIntValidator> m_int_validator{
      std::make_unique<QIntValidator>(
          0,
          std::numeric_limits<int>::max())};  //Диапазон значений
};
