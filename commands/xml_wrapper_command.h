#pragma once
#include "command_interface.h"
#include "company_manager_engine.h"

#include <utility>
#include <variant>
#include <string_view>
#include <type_traits>

namespace command {
	namespace xml_wrapper {
		template <class ConcreteCommand>
		class ModifyCommand : public AllocatedCommand<ConcreteCommand, CompanyManager> {
			using MyBase = AllocatedCommand<ConcreteCommand, CompanyManager>;
		public:
			using MyBase::MyBase;
		protected:
			wrapper::Company* get_tree_ptr() {									//ѕри отсутствии открытого документа Modify() выбросит исключение
				return std::addressof(MyBase::get_target().Modify());
			}
			wrapper::Company& get_tree_ref() {
				return MyBase::get_target().Modify();
			}
		};

		class RenameDepartment : public ModifyCommand<RenameDepartment> {
		public:
			using MyBase = ModifyCommand<RenameDepartment>;
			using command_holder = MyBase::command_holder;
		public:
			RenameDepartment(
				CompanyManager& cm,
				wrapper::string_ref current_name,
				std::string new_name
			) noexcept;
			std::any Execute() override;									//return type: wrapper::RenameResult
			std::any Cancel() override;										//return type: wrapper::RenameResult
			Type GetType() const noexcept override;
			static command_holder make_instance(
				CompanyManager& cm,
				wrapper::string_ref current_name,
				std::string new_name
			);
		protected:
			wrapper::string_ref m_current_name;
			std::string m_new_name;
		};

		template <class ConcreteCommand, class WrapperTy, class SubstituteTy>
		class ModifyCompany : public ModifyCommand<ConcreteCommand> {	
		public:
			using MyBase = ModifyCommand<ConcreteCommand>;
			using command_holder = typename MyBase::command_holder;
		public:
			ModifyCompany(CompanyManager& cm, const SubstituteTy& substitute)		//SubstituteTy должен дЄшево копироватьс€	
				noexcept(std::is_nothrow_copy_constructible_v<SubstituteTy>)
				: MyBase(cm), m_value(substitute) 
			{
			}
			ModifyCompany(CompanyManager& cm, WrapperTy&& value) noexcept		    //Ёкономи€ вызова move c-tor
				: MyBase(cm), m_value(std::move(value))
			{
			}
		protected:
			WrapperTy& get_wrapper() {
                return std::get<WrapperTy>(m_value);
			}
			SubstituteTy& get_substitute() {
                return std::get<SubstituteTy>(m_value);
			}
		protected:
			std::variant<
				WrapperTy,
				SubstituteTy
			> m_value;
		};

		class AddDepartment : public ModifyCompany<AddDepartment, wrapper::Department, wrapper::string_ref> {
		public:																			
			using MyBase = ModifyCompany<AddDepartment, wrapper::Department, wrapper::string_ref>;  //’ранить итератор нельз€, т.к. он может инвалидироватьс€
			using command_holder = MyBase::command_holder;										//при последовательности операций "вставка-удаление-<отмена>"	
		public:
			AddDepartment(CompanyManager& cm, wrapper::Department department) noexcept;
			std::any Execute() override;										//return type: wrapper::Company::department_it	
			std::any Cancel() override;											//return type: empty std::any
			Type GetType() const noexcept override;
			static command_holder make_instance(CompanyManager& cm, wrapper::Department department);
		};

		//»з-за ObjectPool'a неследоватьс€ напр€мую от AddDepartment нельз€
		class InsertDepartment : public ModifyCompany<InsertDepartment, wrapper::Department, wrapper::string_ref> {
		public:
			using MyBase = ModifyCompany<InsertDepartment, wrapper::Department, wrapper::string_ref>;
			using command_holder = MyBase::command_holder;
		public:
			InsertDepartment(
				CompanyManager& cm,
				wrapper::string_ref before,
				wrapper::Department department
			) noexcept;
			std::any Execute() override;										//return type: wrapper::Company::department_it	
			std::any Cancel() override;											//return type: empty std::any	
			Type GetType() const noexcept override;
			static command_holder make_instance(
				CompanyManager& cm,
				wrapper::string_ref before,
				wrapper::Department department
			);
		protected:
			wrapper::string_ref m_before;
		};
	
