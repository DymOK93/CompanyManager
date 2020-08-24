#pragma once
#include "xml.h"
#include "xml_node_builders.h"

#include <variant>
#include <stdexcept>
#include <string_view>
#include <map>
#include <list>
#include <unordered_map>	
#include <unordered_set>
#include <algorithm>
#include <functional>
#include <type_traits>

namespace wrapper {
	class XmlWrapper {
	public:
		enum class Mode {
			View,
			Own
		};
		enum class Type {
			Employee,
			Department,
			Company
		};
		static constexpr std::string_view ModeAsString(Mode mode) {
			return mode_as_string[static_cast<size_t>(mode)];
		}
	private:
		static constexpr std::array<std::string_view, 4> mode_as_string{
			"view", "own"
		};
	public:
		XmlWrapper() = default;
		XmlWrapper(xml::Node* node_ptr);								//������������� � ������ ����������
		XmlWrapper(XmlWrapper&& other) noexcept;
		XmlWrapper& operator=(XmlWrapper&& other) noexcept;
		virtual ~XmlWrapper() = default;

		Mode CurrentMode() const noexcept;
		bool IsObserver() const noexcept;								//true, ���� CurrentMode() == VIEW

		virtual XmlWrapper& OwnFrom(XmlWrapper& source);				//������� ����, ���������� � ���� ������ �� source � ������������� � ����� ��������
		XmlWrapper& CopyFrom(const XmlWrapper& source);					//�������� ���������� �� ����, �� ������� ��������� source

		virtual XmlWrapper& Synchronize() = 0;							//�������������� �������� �������� ��� ����������� ���������� ����. 
		virtual XmlWrapper& Reset() = 0;								//���������� ��������� �� ���� 
		virtual xml::node_holder BuildXmlTree();
		static xml::node_holder BuildXmlTree(XmlWrapper& wrapper);		//���������� ��������� ���� � ������������� � ����� ����������

		xml::allocator_holder GetAllocator() const noexcept;			//��������� �������� �������� �� �������������� ����
	protected:
		template <class WrapperTy>										//������� ����� ������� ���� �� XmlWrapper 
		static WrapperTy MoveFrom(WrapperTy& source) {
			WrapperTy wrapper;
			wrapper.OwnFrom(source);
			return wrapper;
		}

		static void throw_if_another_node_type(const xml::Node&, xml::Node::Type expected);
		static void throw_if_different_wrapper_types(Type left, Type right);
		static void throw_non_existent_key(std::string key);

		virtual Type get_type() const noexcept = 0;							//����������� ��� �������� �������
		virtual XmlWrapper& update_dependencies() = 0;						//��������� ��������� ������ ����� ������ OwnFrom()
		virtual XmlWrapper& take_dependencies(XmlWrapper& other) = 0;		//�������� ��������� ������ � ������ ������

		xml::Node& get_node();
		const xml::Node& get_node() const;
	protected:
		std::variant<
			xml::Node*,														//����� ����������
			xml::node_holder												//����� ��������
		> m_node{ nullptr };
	protected:
		XmlWrapper(xml::node_holder ready_node);
	};
	
	/***********************************************************************************************************************
	������������ string_view �����������, ��������� ��� ��������� ������ �������� ���� ������ const char*, 
	�� ������� ��������� string_view, ����� ������������ �������. ����� �� ��������� �������� FullName ������� ������� 
	������ ������ �����, ������������� ��������� StringRef, �.�. ���� ������� ����� �� ����� �������������� ��
	����� ����� ����� �������� �� XML-�����.
	(���� ������ ��������� - �������� ���� Employee-node, 
	��� �������� - ������� ������������� ���������� ������ Department-node).
	������� � �++17, ���������� �������������, ��� string_ref - TriviallyCopyable
	************************************************************************************************************************/

	using string_ref = std::reference_wrapper<const std::string>;

	struct FullNameRef {
		string_ref
			surname,
			name,
			middle_name;
	};
	bool operator<(const FullNameRef& left, const FullNameRef& right);
	bool operator!=(const FullNameRef& left, const FullNameRef& right);
	bool operator==(const FullNameRef& left, const FullNameRef& right);
	struct FullNameHasher {
		size_t operator()(const FullNameRef& name) const noexcept;
	};
	std::string FullNameRefAsString(const FullNameRef& full_name);

