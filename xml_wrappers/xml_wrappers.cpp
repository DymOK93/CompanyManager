#include "xml_wrappers.h"
using namespace std;
using xml::Node;
using xml::node_holder;
using xml::allocator_holder;

namespace wrapper {
	XmlWrapper::XmlWrapper(Node* node_ptr)
		: m_node(node_ptr)
	{
	}

	XmlWrapper::XmlWrapper(node_holder ready_node)
		: m_node(move(ready_node))
	{
	}

	XmlWrapper::XmlWrapper(XmlWrapper&& other) noexcept
        : m_node(exchange(other.m_node, nullptr) )
	{
	}

	XmlWrapper& XmlWrapper::operator=(XmlWrapper&& other) noexcept {
		if (this != addressof(other)) {
            m_node = exchange(other.m_node, nullptr);
		}
		return *this;
	}

	XmlWrapper& XmlWrapper::OwnFrom(XmlWrapper& source) {
		if (source.IsObserver()) {
			m_node = xml::NodeBuilder::Take(move(source.get_node()));	
		}
		else {
			m_node = exchange(source.m_node, nullptr);										
		}
		throw_if_different_wrapper_types(get_type(), source.get_type());					//В take_dependencies() будет выполнен dynamic_cast
		take_dependencies(source);
		return *this;
	}

	XmlWrapper& XmlWrapper::CopyFrom(const XmlWrapper& source) {
		m_node = xml::NodeBuilder::Duplicate(source.get_node());
		update_dependencies();
		return *this;
	}

	XmlWrapper& XmlWrapper::Reset() {
		m_node = nullptr;
		return *this;
	}

	XmlWrapper::Mode XmlWrapper::CurrentMode() const noexcept {
		return static_cast<Mode>(m_node.index());
	}

	bool XmlWrapper::IsObserver() const noexcept {
		return CurrentMode() == Mode::View;
	}

	node_holder XmlWrapper::BuildXmlTree() {
		Synchronize();															//Предварительная синхронизация обязательна во избежание утери изменений
		if (IsObserver()) {
			throw runtime_error("XML Node isn't owned by wrapper");
		}
		node_holder detached_node{ move(get<node_holder>(m_node)) };
		m_node = detached_node.get();											//Переключаемся в режим наблюдения
		return detached_node;
	}

	node_holder XmlWrapper::BuildXmlTree(XmlWrapper& wrapper) {
		return wrapper.BuildXmlTree();
	}

	allocator_holder XmlWrapper::GetAllocator() const noexcept {
		return get_node().GetAllocator();
	}

	void XmlWrapper::throw_if_another_node_type(const Node& node, Node::Type expected) {
		if (auto type = node.GetType(); type != expected) {
			throw runtime_error(
				"Invalid XML node type: "
				+ string(Node::TypeAsString(expected))
				+ " expected, but "
				+ string(Node::TypeAsString(type))
				+ " found"
			);
		}
	}

	void XmlWrapper::throw_if_different_wrapper_types(Type left, Type right) {
		if (left != right) {
			throw runtime_error("Different wrapper types");
		}
	} 

	void XmlWrapper::throw_non_existent_key(string key) {
		throw out_of_range(move(key) + " doesn't exist");
	}

	xml::Node& XmlWrapper::get_node() {
		if (holds_alternative<Node*>(m_node)) {										//Получение ссылки на узел
			return *get<Node*>(m_node);
		}
		else {
			return *get<node_holder>(m_node);
		}
	}

	const xml::Node& XmlWrapper::get_node() const {
		if (holds_alternative<Node*>(m_node)) {
			return *get<Node*>(m_node);
		}
		else {
			return *get<node_holder>(m_node);
		}
	}

	Employee::Employee(xml::Node* node_ptr)
		: XmlWrapper(node_ptr), 
		m_properties(collect_properties(*node_ptr))	
	{
	}

	Employee::Employee(node_holder ready_node)
		: XmlWrapper(move(ready_node)),
		m_properties(collect_properties(get_node()))
	{
	}

	Employee& Employee::Synchronize() {
		return *this;
	}

