#include "xml_wrapper_command.h"
using namespace std;

namespace command {
namespace xml_wrapper {

RenameDepartment::RenameDepartment(CompanyManager& cm,
                                   wrapper::string_ref current_name,
                                   string new_name) noexcept
    : MyBase(cm), m_current_name(current_name), m_new_name(move(new_name)) {}

any RenameDepartment::Execute() {
  string old_name(m_current_name);
  auto result{
      get_tree_ref().RenameDepartment(m_current_name, move(m_new_name))};
  m_new_name = move(old_name);
  return result;
}

any RenameDepartment::Cancel() {
  return Execute();  //"data swap"
}

Type RenameDepartment::GetType() const noexcept {
  return Type::Xml_RenameDepartment;
}

RenameDepartment::command_holder RenameDepartment::make_instance(
    CompanyManager& cm,
    wrapper::string_ref current_name,
    string new_name) {
  return allocate_instance(cm, current_name, move(new_name));
}
AddDepartment::AddDepartment(CompanyManager& cm,
                             wrapper::Department&& department) noexcept
    : MyBase(cm, move(department)) {}

any AddDepartment::Execute() {
  auto it{get_tree_ref().AddDepartment(move(get_wrapper()))};
  m_value.emplace<wrapper::string_ref>(
      it->GetName());  //—охран€ем название подразделени€
  return it;
}

any AddDepartment::Cancel() {
  m_value = get_tree_ref().ExtractDepartment(get_substitute());
  return make_default_value();
}

Type AddDepartment::GetType() const noexcept {
  return Type::Xml_AddDepartment;
}

AddDepartment::command_holder AddDepartment::make_instance(
    CompanyManager& cm,
    wrapper::Department&& department) {
  return allocate_instance(cm, move(department));
}

InsertDepartment::InsertDepartment(CompanyManager& cm,
                                   wrapper::string_ref before,
                                   wrapper::Department&& department) noexcept
    : MyBase(cm, move(department)), m_before(before) {}

any InsertDepartment::Execute() {
  auto it{get_tree_ref().InsertDepartment(m_before, move(get_wrapper()))};
  m_value.emplace<wrapper::string_ref>(
      it->GetName());  //—охран€ем название подразделени€
  return it;
}

any InsertDepartment::Cancel() {
  m_value = get_tree_ref().ExtractDepartment(get_substitute());
  return make_default_value();
}

Type InsertDepartment::GetType() const noexcept {
  return Type::Xml_InsertDepartment;
}

InsertDepartment::command_holder InsertDepartment::make_instance(
    CompanyManager& cm,
    wrapper::string_ref before,
    wrapper::Department&& department) {
  return allocate_instance(cm, before, move(department));
}

RemoveDepartment::RemoveDepartment(CompanyManager& cm,
                                   wrapper::string_ref department) noexcept
    : MyBase(cm, department) {}

any RemoveDepartment::Execute() {
  auto& tree{get_tree_ref()};
  const auto* next{//”казатель на следующее по списку подразделение
                   tree.TryGetNext(get_substitute())};
  if (next) {
    m_before = next->GetName();  //≈сли удал€емое - не последнее, сохран€ем им€
                                 //следующего
  }
  m_value = get_tree_ref().ExtractDepartment(get_substitute());
  return make_default_value();
}

any RemoveDepartment::Cancel() {
  auto& tree{get_tree_ref()};
  department_it it;
  if (m_before) {
    it = tree.InsertDepartment(*m_before, move(get_wrapper()));
  } else {
    it = tree.AddDepartment(
        move(get_wrapper()));  //—охран€ем наименование подразделени€
  }
  m_value = it->GetName();
  return it;
}

Type RemoveDepartment::GetType() const noexcept {
  return Type::Xml_RemoveDepartment;
}

RemoveDepartment::command_holder RemoveDepartment::make_instance(
    CompanyManager& cm,
    wrapper::string_ref department) {
  return allocate_instance(cm, department);
}

any ChangeEmployeeSurname::Execute() {
  auto& employee{get_employee()};
  string old_surname(employee.GetSurname());
  wrapper::RenameResult result{
      get_department().ChangeEmployeeSurname(get_full_name(), move(m_value))};
  m_value = move(old_surname);
  return result;
}

Type ChangeEmployeeSurname::GetType() const noexcept {
  return Type::Xml_ChangeEmployeeSurname;
}

any ChangeEmployeeName::Execute() {
  auto& employee{get_employee()};
  string old_name(employee.GetName());
  wrapper::RenameResult result{
      get_department().ChangeEmployeeName(get_full_name(), move(m_value))};
  m_value = move(old_name);
  return result;
}

Type ChangeEmployeeName::GetType() const noexcept {
  return Type::Xml_ChangeEmployeeName;
}

any ChangeEmployeeMiddleName::Execute() {
  auto& employee{get_employee()};
  string old_middle_name(employee.GetMiddleName());
  wrapper::RenameResult result{get_department().ChangeEmployeeMiddleName(
      get_full_name(), move(m_value))};
  m_value = move(old_middle_name);
  return result;
}

Type ChangeEmployeeMiddleName::GetType() const noexcept {
  return Type::Xml_ChangeEmployeeMiddleName;
}

any ChangeEmployeeFunction::Execute() {
  auto& employee{get_employee()};
  string old_function(employee.GetFunction());
  employee.SetFunction(move(m_value));
  m_value = move(old_function);
  return make_default_value();
}

Type ChangeEmployeeFunction::GetType() const noexcept {
  return Type::Xml_ChangeEmployeeFunction;
}

any UpdateEmployeeSalary::Execute() {
  salary_t previous_value{get_employee().GetSalary()};
  get_department().UpdateEmployeeSalary(get_full_name(), get_value());
  m_value = previous_value;
  return previous_value;
}

Type UpdateEmployeeSalary::GetType() const noexcept {
  return Type::Xml_UpdateEmployeeSalary;
}

InsertEmployee::InsertEmployee(CompanyManager& cm,
                               wrapper::string_ref department,
                               wrapper::Employee&& employee) noexcept
    : MyBase(cm, department, std::move(employee)) {}

any InsertEmployee::Execute() {
  auto it{get_department().InsertEmployee(std::move(get_wrapper()))};
  m_value = it->first;  // FullNameRef
  return it;
}

any InsertEmployee::Cancel() {
  m_value = get_department().ExtractEmployee(get_substitute());
  return make_default_value();
}

Type InsertEmployee::GetType() const noexcept {
  return Type::Xml_InsertEmployee;
}

InsertEmployee::command_holder InsertEmployee::make_instance(
    CompanyManager& cm,
    wrapper::string_ref department,
    wrapper::Employee&& employee) {
  return allocate_instance(cm, department, move(employee));
}

RemoveEmployee::RemoveEmployee(CompanyManager& cm,
                               const EmployeePersonalFile& employee)
    : MyBase(cm, employee.department_name, employee.employee_name) {}

any RemoveEmployee::Execute() {
  m_value = get_department().ExtractEmployee(get_substitute());
  return make_default_value();
}

any RemoveEmployee::Cancel() {
  auto it{get_department().InsertEmployee(std::move(get_wrapper()))};
  m_value = it->first;  // FullNameRef
  return it;
}

Type RemoveEmployee::GetType() const noexcept {
  return Type::Xml_RemoveEmployee;
}

RemoveEmployee::command_holder RemoveEmployee::make_instance(
    CompanyManager& cm,
    const EmployeePersonalFile& employee) {
  return allocate_instance(cm, employee);
}
}  // namespace xml_wrapper
}  // namespace command