	class Employee : public XmlWrapper {
	public:
		using salary_t = size_t;
		using properties_view_t = std::unordered_map<std::string_view, std::string*>;
	public:
		Employee() = default;
		Employee(xml::Node*);

		Employee& Synchronize() override;									//�����-��������: ������������� ����������� ��������������� ��� �������� ���������
		Employee& Reset() override;

		FullNameRef GetFullName() const;									//���
		string_ref GetSurname() const;
		string_ref GetName() const;
		string_ref GetMiddleName() const;
		string_ref GetFunction() const;
		size_t GetSalary() const;
																											
		Employee& SetSurname(std::string new_surname);						//�.�. ����� ����� ��� ������� ����� ������� � �������������, 
		Employee& SetName(std::string new_name);							//��� ������ ����������� ������ ����� ��������� ����������!	
		Employee& SetMiddleName(std::string new_middle_name);				//(���� �������� �� ��������)		
		Employee& SetFunction(std::string new_function);											
		Employee& SetSalary(size_t new_salary);				//��� ����������� ����� ���������� ����� �/� ������ ������������� ����� ��������� �������������

	protected:
		friend class EmployeeBuilder;
		Employee(xml::node_holder ready_node);

		Type get_type() const noexcept override;
		Employee& update_dependencies() override;
		Employee& take_dependencies(XmlWrapper& other) override;

		static properties_view_t collect_properties(xml::Node&);

	protected:
		properties_view_t m_properties;
	};

	template <class CachedTy, class Hasher = std::hash<CachedTy>>					//CachedTy must be easy-copyable
	class XmlContainerWrapper : public XmlWrapper {
	public:
		using title_cache_t = std::unordered_set<CachedTy, Hasher>;
		struct State {
			size_t inserted_back_count{ 0 }, erased_back_count{ 0 };
			bool force_rebuild{ false };
		};
	public:
		using XmlWrapper::XmlWrapper;
	protected:
		void register_insert(const CachedTy& value, bool insert_back) {
            m_inserted.insert(value);
			if (insert_back) {
				if (m_state.erased_back_count > 0) {
					--m_state.erased_back_count;
					force_rebuild();											//�� ��������� �������� � ��������� � �������� ����������� ���-�� ������ ���������
				}
				else {
					++m_state.inserted_back_count;
				}			
			}
			else {
				force_rebuild();
			}
		}

		void register_erase(const CachedTy& value, bool erase_back) {
			if (erase_back) {
				m_state.inserted_back_count > 0 ?
					--m_state.inserted_back_count : ++m_state.erased_back_count;
				m_inserted.erase(value);
			}
			else {
				if (auto it = m_inserted.find(value);
					it == m_inserted.end()) {
					force_rebuild();
				}
				else {
					m_inserted.erase(it);
				}
			}
		}

		void register_rename(const CachedTy& old_value, const CachedTy& new_value) {
			if (auto it = m_inserted.find(old_value);
				it != m_inserted.end()) {
				auto node{ m_inserted.extract(it) };
				node.value() = new_value;
				m_inserted.insert(std::move(node));
			}
		}

		bool contains(const CachedTy& value) {
			return static_cast<bool>(m_inserted.count(value));						//�� C++20 � unordered_set ��� ������ contains()
		}

		void force_rebuild() {
			m_state.force_rebuild = true;
		}

		template <class WrapperTy, class InputIt, class Extractor>					//Extractor �� ������� ���� �������, �.�. ����� ����������
		void synchronize_helper(													//WrapperTy - ��� �������� ���������, ���������� � ��������� [first, last)
			xml::container_t* dst,
			InputIt first,
			InputIt last,
			size_t range_length,
			Extractor extractor
		) {
			static_assert(std::is_base_of_v<XmlWrapper, WrapperTy>, "WrapperTy must be derived from XMLWrapper");
			if (m_state.force_rebuild) {
				rebuild_tree<WrapperTy>(dst, first, last, range_length, extractor);
			}
			else {																			//������� ������� ������ ����� ���� �� �������, 
				sync_and_update_size<WrapperTy>(dst, first, last, range_length, extractor);	//������ ��������� ����� ������ ������� ����������
			}																				//���� dst->size() == range_length (��� ����� ����������� � ��������� �����),
			reset_state();																	//���������� ������ ������������ ����� ���������
		}
		void reset_state() {
			m_inserted.clear();
			m_state = {};
		}