	Employee& Employee::Reset() {
		XmlWrapper::Reset();
		m_properties.clear();
		return *this;
	}

	XmlWrapper::Type Employee::get_type() const noexcept {
		return Type::Employee;
	}

	Employee& Employee::update_dependencies() {
		m_properties = collect_properties(get_node());
		return *this;
	}

	Employee& Employee::take_dependencies(XmlWrapper& other) {
		m_properties = move(static_cast<Employee&>(other).m_properties);
		return *this;
	}

	FullNameRef Employee::GetFullName() const {
		return FullNameRef{
			*m_properties.at("surname"),
			*m_properties.at("name"),
			*m_properties.at("middleName")
		};
	}

	string_ref Employee::GetSurname() const {
		return *m_properties.at("surname");
	}

	string_ref Employee::GetName() const {
		return *m_properties.at("name");
	}

	string_ref Employee::GetMiddleName() const {
		return *m_properties.at("middleName");
	}


	string_ref Employee::GetFunction() const {
		return *m_properties.at("function");
	}

	size_t Employee::GetSalary() const {
		return stoll(*m_properties.at("salary"));
	}

	Employee& Employee::SetSurname(string new_surname) {
		*m_properties["surname"] = move(new_surname);
		return *this;
	}

	Employee& Employee::SetName(string new_name) {
		*m_properties["name"] = move(new_name);
		return *this;
	}
	
	Employee& Employee::SetMiddleName(string new_middle_name) {
		*m_properties["middleName"] = move(new_middle_name);
		return *this;
	}

	Employee& Employee::SetFunction(string new_function) {
		*m_properties["function"] = move(new_function);
		return *this;
	}

	Employee& Employee::SetSalary(size_t new_salary) {
		*m_properties["salary"] = to_string(new_salary);
		return *this;
	}

	Employee::properties_view_t Employee::collect_properties(Node& node) {
		throw_if_another_node_type(node, Node::Type::Tree);									//XML-узел должен хранить дочерние узлы

		auto& raw_properties{ node.AsContainer() };
		properties_view_t properties;
		for (auto& employee_holder : raw_properties) {
			properties.emplace(employee_holder->GetName(), addressof(employee_holder->AsText()));
		}
		return properties;
	}

	Department::Department(Node* node_ptr) 
		: XmlContainerWrapper(node_ptr),
		m_workgroup(collect_employees(*node_ptr)),
		m_summary_salary(calc_summary_salary(m_workgroup))
	{
	}

	Department::Department(node_holder ready_node)
		:XmlContainerWrapper(move(ready_node)),
		m_workgroup(collect_employees(get_node())),
		m_summary_salary(calc_summary_salary(m_workgroup))
	{
	}

	Department& Department::Reset() {
		XmlWrapper::Reset();
		m_workgroup.clear();
		m_summary_salary = 0;
		return *this;
	}

	XmlWrapper::Type Department::get_type() const noexcept {
		return Type::Department;
	}

	Department& Department::update_dependencies() {
		m_workgroup = collect_employees(get_node());
		m_summary_salary = calc_summary_salary(m_workgroup);
		return *this;
	}

	Department& Department::take_dependencies(XmlWrapper& other) {
		auto& other_department{ static_cast<Department&>(other) };
		m_workgroup = move(other_department.m_workgroup);
		m_summary_salary = exchange(other_department.m_summary_salary, 0);
		return *this;
	}