		class RemoveDepartment : public ModifyCompany<RemoveDepartment, wrapper::Department, wrapper::string_ref> {																										
		public:																								
			using MyBase = ModifyCompany<RemoveDepartment, wrapper::Department, wrapper::string_ref>;
			using command_holder = MyBase::command_holder;
			using department_it = wrapper::Company::department_it;
		public:
			RemoveDepartment(
				CompanyManager& cm,
				wrapper::string_ref department
			) noexcept;
			std::any Execute() override;										//return type: empty std::any	
			std::any Cancel() override;											//return type: wrapper::Company::department_it	
			Type GetType() const noexcept override;
			static command_holder make_instance(
				CompanyManager& cm,
				wrapper::string_ref department
			);
		protected:
			std::optional<wrapper::string_ref> m_before;
		};

		/************************************************************************************************************************
		ѕочему можно в EmployeePersonalFile хранить FullNameRef, не опаса€сь инвалидации?
		ѕол€ FullNameRef ссылаютс€ на дочерние узлы XML-узла типа Employee.  онтейнер дочерних узлов - xml::container_t = 
		перемещаетс€ без перемещени€ элементов, кроме того, он содержит не сами узлы, а node_holder'ы.
		ѕоскольку все операции вставки и удалени€ извлекают данные из хранилищ, а не удал€ют непосредственно, 
		инвалидируютс€ лишь итераторы, а все текстовые дочерние узлы XML-Employee (surname, name, middle_name, function, salary) 
		продолжат быть доступны.
		*************************************************************************************************************************/

		struct EmployeePersonalFile {
			wrapper::string_ref department_name;
			wrapper::FullNameRef employee_name;
		};

		template <class ConcreteCommand, class SwappedFieldTy>
		class ModifyEmployeeFields : public ModifyCommand<ConcreteCommand> {
		public:
			using MyBase = ModifyCommand<ConcreteCommand>;
			using command_holder = typename MyBase::command_holder;
		public:
			ModifyEmployeeFields(															
				CompanyManager& cm,
				const EmployeePersonalFile& employee,
				SwappedFieldTy value
			) noexcept (std::is_nothrow_move_constructible_v<SwappedFieldTy>)
				: MyBase(cm), m_employee(employee), m_value(std::move(value))
			{
			}
			std::any Cancel() override {
				return static_cast<ConcreteCommand*>(this)->Execute();						//"data swap"
			}
            static command_holder make_instance(
                CompanyManager& cm,
                const EmployeePersonalFile& employee,
                SwappedFieldTy value
            ) {
                return MyBase::allocate_instance(cm, employee, std::move(value));
            }
		protected:
			SwappedFieldTy& get_value() noexcept {
				return m_value;
			}
			const wrapper::FullNameRef& get_full_name()noexcept {
				return m_employee.employee_name;
			}
			wrapper::Department& get_department() {
				return MyBase::get_tree_ref().at(m_employee.department_name);
			}
			wrapper::Employee& get_employee() {
				return get_department().at(get_full_name());
			}	
		protected:
			EmployeePersonalFile m_employee;
			SwappedFieldTy m_value;
		};

		template <class ConcreteCommand>
		class ModifyEmployeeTextFields : public ModifyEmployeeFields<ConcreteCommand, std::string> {
		public:
			using MyBase = ModifyEmployeeFields<ConcreteCommand, std::string> ;
			using command_holder = typename MyBase::command_holder;
		public:
			using MyBase::MyBase;
		};

		class ChangeEmployeeSurname : public ModifyEmployeeTextFields<ChangeEmployeeSurname> {
		public:
			using MyBase = ModifyEmployeeTextFields<ChangeEmployeeSurname>;
			using command_holder = MyBase::command_holder;
		public:
			using MyBase::MyBase;
			std::any Execute() override;										//return type: wrapper::RenameResult
            Type GetType() const noexcept override;
		};

