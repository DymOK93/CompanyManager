#pragma once
#include "command_interface.h"

#include <memory>
#include <utility>
#include <list>
#include <deque>
#include <string>
#include <functional>
#include <stdexcept>

namespace task {
	enum class ResultType {
		Success,
		Fail,
		EmptyQueue
	};
}

template <class TargetTy>
class TaskManager {
public:
	static constexpr size_t DEFAULT_QUEUE_CAPACITY{ 20 };						//Стандартная емкость очереди команд

	using task_t = command::ICommand<TargetTy>;
	using task_holder = command::command_holder<TargetTy>;
	using command_queue = std::list<task_holder>;
	using error_log_t = std::deque<std::string>;								//deque эффективнее при заранее неизвестном числе добавленных в конец элементов
	using ResultType = task::ResultType;	
	struct Result {
		std::any value;
		ResultType type;
	};
protected:
	using internal_result_t = std::pair<std::any, bool>;
public:
	TaskManager() = default;
	TaskManager(size_t queue_capacity)
		: m_queue_capacity(queue_capacity)
	{
	}

	/******************************************************
	Для возможности синхронизации нескольких TM необходимо
	сначала добавлять команды в очередь и лишь после 
	пытаться их обработать. В противном случае возможна 
	ситуация:
	<TM1>: 
		cancel queue before process: a5-a4-a3-a2-a1
		process: ax, success
		cancel queue after process: a4-a3-a2-a1-ax
	<TM2>: 
		cancel queue before process: b5-b4-b3-b2-b1
		process: bx, fail
		cancel queue after process: b5-b4-b3-b2-b1
	<Вызов cancel у TM1 для освобождения ресурсов>
	<TM1>:
		cancel queue after cancel: (empty)-a4-a3-a2-a1
	Результат: b5 - sync failed
	*******************************************************/

	Result Process(task_holder&& new_command) {
		task_t* command;
		if (!m_queue_capacity) {						//Если сохранение в очередь для отмены отключено
			command = new_command.get();
		}
		else {
			update_cancel_queue(std::move(new_command));
			command = get_last_command(m_cancel);
		}
		auto [result, success] {try_to_execute(command)};
		if (!success) {
			if (m_queue_capacity > 0) {
				extract_last_command(m_cancel);
			}	
			return make_failed_operation_result();		
		}
		return make_successful_operation_result(std::move(result));
	}

	Result Cancel() {					
		if (m_cancel.empty()) {												//Попытка проверить наличие команды в очереди
			return make_empty_queue_operation_result();			
		}
		update_repeat_queue(												//Выброс исключения при пустой очереди команд
			extract_last_command(m_cancel)									//Строго для внутреннего использования
		);
		auto [result, success] {						//Команду сначала необходимо поместить в очередь повтора, чтобы избежать рассинхронизации
			try_to_cancel(
				get_last_command(m_repeat)
			)
		};
		if (!success) {
			extract_last_command(m_repeat);
			return make_failed_operation_result();
		}	
		return make_successful_operation_result(move(result));
	}

	Result CancelWithoutQueueing() {									//Отмена без сохранения в очередь repeat (для нештатных ситуаций)
		if (m_cancel.empty()) {												
			return make_empty_queue_operation_result();
		}
		task_holder task{ extract_last_command(m_cancel) };

		auto [result, success] {try_to_cancel(task.get())};
		if (!success) {
			return make_failed_operation_result();
		}
		return make_successful_operation_result(move(result));
	}

	Result Repeat() {
		if (m_repeat.empty()) {
			return make_empty_queue_operation_result();
		}
		update_cancel_queue(
			extract_last_command(m_repeat)
		);
		auto [result, success] {						//Команду сначала необходимо поместить в очередь отмены, чтобы избежать рассинхронизации
			try_to_execute(
				get_last_command(m_cancel)
			)
		};
		if (!success) {
			extract_last_command(m_cancel);
			return make_failed_operation_result();
		}																	
		return make_successful_operation_result(move(result));
	}