	RenameResult Department::employee_rename_helper(const FullNameRef& old_name, string&& value, FullNameField field){
		FullNameRef new_full_name(old_name);
		switch (field) {											//Для поиска дубликатов
		case FullNameField::Surname: new_full_name.surname = value; break;
		case FullNameField::Name: new_full_name.name = value; break;
		case FullNameField::MiddleName: new_full_name.middle_name = value; break;
		}

		if (m_workgroup.count(new_full_name)) {						//Смена имени требует выполнения "тяжелой" операции - извлечения из workgroup
			return new_full_name == old_name ?
				RenameResult::NothingChanged : RenameResult::IsDuplicate; //Найдены дубликаты или имя не изменилось
		}

		auto employee_it{ m_workgroup.find(old_name) };				//todo: изменение позиции в кэше
		if (employee_it == m_workgroup.end()) {	
			throw_non_existent_key("Employee with full name " + FullNameRefAsString(old_name));
		}
		register_rename(old_name, new_full_name);					//Обновление позиции элемента в кэше

		auto next_it{ next(employee_it) };
		auto node{ m_workgroup.extract(employee_it) };

		switch (field) {											//Обновление поля
		case FullNameField::Surname: node.mapped().SetSurname(move(value)); break;
		case FullNameField::Name: node.mapped().SetName(move(value)); break;
		case FullNameField::MiddleName:  node.mapped().SetMiddleName(move(value)); break;
		};

		node.key() = node.mapped().GetFullName();
		employee_it = m_workgroup.insert(move(node)).position;
		if (next(employee_it) != next_it) {
			force_rebuild();
		}
		return RenameResult::Success;
	}

	void Department::recalc_summary_salary_after_recruitment(size_t new_employee_salary) {
		m_summary_salary += new_employee_salary;
	}

	void Department::recalc_summary_salary_before_dismissal(size_t former_employee_salary) {
		m_summary_salary -= former_employee_salary;
	}

	Department& Department::Synchronize() {
		auto* staff{ try_get_staff(get_node()) };
		if (!staff) {
			auto& cont{ get_node().AsContainer() };
			cont.push_back(
				xml::DocumentNodeBuilder()
				.SetName("employments")
				.SetAllocator(get_node().GetAllocator())
				.Assemble()
			);
			staff = cont.back().get();
		}
		synchronize_helper<Employee>(									
			addressof(staff->AsContainer()),
			m_workgroup.begin(),
			m_workgroup.end(),
			m_workgroup.size(),
			[](workgroup_t::value_type& name_and_employee) -> Employee& {					//Без явного указания, что возвращается ссылка,			
				return name_and_employee.second;											//будет попытка копирования (т.к. возвращается поле аргумента ф-ции)
			});	
		return *this;
	}

	bool operator<(const FullNameRef& left, const FullNameRef& right) {
		return 
			tie(left.surname.get(), left.name.get(), left.middle_name.get())
			< tie(right.surname.get(), right.name.get(), right.middle_name.get());
	}

	bool operator==(const FullNameRef& left, const FullNameRef& right) {
		return
			tie(left.surname.get(), left.name.get(), left.middle_name.get())
			== tie(right.surname.get(), right.name.get(), right.middle_name.get());
	}

	bool operator!=(const FullNameRef& left, const FullNameRef& right) {
		return !(left == right);
	}

	size_t FullNameHasher::operator()(const FullNameRef& name) const noexcept {
		static constexpr size_t coef{ 1873 };											//Простое число
		return
			hash<string>()(name.surname) * coef * coef
			+ hash<string>()(name.surname) * coef
			+ hash<string>()(name.surname);
	}

	string FullNameRefAsString(const FullNameRef& full_name) {
		string full_name_str;
		full_name_str.reserve(
			full_name.surname.get().size()
			+ full_name.name.get().size()
			+ full_name.middle_name.get().size()
			+ 2															//Пробельные символы
		);
		full_name_str += full_name.surname;
		full_name_str.push_back(' ');
		full_name_str += full_name.name;
		full_name_str.push_back(' ');
		full_name_str += full_name.middle_name;
		return full_name_str;
	}


	Department::workgroup_t Department::collect_employees(xml::Node& node) {
		throw_if_another_node_type(node, Node::Type::Tree);

		workgroup_t workgroup;
		auto* staff{ try_get_staff(node) };
		if (staff) {
			for (auto& nodeholder : staff->AsContainer()) {
				Employee employee(nodeholder.get());
				workgroup.emplace(employee.GetFullName(), move(employee));
			}
		}
		return workgroup;
	}
		
	xml::Node* Department::try_get_staff(xml::Node& node) {								//Попытка обращения к XML-узлу <employments>...</employments>
		auto& raw_info{ node.AsContainer() };
		auto staff_it{
			find_if(
				raw_info.begin(),
				raw_info.end(),
				[](const node_holder& node) {
					return node->GetName() == "employments"; 
				}
			)
		};
		return staff_it == raw_info.end() ?
			nullptr : staff_it->get();
	}

