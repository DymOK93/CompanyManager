#pragma once
#include "command_interface.h"
#include "tree_item.h"
#include "tree_model.h"
#include "xml_wrappers.h"
#include "company_manager_engine.h"

#include <utility>
#include <variant>
#include <string>
#include <string_view>

namespace command {
	namespace tree_model {
		template <class ConcreteCommand>
		class ModifyCommand : public AllocatedCommand<ConcreteCommand, CompanyTreeModel> {
		public:
			using MyBase = AllocatedCommand<ConcreteCommand, CompanyTreeModel>;
			using command_holder = typename MyBase::command_holder;
		public:
			using MyBase::MyBase;
		};

		template <class ConcreteCommand>
		class ModifyCompany : public ModifyCommand<ConcreteCommand> {
		public:
			using MyBase = ModifyCommand<ConcreteCommand>;
			using command_holder = typename MyBase::command_holder;	
			using ItemType = CompanyTreeModel::ItemType;
		public:
			ModifyCompany(
				CompanyTreeModel& ctm,
				const CompanyManager& company_manager,
				wrapper::string_ref department_name,
				size_t department_pos
			)	noexcept : MyBase(ctm), 
				m_department_name(std::move(department_name)), 
				m_company_manager(std::addressof(company_manager)),
				m_department_pos(department_pos)
			{
			}
		protected:
			wrapper::string_ref get_department_name() const noexcept {
				return m_department_name;
			}
			const wrapper::Company& get_company() const noexcept {
				return m_company_manager->Read();
			}
			size_t get_department_position() const noexcept {
				return m_department_pos;
			}
		protected:
			wrapper::string_ref m_department_name;
			const CompanyManager* m_company_manager;
			size_t m_department_pos{ 0 };
		};

		class InsertDepartment : public ModifyCompany<InsertDepartment>{
		public:
			using MyBase = ModifyCompany<InsertDepartment>;
			using command_holder = MyBase::command_holder;
		public:
			InsertDepartment(
				CompanyTreeModel& ctm, 
				const CompanyManager& company_manager,
				wrapper::string_ref department_name,							//Исходная строка живёт в m_attributes XML-узла
				size_t pos
			) noexcept;
			std::any Execute() override;										//return type: bool
			std::any Cancel() override;											//return type: bool
			Type GetType() const noexcept override;
			static command_holder make_instance(
				CompanyTreeModel& ctm,
				const CompanyManager& company_manager,
				wrapper::string_ref department_name,
				size_t pos
			);
		};

		class RenameDepartment : public ModifyCommand<RenameDepartment> {
		public:
			using MyBase = ModifyCommand<RenameDepartment>;
			using command_holder = MyBase::command_holder;
		public:
			RenameDepartment(
				CompanyTreeModel& ctm,
				size_t pos,														//QModelIndex могут инвалидироваться
				QString new_name												//Т.к. команда обращается только к модели, можно использовать QString
			);
			std::any Execute() override;										//return type: bool
			std::any Cancel() override;											//return type: bool
			Type GetType() const noexcept override;
			static command_holder make_instance(
				CompanyTreeModel& ctm,
				size_t pos,
				QString new_name				
			);
		private:
			QString m_name;
			size_t m_pos{ 0 };
		};

		class RemoveDepartment : public ModifyCompany<RemoveDepartment> {
		public:
			using MyBase = ModifyCompany<RemoveDepartment>;
			using command_holder = MyBase::command_holder;
		public:
			RemoveDepartment(
				CompanyTreeModel& ctm,
				const CompanyManager& company_manager,
				wrapper::string_ref department_name,							//Для обновления указателя на подразделение
				size_t pos
			) noexcept;
			std::any Execute() override;										//return type: empty std::any
			std::any Cancel() override;											//return type: bool
			Type GetType() const noexcept override;
			static command_holder make_instance(
				CompanyTreeModel& ctm,
				const CompanyManager& company_manager,
				wrapper::string_ref department_name,
				size_t pos
			);
		private:
			std::optional<CompanyTreeModel::Memento> m_branch;
		};

		struct EmployeePersonalFile {
			size_t 
				department_pos{ 0 }, 
				employee_pos{ 0 };
		};