		template <class WrapperTy, class InputIt, class Extractor>
		void rebuild_tree(
			xml::container_t* dst,
			InputIt first,
			InputIt last,
			size_t range_length,
			Extractor extractor
		) {
			xml::container_t buffer;
			collect_children<WrapperTy>(
				std::addressof(buffer),										//������������� ����� ���������� � �������� ���������
				first, last, range_length,
				extractor
			);
			*dst = std::move(buffer);
		}

		template <class WrapperTy, class InputIt, class Extractor>
		void collect_children(
			xml::container_t* dst,
			InputIt first,
			InputIt last,
			size_t range_length,
			Extractor extractor
		) {
			dst->reserve(dst->size() + range_length);						//���� dst �� ����, ����� ������� reserve(), �.�. �������� ����������� � �����
			for (; first != last; ++first) {
				auto& wrapper{ extractor(*first) };
				if (wrapper.IsObserver()) {									//���� ���� - �����������, �������� �������� 
					wrapper = MoveFrom<WrapperTy>(wrapper);					//� ������������ � ������������ ������������� unique_ptr �� �������������� ���������
				}															//���������� ��� ���� � �������� ��� ���� �� ��������
				dst->push_back(extractor(*first).BuildXmlTree());			
			}																											
		}

		template <class WrapperTy, class InputIt, class Extractor>				//��������������� ������ ����� ����������� ��������
		void sync_and_update_size(
			xml::container_t* dst,
			InputIt first,
			InputIt last,
			size_t range_length,
			Extractor extractor
		) {					
			first = synchronize_range(first, std::min(dst->size(), range_length), extractor); //������������� ������, �� ��������� ������ XML-���������	
			if (range_length <= dst->size()) {
				dst->resize(range_length);
			}
			else {															
				dst->reserve(range_length);
				collect_children<WrapperTy>(
					dst,												
					first, last, range_length - dst->size(),								//first ��� ������� �� dst.size() �������		
					extractor	
				);
			}
		}

		template <class InputIt, class Extractor>								
		InputIt synchronize_range(															
			InputIt range_begin,											
			size_t range_length,													//���������� last �� ����� ������ - ����� ������ ������ ��������� � ��� �����
			Extractor&& extractor											
		) {
			while(range_length--) {
				extractor(*range_begin++).Synchronize();
			}
			return range_begin;
		}
	protected:
		title_cache_t m_inserted;
		State m_state;
	};


	enum class RenameResult {														//��������� ������� ������� ��� ������������� ��� ����������						
		Success,
		IsDuplicate,
		NothingChanged
	};

	class Department : public XmlContainerWrapper<FullNameRef, FullNameHasher> {
	public:
		using salary_t = Employee::salary_t;
		using workgroup_t = std::map<FullNameRef, Employee>;
		using employee_it = workgroup_t::iterator;									//�������������, ��� ��������� �� ����� �������������� ��� ����� ���������
		using employee_view_it = workgroup_t::const_iterator;						//� Employee �� ���������� Department, ����� ������� Extract.../Erase...
		using employee_range = Range<employee_it>;
		using employee_view_range = Range<employee_view_it>;
	private:
		enum class FullNameField {													//��� ���������� ���������� ����� ����� ���������
			Surname,
			Name,
			MiddleName
		};
	public:
		Department() = default;
		Department(xml::Node*);

		Department& Reset() override;
		Department& Synchronize() override;

		string_ref GetName() const;
		Department& SetName(std::string new_name) noexcept;
		
		RenameResult ChangeEmployeeSurname(const FullNameRef& old_name, std::string new_surname);			//�.�. ����� ����� ��� ������� ����� ������� � m_workgroup, 
		RenameResult ChangeEmployeeName(const FullNameRef& old_name, std::string new_name);				//��� ������ ����������� ������ ����� ��������� �������������!
		RenameResult ChangeEmployeeMiddleName(const FullNameRef& old_name, std::string new_middle_name);
		
		Department& UpdateEmployeeSalary(const FullNameRef& full_name, size_t new_salary);			//��� ����� ���������� ���������� ��������� �������� ����������� 
		size_t EmployeeCount() const noexcept;														//����� ��������� �������������
		double AverageSalary() const noexcept;