	Department::salary_t Department::calc_summary_salary(const workgroup_t& workgroup) {
		salary_t summary_salary{ 0 };
		for (const auto& [full_name, employee] : workgroup) {
			summary_salary += employee.GetSalary();
		}
		return summary_salary;
	}

	string_ref Department::GetName() const {
		return get_node().at("name");
	}

	Department& Department::SetName(string new_name) noexcept {
		get_node()["name"] = move(new_name);
		return *this;
	}

	RenameResult Department::ChangeEmployeeSurname(const FullNameRef& old_name, std::string new_surname) {
		return employee_rename_helper(old_name, move(new_surname), FullNameField::Surname);
	}

	RenameResult Department::ChangeEmployeeName(const FullNameRef& old_name, std::string new_name) {
		return employee_rename_helper(old_name, move(new_name), FullNameField::Name);
	}

	RenameResult Department::ChangeEmployeeMiddleName(const FullNameRef& old_name, std::string new_middle_name) {
		return employee_rename_helper(old_name, move(new_middle_name), FullNameField::MiddleName);
	}

	Department& Department::UpdateEmployeeSalary(const FullNameRef& full_name, size_t new_salary) {
		auto& employee{ at(full_name) };
		m_summary_salary += (new_salary - employee.GetSalary());
		employee.SetSalary(new_salary);
		return *this;
	}

	size_t Department::EmployeeCount() const noexcept {
		return m_workgroup.size();
	}

	double Department::AverageSalary() const noexcept {
		return EmployeeCount() ? 
			static_cast<double>(m_summary_salary) / EmployeeCount() : 0;
	}

	bool Department::Containts(const FullNameRef& name) const noexcept {
		return static_cast<bool>(m_workgroup.count(name));
	}

	Employee& Department::at(const FullNameRef& name) {
		return m_workgroup.at(name);
	}

	const Employee& Department::at(const FullNameRef& name) const {
		return m_workgroup.at(name);
	}

	Department::employee_it Department::Find(const FullNameRef& name) {
		return m_workgroup.find(name);
	}

	Department::employee_view_it Department::Find(const FullNameRef& name) const {
		return m_workgroup.find(name);
	}

	Department::employee_it Department::InsertEmployee(Employee&& employee) {
		auto [it, success] { 
			m_workgroup.emplace(
				employee.GetFullName(),
				move(employee)
			)
		};
		if (success) {
			register_insert(it->first, it == prev(m_workgroup.end()));
			recalc_summary_salary_after_recruitment(it->second.GetSalary());
		}
		return it;
	}

	Department::employee_it Department::EraseEmployee(employee_it employee) {
		if (employee == m_workgroup.end()) {
			throw_non_existent_key("Employee");
		}
		register_erase(employee->second.GetFullName(), employee == prev(m_workgroup.end()));
		recalc_summary_salary_before_dismissal(employee->second.GetSalary());
		return m_workgroup.erase(employee);
	}

	bool Department::EraseEmployee(const FullNameRef& name) {
		auto it { m_workgroup.find(name) };
		if (it == m_workgroup.end()) {
			return false;
		}	
		EraseEmployee(it);
		return true;
	}

	Employee Department::ExtractEmployee(employee_it employee) {
		if (employee == m_workgroup.end()) {
			throw_non_existent_key("Employee");
		}
		register_erase(
			employee->second.GetFullName(), employee
			== prev(m_workgroup.end())
		);
		recalc_summary_salary_before_dismissal(employee->second.GetSalary());
		Employee extracted_employee{ MoveFrom<Employee>(employee->second) };	
		m_workgroup.erase(employee);
		return extracted_employee;
	}

	Employee Department::ExtractEmployee(const FullNameRef& name) {
		return ExtractEmployee(m_workgroup.find(name));
	}

	Department& Department::SetWorkgroup(workgroup_t new_workgroup) {
		m_workgroup = move(new_workgroup);
		force_rebuild();
		return *this;
	}

	Department::employee_range Department::GetEmployees() noexcept {
		return { m_workgroup.begin(), m_workgroup.end() };
	}

