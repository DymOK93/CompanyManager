#pragma once
#include "object_pool.h"

#include <any>
#include <optional>
#include <variant>
#include <memory>
#include <utility>
#include <stdexcept>
#include <algorithm>
#include <type_traits>

/***********************************************************
Шаблон ICommand реализует интерфейс системы команд с 
возможность выполнения и отмены. 
Ссылка на целевой объект типа TargetTy передается в конструктор, 
чтобы предотвратить вызов Cancel() для другого объекта
************************************************************/

namespace command {
	enum class Type {							//Список команд для определения типа вызовом GetType()
	/*FileIO*/
		File_GetPath,
		File_SetPath,
		File_CreateDocument,
		File_Load,
		File_Save,
		File_CheckLoaded,
		File_CheckSaved,
		File_Reset,

	/*ModifyXml and TreeModel(derived from QAbstractItemModel)*/	
		Xml_AddDepartment,
		Xml_InsertDepartment,
		View_InsertDepartment,
		Xml_InsertEmployee,
		View_InsertEmployee,

		Xml_RemoveDepartment,	
		View_RemoveDepartment,
		Xml_RemoveEmployee,
		View_RemoveEmployee,

		Xml_RenameDepartment,
		View_RenameDepartment,
		Xml_ChangeEmployeeSurname,
		Xml_ChangeEmployeeName,
		Xml_ChangeEmployeeMiddleName,
		Xml_ChangeEmployeeFunction,
		View_ChangeEmployeeFullName,
		Xml_UpdateEmployeeSalary,	
	};

	enum class Purpose {	//Объединение команд различных типов в более крупные систематические единицы для регулирования порядка выполнения и отмены
		FileIO = 7,				
		Insert = 12,			
		Remove = 16,		
		EditFields = 24
	};

	/***********************************************************
	Последовательность выполнения операций Execute() и Cancel() 
	для команд различных категорий, применяемых попарно к
	XMLWrapperTree (далее - xml) и TreeModelView (далее - view)

	Insert: 
		Execute (xml) -> Execute (view)
		Cancel - unspecified
	Remove:
		Execute (view) -> Execute (xml)
		Cancel (xml) -> cancel (view)
	EditFields:
		Execute (xml) -> Execute (view)
		Cancel - unspecified
	************************************************************/

	template <class TargetTy>
	class ICommand {
	private:
		using purpose_t = std::underlying_type_t<Purpose>;
	public:
		ICommand(TargetTy& target) noexcept
			: m_target(std::addressof(target))
		{
		}
		virtual ~ICommand() = default;

		virtual std::any Execute() = 0;
		virtual std::any Cancel() = 0;
		virtual Type GetType() const noexcept = 0;
		Purpose GetPurpose() const noexcept {
			return static_cast<Purpose>(
				*std::upper_bound(
					command_purpose.begin(),
					command_purpose.end(),
					static_cast<purpose_t>(GetType())
				)
			);
		}
	protected:
		TargetTy& get_target() const noexcept {
			return *m_target;
		}
		static std::any make_default_value() noexcept {
			return std::any{};
		}
	private:
		static constexpr std::array<std::underlying_type_t<Purpose>, 4> command_purpose{
			static_cast<purpose_t>(Purpose::FileIO),
			static_cast<purpose_t>(Purpose::Insert),
			static_cast<purpose_t>(Purpose::Remove),
			static_cast<purpose_t>(Purpose::EditFields)
		};

		enum class State {
			Default,
			Executed,
			Cancelled
		};
		void register_execute() {
			if (m_state == State::Executed) {											//Повторное выполнение команды запрещено
				throw_impossible_to_execute();
			}
			m_state = State::Executed;
		}
		void register_cancel() {													//Отмена невыполненной команды недопустима
			if (m_state != State::Executed) {
				throw_impossible_to_execute();
			}
			m_state = State::Cancelled;
		}
		static void throw_impossible_to_execute() {
			throw std::logic_error("Unable to execute already executed command");
		}
		static void throw_impossible_to_cancel() {
			throw std::logic_error("Unable to cancel unexecuted command");
		}
	private:
		TargetTy* m_target;
		State m_state{ State::Default };
	};

	template <class TargetTy>
	using command_holder = object_holder<ICommand<TargetTy>>;

	/***********************************************************
	Промежуточный класс, объединяющий возможности
	ObjectPool и интерфейс ICommand
	***********************************************************/
	template <class ConcreteCommand, class TargetTy>
	class AllocatedCommand : public ObjectPool<ICommand<TargetTy>, ConcreteCommand>, public ICommand<TargetTy> {
	public:
		using MyAllocationBase = ObjectPool<ICommand<TargetTy>, ConcreteCommand>;
		using MyCommandBase = ICommand<TargetTy>;
		using command_holder = command_holder<TargetTy>;
	public:
		using MyCommandBase::MyCommandBase;
		static command_holder make_instance(TargetTy& target) {						//Для классов команд без дополнительных аргументов конструктора
			return MyAllocationBase::allocate_instance(target);
		}
	};
}