	Result RepeatWithoutQueueing() {									//Повтор без сохранения в очередь cancel (для нештатных ситуаций)
		if (m_cancel.empty()) {
			return make_empty_queue_operation_result();
		}
		task_holder task{ extract_last_command(m_repeat) };

		auto [result, success] {try_to_execute(task.get())};
		if (!success) {
			return make_failed_operation_result();
		}
		return make_successful_operation_result(move(result));
	}

	size_t CanBeCanceled() const noexcept {
		return m_cancel.size();
	}

	size_t CanBeRepeated() const noexcept {
		return m_repeat.size();
	}

	std::optional<command::Type> GetLastProcessedType() const noexcept {					//Получение типа последней успешно обработанной команды
		task_t* command{ get_last_command(m_cancel) };
		if (!command) {
			return std::nullopt;
		}
		return command->GetType();
	}

	std::optional<command::Type> GetLastCanceledType() const noexcept {						//Получение типа последней отменённой команды
		task_t* command{ get_last_command(m_repeat) };
		if (!command) {
			return std::nullopt;
		}
		return command->GetType();
	}

	std::optional<command::Purpose> GetLastProcessedPurpose() const noexcept {				//Получение категории последней успешно обработанной команды
		task_t* command{ get_last_command(m_cancel) };
		if (!command) {
			return std::nullopt;
		}
		return command->GetPurpose();
	}

	std::optional<command::Purpose> GetLastCanceledPurpose() const noexcept {				//Получение категории последней отменённой команды
		task_t* command{ get_last_command(m_repeat) };	
		if (!command) {
			return std::nullopt;
		}
		return command->GetPurpose();
	}

	void ResetCancelQueue() noexcept{
		m_cancel.clear();
	}

	void ResetRepeatQueue() noexcept {
		m_repeat.clear();
	}

	void ResetQueues() noexcept {
		m_cancel.clear();
		m_repeat.clear();
	}

	void ResetAll() noexcept {
        ResetQueues();
		m_error_log.clear();
	}

	const error_log_t& GetErrorLog() const noexcept {
		return m_error_log;
	}

	bool Fail() const noexcept {
		return !m_error_log.empty();
	}

	explicit operator bool() const noexcept {
		return !Fail();
	}
protected:
	void update_cancel_queue(task_holder&& command) {
		m_cancel.push_back(move(command));
		if (CanBeCanceled() > m_queue_capacity) {
			m_cancel.pop_front();
		}
	}

	void update_repeat_queue(task_holder&& command) {
		m_repeat.push_back(move(command));
		if (CanBeRepeated() > m_queue_capacity) {
			m_repeat.pop_front();
		}
	}

	internal_result_t try_to_execute(task_t* command) {
		return try_to_process(&task_t::Execute, command);
	}

	internal_result_t try_to_cancel(task_t* command) {
		return try_to_process(&task_t::Cancel, command);
	}

	static task_t* get_last_command(const command_queue& queue) noexcept {
		return queue.empty() ? nullptr : queue.back().get();
	}

	static task_holder extract_last_command(command_queue& queue) {
		if (queue.empty()) {
			throw std::out_of_range("Command queue is empty");
		}
		auto extracted_command = move(queue.back());
		queue.pop_back();
		return extracted_command;
	}

	static Result make_empty_queue_operation_result() noexcept {
		return { std::any{}, ResultType::EmptyQueue };
	}

	static Result make_failed_operation_result() noexcept {
		return { std::any{}, ResultType::Fail };
	}

	static Result make_successful_operation_result(std::any&& value) noexcept {
		return { move(value), ResultType::Success };							//Принимаем по r-value ref - экономим один вызов move c-tor
	}

private:
	internal_result_t try_to_process(std::any(task_t::* method)(), task_t* command) {
		size_t error_count_before_op{ m_error_log.size() };
		internal_result_t result;
		try {
            result.first = std::invoke(method, command);
		}
		catch (const std::exception& exc) {
			log_error_message(exc.what());
		}
		catch (...) {
			log_error_message("Unknown error");
		}
		result.second = m_error_log.size() == error_count_before_op;
		return result;
	}

	void log_error_message(std::string msg) {
		m_error_log.push_back(move(msg));
	}

private:
	command_queue m_cancel, m_repeat;
	error_log_t m_error_log;
	size_t m_queue_capacity{ DEFAULT_QUEUE_CAPACITY };
};