		class ChangeEmployeeName : public ModifyEmployeeTextFields<ChangeEmployeeName> {
		public:
			using MyBase = ModifyEmployeeTextFields<ChangeEmployeeName>;
			using command_holder = MyBase::command_holder;
		public:
			using MyBase::MyBase;
			std::any Execute() override;										//return type: empty std::any
            Type GetType() const noexcept override;
		};

		class ChangeEmployeeMiddleName : public ModifyEmployeeTextFields<ChangeEmployeeMiddleName> {
		public:
			using MyBase = ModifyEmployeeTextFields<ChangeEmployeeMiddleName>;
			using command_holder = MyBase::command_holder;
		public:
			using MyBase::MyBase;	
			std::any Execute() override;										//return type: empty std::any
            Type GetType() const noexcept override;
		};

		class ChangeEmployeeFunction : public ModifyEmployeeTextFields<ChangeEmployeeFunction> {
		public:
			using MyBase = ModifyEmployeeTextFields<ChangeEmployeeFunction>;
			using command_holder = MyBase::command_holder;
		public:
			using MyBase::MyBase;
			std::any Execute() override;										//return type: empty std::any
            Type GetType() const noexcept override;
		};

		class UpdateEmployeeSalary : public ModifyEmployeeFields<UpdateEmployeeSalary, wrapper::Employee::salary_t> {
		public:
			using MyBase = ModifyEmployeeFields<UpdateEmployeeSalary, wrapper::Employee::salary_t>;
			using command_holder = MyBase::command_holder;
			using salary_t = wrapper::Employee::salary_t;
		public:
			using MyBase::MyBase;
			std::any Execute() override;										//return type: salary_t (salary before execute)
            Type GetType() const noexcept override;
		};

		template <class ConcreteCommand>
		class ModifyDepartment : public ModifyCompany<ConcreteCommand, wrapper::Employee, wrapper::FullNameRef> {
		public:
			using MyBase = ModifyCompany<ConcreteCommand, wrapper::Employee, wrapper::FullNameRef>;
			using command_holder = typename MyBase::command_holder;
		public:
			ModifyDepartment(
				CompanyManager& cm,
				wrapper::string_ref department,
				wrapper::Employee employee
			) noexcept : MyBase(cm, std::move(employee)),
				m_department(department)
			{
			}
			ModifyDepartment(
				CompanyManager& cm,
				wrapper::string_ref department,
				const wrapper::FullNameRef& full_name
			) noexcept : MyBase(cm, full_name),
				m_department(department)
			{
			}
		protected:
			wrapper::Department& get_department() {
				return MyBase::get_tree_ref().at(m_department);
			}
		protected:
			wrapper::string_ref m_department;
		};

		class InsertEmployee : public ModifyDepartment<InsertEmployee> {		
		public:
			using MyBase = ModifyDepartment<InsertEmployee>;				
			using command_holder = MyBase::command_holder;
		public:
			InsertEmployee(
				CompanyManager& cm,
				wrapper::string_ref department,
				wrapper::Employee employee
			) noexcept;
			std::any Execute() override;										//return type: wrapper::Department::employee_it
			std::any Cancel() override;											//return type: empty std::any
			Type GetType() const noexcept override;
			static command_holder make_instance(
				CompanyManager& cm,
				wrapper::string_ref department,
				wrapper::Employee employee
			);
		};

		class RemoveEmployee : public ModifyDepartment<RemoveEmployee> {		//¬ыполн€ет действи€, полность обратные операци€м в InsertEmployee
		public:
			using MyBase = ModifyDepartment<RemoveEmployee>;
			using command_holder = MyBase::command_holder;
		public:
			RemoveEmployee(
				CompanyManager& cm,
				const EmployeePersonalFile& employee
			);
			std::any Execute() override;										//return type: empty std::anyt
			std::any Cancel() override;											//return type: wrapper::Department::employee_i
			Type GetType() const noexcept override;
			static command_holder make_instance(
				CompanyManager& cm,
				const EmployeePersonalFile& employee
			);
		};
	}
}
