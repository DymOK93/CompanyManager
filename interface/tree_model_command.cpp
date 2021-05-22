#include "tree_model_command.h"

namespace command {
	namespace tree_model {
		InsertDepartment::InsertDepartment(
			CompanyTreeModel& ctm,
			const CompanyManager& company_manager,
			wrapper::string_ref department_name,
			size_t pos
		) noexcept : MyBase(ctm, company_manager, department_name, pos)
		{
		}

		std::any InsertDepartment::Execute() {
			auto& tree_model{ get_target() };
			QModelIndex q_idx{
				tree_model.InsertDepartmentItem(
					get_company().at(
						get_department_name()
					),
					get_department_position()			//Вставка в указанную позицию
				)
			};
			return q_idx.isValid();								//Сохраняем в m_department наименование подразделения
		}

		std::any InsertDepartment::Cancel() {
			auto& tree_model{ get_target() };
			return tree_model.RemoveDepartmentItem(get_department_position());
		}

		Type InsertDepartment::GetType() const noexcept {
			return Type::View_InsertDepartment;
		}

		InsertDepartment::command_holder InsertDepartment::make_instance(
			CompanyTreeModel& ctm,
			const CompanyManager& company_manager,
			wrapper::string_ref department_name,
			size_t pos
		) {
			return allocate_instance(ctm, company_manager, department_name, pos);
		}

		RenameDepartment::RenameDepartment(
			CompanyTreeModel& ctm,
			size_t pos,
			QString new_name
		) : MyBase(ctm),
			m_name(std::move(new_name)),
			m_pos(pos)
		{
		}

		std::any RenameDepartment::Execute() {
			auto& tree_model{ get_target() };
			QModelIndex q_idx{ tree_model.DepartmentIndex(m_pos) };
			auto old_name{ tree_model.ExchangeItemName(q_idx, m_name) };		//Пользуемся преимуществами CoW
			if (!old_name) {
				return false;
			}
			m_name = std::move(*old_name);
			return true;
		}

		std::any RenameDepartment::Cancel() {
			return Execute();													//let's swap
		}

		Type RenameDepartment::GetType() const noexcept {
			return Type::View_RenameDepartment;
		}

		RenameDepartment::command_holder RenameDepartment::make_instance(
			CompanyTreeModel& ctm,
			size_t row,
			QString new_name
		) {
			return allocate_instance(ctm, row, std::move(new_name));
		}

		RemoveDepartment::RemoveDepartment(
			CompanyTreeModel& ctm,
			const CompanyManager& company_manager,
			wrapper::string_ref department_name,							//Для обновления указателя на подразделение
			size_t pos
		) noexcept : MyBase(ctm, company_manager, department_name, pos)
		{
		}

		std::any RemoveDepartment::Execute() {
			m_branch = get_target().DumpDepartmentItem(
				get_department_position()
			);
			return make_default_value();
		}

		std::any RemoveDepartment::Cancel() {
			return get_target().RestoreDepartmentItem(
				std::move(*m_branch),
				std::addressof(get_company().at(get_department_name()))
			);
		}

		Type RemoveDepartment::GetType() const noexcept {
			return Type::View_RemoveDepartment;
		}

		RemoveDepartment::command_holder RemoveDepartment::make_instance(
			CompanyTreeModel& ctm,
			const CompanyManager& company_manager,
			wrapper::string_ref department_name,
			size_t pos
		) {
			return allocate_instance(ctm, company_manager, department_name, pos);
		}

		ChangeEmployeeFullName::ChangeEmployeeFullName(
			CompanyTreeModel& ctm,
			EmployeePersonalFile personal_info,
			QString new_full_name
		) noexcept : MyBase(ctm, personal_info),
			m_full_name(std::move(new_full_name))
		{
		}


		std::any ChangeEmployeeFullName::Execute() {
			auto& tree_model(get_target());
			QModelIndex department_idx(get_department_index());
			if (!m_employee_cached_insert_pos) {										//Поиск позиции для вставки
				m_employee_cached_insert_pos = upper_bound_by_full_name(department_idx, m_full_name);
			}

			m_full_name = *tree_model.ExchangeItemName(									//Смена ФИО с сохранением предыдущего
				tree_model.EmployeeIndex(department_idx, m_personal_info.employee_pos),
				std::move(m_full_name)
			);
			std::swap(*m_employee_cached_insert_pos, m_personal_info.employee_pos);			//Теперь в cached - предыдущая позиция
			size_t result{
				*tree_model.MoveItem(												//Т.к. используется UpperBound, совпасть индексы не могут
				*m_employee_cached_insert_pos,
				m_personal_info.employee_pos,											
				department_idx)
			};
			if (result < m_personal_info.employee_pos) {
				m_personal_info.employee_pos = result;
			}
			else {										
				*m_employee_cached_insert_pos += 1;				//Смещаем индекс для вставки, если целевая и итоговая с учетом сдвига элементов позиция совпали
			}																			
			return tree_model.EmployeeIndex(department_idx, m_personal_info.employee_pos);	//Возвращаем обновленный индекс
		}