	Department::employee_view_range Department::GetEmployees() const noexcept {
		return { m_workgroup.begin(), m_workgroup.end() };
	}

	Company::Company(Node* node_ptr)
		: XmlContainerWrapper(node_ptr),
		m_subdivision(collect_departaments(*node_ptr)),
		m_subdivision_map(make_subdivision_map(m_subdivision))
	{
	}

	Company::Company(node_holder ready_node)
		: XmlContainerWrapper(move(ready_node)),
		m_subdivision(collect_departaments(get_node())),
		m_subdivision_map(make_subdivision_map(m_subdivision))
	{
	}

	Company& Company::Reset() {
		XmlWrapper::Reset();
		m_subdivision_map.clear();
		m_subdivision.clear();
		return *this;
	}

	XmlWrapper::Type Company::get_type() const noexcept {
		return Type::Company;
	}

	Company& Company::update_dependencies() {
		m_subdivision = collect_departaments(get_node());
		m_subdivision_map = make_subdivision_map(m_subdivision);
		return *this;
	}

	Company& Company::take_dependencies(XmlWrapper& other) {
		auto& other_company{ static_cast<Company&>(other) };							//Допустимо, т.к. в случае несовпадения типов исключение выбросит OwnFrom()
			
		m_subdivision = move(other_company.m_subdivision);
		m_subdivision_map = move(other_company.m_subdivision_map);
		return *this;
	}

	Company& Company::Synchronize() {
		synchronize_helper<Department>(														//Тип элементов диапазона
			addressof(get_node().AsContainer()),
			m_subdivision.begin(),
			m_subdivision.end(),
			m_subdivision.size(),
			[](Department& department) -> Department& {										//Экстрактор-заглушка
			return department;
		}
		);
		return *this;
	}

	Company::subdivision_t Company::collect_departaments(Node& node) {
		throw_if_another_node_type(node, Node::Type::Tree);

		auto& raw_subdivision{ node.AsContainer() };
		subdivision_t subdivision;
	
		for (auto& departament_holder : raw_subdivision) {
			subdivision.push_back(Department(departament_holder.get()));
		}
		return subdivision;
	}

	Company::subdivision_map_t Company::make_subdivision_map(subdivision_t& list) {
		subdivision_map_t subdivision_map;
		for (auto it = list.begin(); it != list.end(); ++it) {
			insert_to_subdivision_map(addressof(subdivision_map), it);
		}
		return subdivision_map;
	}

	void Company::throw_non_existent_department(const string& name) {
		throw_non_existent_key("Department "s + string(name) + ' ');
	}

	RenameResult Company::RenameDepartment(const string& department, string new_name) {
		if (m_subdivision_map.count(new_name)) {								//Операции извлечения и вставки - долгие. Эффективнее предварительно сравнить строки
			return department == new_name ?
				RenameResult::NothingChanged : RenameResult::IsDuplicate;
		}	
		if (!m_subdivision_map.count(department)) {
			throw_non_existent_department(department);
		}
		register_rename(department, new_name);								

		auto node{ m_subdivision_map.extract(department) };
		node.mapped()->SetName(move(new_name));
		node.key() = node.mapped()->GetName().get();
		m_subdivision_map.insert(move(node));
		return RenameResult::Success;
	}

	size_t Company::DepartmentCount() const noexcept {
		return m_subdivision.size();
	}

	bool Company::Containts(const std::string& name) const noexcept {
		return static_cast<bool>(m_subdivision_map.count(name));
	}

	Company::department_range Company::GetDepartments() noexcept {
		return { m_subdivision.begin(), m_subdivision.end() };
	}


	Company::department_view_range Company::GetDepartments() const noexcept {
		return { m_subdivision.begin(), m_subdivision.end() };
	}

	void Company::insert_to_subdivision_map(subdivision_map_t* map, department_it it) {
		map->emplace(it->GetName().get(), it);
	}

	Company::department_it Company::erase_from_subdivision(department_it department) {
		return m_subdivision.erase(department);
	}

