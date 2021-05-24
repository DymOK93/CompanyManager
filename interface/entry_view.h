#pragma once
#include "dialog_base.h"
#include "xml_wrappers.h"

#include <limits>
#include <memory>
#include <optional>
#include <tuple>

#include <QRegExp>
#include <QString>
#include <QtWidgets>

namespace Ui {
class DepartmentViewGUI;
class EmployeeViewGUI;
}  // namespace Ui

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
    return m_view_info.value().index;  //�������� �� ������ ������� ��������� �
                                       //����������� �����������
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
  DepartmentView(const DepartmentView&) = delete;
  DepartmentView(DepartmentView&&) noexcept = default;
  DepartmentView& operator=(const DepartmentView&) = delete;
  DepartmentView& operator=(DepartmentView&&) noexcept = default;
  ~DepartmentView() noexcept;

  DepartmentView& SetDepartment(const view_info& department);
  DepartmentView& Reset() noexcept;

  void UpdateView() override;  //���������� ���� �����
  void RestoreName();          //������ ��������� ���
  void UpdateStats();  //��������� ����� ������ ���������� (��������, �����
                       //���������� ���������)
 signals:
  void name_changed(const view_info&, const QString& new_name);
  void erased(const view_info&);
  void employee_added(const view_info&);
 private slots:
  void view_mode(int mode);  //����������� ������ ������ � ��������������
  void name_changed_transmitter();
  void erased_transmitter();
  void employee_added_transmitter();

 private:
  void set_read_only_mode(bool on);
  void uncheck_enable_editing_chbox();

 private:
  std::unique_ptr<Ui::DepartmentViewGUI> m_gui;
};

class EmployeeView
    : public ViewBase<wrapper::string_ref,
                      const wrapper::Employee*> {  //��� ������������� �
                                                   //��������� �� ����������
  Q_OBJECT  //��������� �� ������������ ��-�� �� �����������
      public : using MyBase =
                   ViewBase<wrapper::string_ref, const wrapper::Employee*>;
  using view_info = typename MyBase::ViewInfo;

 private:
  struct Impl;

 public:
  EmployeeView(QWidget* parent = nullptr);
  EmployeeView(const EmployeeView&) = delete;
  EmployeeView(EmployeeView&&) noexcept = default;
  EmployeeView& operator=(const EmployeeView&) = delete;
  EmployeeView& operator=(EmployeeView&&) noexcept = default;
  ~EmployeeView() noexcept;

  EmployeeView& SetEmployee(const view_info& employee);
  EmployeeView& Reset() noexcept;
  void UpdateModelIndex(
      const QModelIndex& new_q_idx);  //��������� ������ ��������

  void UpdateView() override;  //������ ��������� ��� ������ �� ���� Employee
  void RestoreSurname();  //������ ��������� �����
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
  void view_mode(int mode);  //����������� ������ ������ � ��������������
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
  std::unique_ptr<Impl> m_impl;
};