		bool Containts(const FullNameRef& name) const noexcept;

		Employee& at(const FullNameRef& name);
		const Employee& at(const FullNameRef& name) const;

		employee_it Find(const FullNameRef& name);
		employee_view_it Find(const FullNameRef& name) const;

		employee_range GetEmployees() noexcept;
		employee_view_range GetEmployees() const noexcept;

		employee_it InsertEmployee(Employee employee);

		employee_it EraseEmployee(employee_it employee);
		bool EraseEmployee(const FullNameRef& name);
		Employee ExtractEmployee(employee_it employee);
		Employee ExtractEmployee(const FullNameRef& name);

		Department& SetWorkgroup(workgroup_t new_workgroup);
	protected:
		friend class DepartmentBuilder;
		Department(xml::node_holder ready_node);

		Type get_type() const noexcept override;
		Department& update_dependencies() override;
		Department& take_dependencies(XmlWrapper& other) override;

		RenameResult employee_rename_helper(const FullNameRef& old_name, std::string&& value, FullNameField field);	//�� ��������� ������������ ���� �������� ����������																			

		void recalc_summary_salary_after_recruitment(size_t new_employee_salary);
		void recalc_summary_salary_before_dismissal(size_t former_employee_salary);

		static xml::Node* try_get_staff(xml::Node&);								//��������� ��������� �� ���� <employments> ������ <department>
		static workgroup_t collect_employees(xml::Node&);
		static salary_t calc_summary_salary(const workgroup_t& workgroup);
	protected:
		workgroup_t m_workgroup;
		salary_t m_summary_salary{ 0 };					//����� ������� ��������� � ��������� �������, ��� ������� ������� � ����������� �����������
	};

	class Company : public XmlContainerWrapper<std::string_view> {
	public: 
		using subdivision_t = std::list<Department>;		
		using department_view_it = subdivision_t::const_iterator;
		using department_view_range = Range<department_view_it>;
		using department_it = subdivision_t::iterator;
		using department_range = Range<department_it>;
	private:
		using subdivision_map_t = std::unordered_map<std::string_view, department_it>;
		using internal_department_it = subdivision_map_t::iterator;
	public:
		Company() = default;
		Company(xml::Node*);

		Company& Reset() override;
		Company& Synchronize() override;

		RenameResult RenameDepartment(const std::string& department, std::string new_name);
		department_range GetDepartments() noexcept;
		size_t DepartmentCount() const noexcept;

		bool Containts(const std::string& name) const noexcept;

		Department& at(const std::string& name);
		const Department& at(const std::string& name) const;

		department_it Find(const std::string& name);
		department_view_it Find(const std::string& name) const;

		department_view_range GetDepartments() const noexcept;
		department_it AddDepartment(Department new_department);

		department_it InsertDepartment(const std::string& before, Department new_department);
		department_it InsertDepartment(department_it before, Department new_department);

		const Department* TryGetNext(const std::string& department) const noexcept;
		const Department* TryGetNext(department_it department) const noexcept;

		department_it EraseDepartment(department_it department);
		bool EraseDepartment(const std::string& name);

		Department ExtractDepartment(department_it department);
		Department ExtractDepartment(const std::string& name);

		Company& SetSubdivision(subdivision_t new_subdivision);
		Company& ClearSubdivision() noexcept;
	protected:
		friend class CompanyBuilder;
		Company(xml::node_holder ready_node);

		Type get_type() const noexcept override;
		Company& update_dependencies() override;
		Company& take_dependencies(XmlWrapper& other) override;

		department_it erase_from_subdivision(department_it department);			//������� �� m_subdivision ��� �������� ������� � m_subdivision_map
		Department extract_from_subdivision(department_it department);			//��������� �� m_subdivision ��� �������� ������� � m_subdivision_map

		static void insert_to_subdivision_map(subdivision_map_t* map, department_it it);
		static subdivision_t collect_departaments(xml::Node&);
		static subdivision_map_t make_subdivision_map(subdivision_t& list);

		static void throw_non_existent_department(const std::string& name);
	protected:
		subdivision_t m_subdivision;
		subdivision_map_t m_subdivision_map;
	};

}