		std::any ChangeEmployeeFullName::Cancel() {
			return Execute();
		}

		Type ChangeEmployeeFullName::GetType() const noexcept {
			return Type::View_ChangeEmployeeFullName;
		}

		ChangeEmployeeFullName::command_holder ChangeEmployeeFullName::make_instance(
			CompanyTreeModel& ctm,
			EmployeePersonalFile personal_info,
			QString new_full_name
		) {
			return allocate_instance(ctm, personal_info, std::move(new_full_name));
		}

		InsertEmployee::InsertEmployee(
			CompanyTreeModel& ctm,
			const WrapperInfo& wrapper_info,
			size_t department_pos,
			const Employee& employee
		) noexcept : MyBase(
			ctm, wrapper_info, 
			EmployeePersonalFile{ department_pos, 0 }				//Позиция пока неизвестна
		),	
			m_employee_ptr(std::addressof(employee))
		{
		}
		
		std::any InsertEmployee::Execute() {
			auto& tree_model{ get_target() };
			const Employee* emp_ptr;
			if (m_employee_ptr) {					//Хранить указатель после первого выполнения Execute() нельзя, т.к. после отмены операции при последующем повторе
				emp_ptr = m_employee_ptr;			//элемент будет заново вставлен в XML-дерево
				m_employee_name = emp_ptr->GetFullName();
				m_employee_ptr = nullptr;
			}
			else {
				emp_ptr = get_employee_ptr(
					get_full_name()
				);
			}	
			QModelIndex department_q_idx{ get_department_index() };
			m_personal_info.employee_pos = MyBase::upper_bound_by_full_name(
							department_q_idx,
							CompanyTreeModel::FullNameRefToQString(
							get_full_name()
					)
			);
			return tree_model.InsertEmployeeItem(
				*emp_ptr,
				department_q_idx,
				m_personal_info.employee_pos
			).isValid();
		}

		std::any InsertEmployee::Cancel() {
			return get_target().RemoveEmployeeItem(				
				get_department_index(),
				m_personal_info.employee_pos
			);								
		}

		Type InsertEmployee::GetType() const noexcept {
			return Type::View_InsertEmployee;
		}

		InsertEmployee::command_holder InsertEmployee::make_instance(
			CompanyTreeModel& ctm,
			const WrapperInfo& wrapper_info,
			size_t department_pos,
			const Employee& employee
		) {
			return allocate_instance(ctm, wrapper_info, department_pos, employee);
		}

		RemoveEmployee::RemoveEmployee(
			CompanyTreeModel& ctm,
			const WrapperInfo& wrapper_info,
			const QModelIndex& employee_q_idx
		) noexcept 
			: MyBase(
			ctm, wrapper_info,
			EmployeePersonalFile{
				get_department_position_from_employee(employee_q_idx),
				static_cast<size_t>(employee_q_idx.row())
			})
		{
		}
			
		std::any RemoveEmployee::Execute() {									
			QModelIndex department_q_idx{ get_department_index() };
			if (!m_employee_name) {				
				QModelIndex employee_q_idx{
				get_target().EmployeeIndex(
					department_q_idx,
					m_personal_info.employee_pos
				)
			};		
				m_employee_name = std::get<const Employee*>(					//На момент выполнения Execute() элемент модели и указатель еще валидны, 
						get_target().GetItemNode(employee_q_idx)		//поскольку удаление из дерева данных выполняется позже
					)->GetFullName();
			}
			m_branch = get_target().DumpEmployeeItem(						
				department_q_idx,
				m_personal_info.employee_pos
			);
			 return make_default_value();
		}

		std::any RemoveEmployee::Cancel() {										//Для Remove-операций восстановление в дереве данных должно производиться раньше
			auto& tree_model{ get_target() };							//восстановления в модели, чтобы можно было извлечь необходимые сведения
			const Employee* emp_ptr{ get_employee_ptr(get_full_name()) };
			return tree_model.RestoreEmployeeItem(
				std::move(*m_branch),
				emp_ptr
			);
		}

		Type RemoveEmployee::GetType() const noexcept {
			return Type::View_RemoveEmployee;
		}

		RemoveEmployee::command_holder RemoveEmployee::make_instance(
			CompanyTreeModel& ctm,
			const WrapperInfo& wrapper_info,
			const QModelIndex& employee_q_idx
		) {
			return allocate_instance(ctm, wrapper_info, employee_q_idx);
		}

		size_t RemoveEmployee::get_department_position_from_employee(const QModelIndex& employee_q_idx) {
			return employee_q_idx.parent().row();
		}
	}
}
