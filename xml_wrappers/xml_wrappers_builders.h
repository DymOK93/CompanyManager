#pragma once
#include "xml_wrappers.h"
#include "builder_base.h"

namespace wrapper {
	struct PersonalData {
		std::string
			surname,
			name,
			middle_name,
			function;
		size_t salary{ 0 };
	};

	class EmployeeBuilder : public xml::BuilderBase<EmployeeBuilder> {
	public:
		using MyBase = xml::BuilderBase<EmployeeBuilder>;
	public:
		EmployeeBuilder& SetSurname(std::string surname) noexcept;
		EmployeeBuilder& SetName(std::string name) noexcept;
		EmployeeBuilder& SetMiddleName(std::string middle_name) noexcept;
		EmployeeBuilder& SetFunction(std::string function) noexcept;
		EmployeeBuilder& SetSalary(size_t salary) noexcept;
		EmployeeBuilder& Reset() noexcept;

		Employee Assemble();
	protected:
		PersonalData m_personal_data;
	};

	class DepartmentBuilder : public xml::BuilderBase<DepartmentBuilder> {
	public:
		using MyBase = xml::BuilderBase<DepartmentBuilder>;
	public:
		DepartmentBuilder& SetName(std::string name) noexcept;
		DepartmentBuilder& InsertEmployee(Employee employee);
		DepartmentBuilder& InsertEmployees(std::vector<Employee> employees);
		DepartmentBuilder& Reset() noexcept;

		Department Assemble();
	protected:
		std::string m_name;
		Department::workgroup_t m_workgroup;
	};

	class CompanyBuilder : public xml::BuilderBase<CompanyBuilder> {
	public:
		using MyBase = xml::BuilderBase<CompanyBuilder>;
	public:
		CompanyBuilder& AddDepartment(Department department);
		CompanyBuilder& Reset() noexcept;

		Company Assemble();
	protected:
		Company::subdivision_t m_subdivision;
	};
}