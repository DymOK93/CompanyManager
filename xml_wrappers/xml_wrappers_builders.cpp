#include "xml_wrappers_builders.h"
using xml::Node;
using namespace std;

namespace wrapper {
	EmployeeBuilder& EmployeeBuilder::SetSurname(std::string surname) noexcept {
		m_personal_data.surname = move(surname);
		return *this;
	}

	EmployeeBuilder& EmployeeBuilder::SetName(std::string name) noexcept {
		m_personal_data.name = move(name);
		return *this;
	}

	EmployeeBuilder& EmployeeBuilder::SetMiddleName(std::string middle_name) noexcept {
		m_personal_data.middle_name = move(middle_name);
		return *this;
	}

	EmployeeBuilder& EmployeeBuilder::SetFunction(string function) noexcept {
		m_personal_data.function = move(function);
		return *this;
	}

	EmployeeBuilder& EmployeeBuilder::SetSalary(size_t salary) noexcept {
		m_personal_data.salary = salary;
		return *this;
	}

	EmployeeBuilder& EmployeeBuilder::Reset() noexcept {
		m_personal_data = {};
		return MyBase::Reset();
	}

	Employee EmployeeBuilder::Assemble() {
		xml::ElementNodeBuilder element_builder;
		element_builder.SetAllocator(take_allocator());

		return xml::DocumentNodeBuilder()
			.SetName("employment")
			.SetAllocator(take_allocator())
			.SetChildren(
				element_builder.SetName("surname").SetText(move(m_personal_data.surname)).Assemble(),
				element_builder.SetName("name").SetText(move(m_personal_data.name)).Assemble(),
				element_builder.SetName("middleName").SetText(move(m_personal_data.middle_name)).Assemble(),
				element_builder.SetName("function").SetText(move(m_personal_data.function)).Assemble(),
				element_builder.SetName("salary").SetText(to_string(m_personal_data.salary)).Assemble()
			)
			.Assemble();
	}

	DepartmentBuilder& DepartmentBuilder::SetName(string new_name) noexcept {
		m_name = move(new_name);
		return *this;
	}

	DepartmentBuilder& DepartmentBuilder::InsertEmployee(Employee&& employee) {
		m_workgroup.emplace(employee.GetFullName(), move(employee));
		return *this;
	}

	DepartmentBuilder& DepartmentBuilder::InsertEmployees(vector<Employee>&& employees) {
		for (auto& employee : employees) {
			m_workgroup.emplace(employee.GetFullName(), move(employee));
		}
		return *this;
	}

	DepartmentBuilder& DepartmentBuilder::Reset() noexcept {
		m_name.clear();
		m_workgroup.clear();
		return MyBase::Reset();
	}

	Department DepartmentBuilder::Assemble() {	
		xml::allocator_holder alloc{ take_allocator() };
		Department department{
			xml::DocumentNodeBuilder()
					.SetName("department")
					.SetAttribute("name", move(m_name))
					.SetAllocator(alloc)
					.SetChildren(
							xml::DocumentNodeBuilder()
								.SetName("employments")
								.SetAllocator(alloc)
								.SetChildren()
								.Assemble()
					)
					.Assemble()
		};
		department.SetWorkgroup(move(m_workgroup));
		return department;
	}

	CompanyBuilder& CompanyBuilder::AddDepartment(Department&& department) {
		m_subdivision.push_back(move(department));
		return *this;
	}

	CompanyBuilder& CompanyBuilder::Reset() noexcept{
		m_subdivision.clear();
		return *this;
	}

	Company CompanyBuilder::Assemble() {
		Company company{
				xml::DocumentNodeBuilder()
					.SetName("departments")
					.SetAllocator(take_allocator())
					.SetChildren()
					.Assemble()
		};
		company.SetSubdivision(move(m_subdivision));
		return company;
	}
}