		template <class ConcreteCommand>
		class ModifyDepartment : public ModifyCommand<ConcreteCommand> {
		public:
			using MyBase = ModifyCommand<ConcreteCommand>;
            using command_holder = typename MyBase::command_holder;
		public:
			ModifyDepartment(CompanyTreeModel& ctm, EmployeePersonalFile personal_info) noexcept
				: MyBase(ctm), m_personal_info(personal_info)
			{
			}
		protected:
			QModelIndex get_department_index() const {
                return MyBase::get_target().DepartmentIndex(m_personal_info.department_pos);
			}
			size_t upper_bound_by_full_name(
				const QModelIndex& department, const QString& full_name
			) const {
                return *MyBase::get_target().UpperBoundChild(full_name, department);			//Поиск позиции для вставки сотрудника
			}
		protected:
			EmployeePersonalFile m_personal_info;
		};

		class ChangeEmployeeFullName : public ModifyDepartment<ChangeEmployeeFullName> {
		public:
			using MyBase = ModifyDepartment<ChangeEmployeeFullName>;
			using command_holder = MyBase::command_holder;
		public:
			ChangeEmployeeFullName(
				CompanyTreeModel& ctm,
				EmployeePersonalFile personal_info,						//QModelIndex может инвалидироваться при удалении элементов - на него полагаться нельзя!
				QString new_full_name
			) noexcept;
			std::any Execute() override;								//return type: new QModelIndex
			std::any Cancel() override;									//return type: new QModelIndex
			Type GetType() const noexcept override;
			static command_holder make_instance(
				CompanyTreeModel& ctm,
				EmployeePersonalFile personal_info,
				QString new_full_name
			);
		protected:
			std::optional<size_t> m_employee_cached_insert_pos;			//Кэширование позиции для ускорения операций отмены и повтора
			QString m_full_name;
		};

		struct WrapperInfo {											//Ничего, кроме имён, хранить не можем из-за риска инвалидации
			const wrapper::Company* company;	
			wrapper::string_ref department_name;
		};

		template<class ConcreteCommand>
		class Recruiter : public ModifyDepartment<ConcreteCommand> {
		public:
			using MyBase = ModifyDepartment<ConcreteCommand>;
			using command_holder = typename MyBase::command_holder;
			using Employee = wrapper::Employee;
		public:
			Recruiter(
				CompanyTreeModel& ctm,
				const WrapperInfo& wrapper_info,
				const EmployeePersonalFile& employee
			) noexcept : MyBase(ctm, employee),
				m_wrapper_info(wrapper_info)
			{
			}
		protected:
			const wrapper::FullNameRef& get_full_name() const {	
				return m_employee_name.value();									//Имя не инвалидируется, пока жив объект Employee
			}
			const Employee* get_employee_ptr(const wrapper::FullNameRef& full_name) const {
				return std::addressof(m_wrapper_info.company->at(m_wrapper_info.department_name).at(full_name));
			}
		protected:
			WrapperInfo m_wrapper_info;									
			std::optional<wrapper::FullNameRef> m_employee_name;				//Для поиска в department
		};

		class InsertEmployee : public Recruiter<InsertEmployee> {
		public:
			using MyBase = Recruiter<InsertEmployee>;
			using command_holder = MyBase::command_holder;
		public:
			InsertEmployee(
				CompanyTreeModel& ctm,
				const WrapperInfo& wrapper_info,
				size_t department_pos,
				const Employee& employee										//Для первой вставки
			) noexcept;
			std::any Execute() override;										//return type: bool
			std::any Cancel() override;											//return type: bool
			Type GetType() const noexcept override;
			static command_holder make_instance(
				CompanyTreeModel& ctm,
				const WrapperInfo& wrapper_info,
				size_t department_pos,
				const Employee& employee
			);
		protected:															
			const Employee* m_employee_ptr;
		};

		class RemoveEmployee : public Recruiter<RemoveEmployee> {
		public:
			using MyBase = Recruiter<RemoveEmployee>;
			using command_holder = MyBase::command_holder;
		public:
			RemoveEmployee(
				CompanyTreeModel& ctm,
				const WrapperInfo& wrapper_info,
				const QModelIndex& employee_q_idx								//Из модельного индекса извлечём данные для EmployeePersonalFile
			) noexcept;
			std::any Execute() override;										//return type: empty std::any
			std::any Cancel() override;											//return type: bool
			Type GetType() const noexcept override;
			static command_holder make_instance(
				CompanyTreeModel& ctm,
				const WrapperInfo& wrapper_info,
				const QModelIndex& employee_q_idx
			);
		protected:
			size_t get_department_position_from_employee(const QModelIndex& employee_q_idx);
		private:
			std::optional<CompanyTreeModel::Memento> m_branch;
		};
	}
}