	Department Company::extract_from_subdivision(department_it department) {
		register_erase(
			department->GetName().get(), department == prev(m_subdivision.end())
		);
		Department extracted_department{ MoveFrom<Department>(*department) };
		m_subdivision.erase(department);
		return extracted_department;
	}

	Company::department_it Company::AddDepartment(Department&& new_department) {
		if (auto already_exists_it = m_subdivision_map.find(new_department.GetName().get());
			already_exists_it != m_subdivision_map.end()) {
			return already_exists_it->second;
		}
		auto it{ m_subdivision.insert(m_subdivision.end(), move(new_department)) };
		register_insert(it->GetName().get(), it == prev(m_subdivision.end()));
		insert_to_subdivision_map(addressof(m_subdivision_map), it);
		return it;
	}

	Company::department_it Company::InsertDepartment(const std::string& before, Department&& new_department) {
		if (auto already_exists_it = m_subdivision_map.find(new_department.GetName().get());
			already_exists_it != m_subdivision_map.end()) {
			return already_exists_it->second;
		}
		auto before_it{ m_subdivision_map.find(before) };
		if (before_it == m_subdivision_map.end()) {
			throw_non_existent_department(before);
		}
		return InsertDepartment(before_it->second, move(new_department));
	}

	Company::department_it Company::InsertDepartment(department_it before, Department&& new_department) {
		auto it{ m_subdivision.insert(before, move(new_department)) };
		register_insert(it->GetName().get(), it == prev(m_subdivision.end()));
		insert_to_subdivision_map(addressof(m_subdivision_map), it);
		return it;
	}

	const Department* Company::TryGetNext(const string& department) const noexcept {
		auto it { m_subdivision_map.find(department)};
		if (it == m_subdivision_map.end()) {
			return nullptr;
		}
		return TryGetNext(it->second);
	}

	const Department* Company::TryGetNext(department_it department) const noexcept {
		if (department == m_subdivision.end()
			|| department == prev(m_subdivision.end())) {
			return nullptr;
		}
		return addressof(*next(department));
	}

	Company::department_it Company::EraseDepartment(department_it department) {
		if (department == m_subdivision.end()) {
			throw_non_existent_key("Department");
		}
		m_subdivision_map.erase(department->GetName().get());
		register_erase(department->GetName().get(), department == prev(m_subdivision.end()));
		return erase_from_subdivision(department);
	}

	bool Company::EraseDepartment(const string& name) {
		auto it{ m_subdivision_map.find(name) };
		if (it == m_subdivision_map.end()) {
			return false;
		}
		erase_from_subdivision(it->second);
		m_subdivision_map.erase(it);
		return true;
	}

	Department Company::ExtractDepartment(department_it department) {
		if (department == m_subdivision.end()) {
			throw_non_existent_key("Department");
		}
		m_subdivision_map.erase(department->GetName().get());
		return extract_from_subdivision(department);
	}

	Department Company::ExtractDepartment(const string& name) {
		auto it{ m_subdivision_map.find(name) };
		if (it == m_subdivision_map.end()) {
			throw_non_existent_department(name);
		}
		auto dep_it{ it->second };
		m_subdivision_map.erase(it);
		return extract_from_subdivision(dep_it);
	}

	Company& Company::SetSubdivision(subdivision_t new_subdivision) {
		m_subdivision = move(new_subdivision);
		m_subdivision_map = make_subdivision_map(m_subdivision);
		force_rebuild();
		return *this;
	}

	Company& Company::ClearSubdivision() noexcept {
		m_subdivision.clear();
		m_subdivision_map.clear();
		force_rebuild();
		return *this;
	}

	Department& Company::at(const string& name) {
		return *m_subdivision_map.at(name);
	}

	const Department& Company::at(const string& name) const {
		return *m_subdivision_map.at(name);
	}

	Company::department_it Company::Find(const string& name) {
		auto department{ m_subdivision_map.find(name) };
		if (department == m_subdivision_map.end()) {
			throw_non_existent_key("Department");
		}
		return department->second;
	}

	Company::department_view_it Company::Find(const string& name) const {
		auto department{ m_subdivision_map.find(name) };
		if (department == m_subdivision_map.end()) {
			throw_non_existent_key("Department");
		}
		return department->second;
	}
}

