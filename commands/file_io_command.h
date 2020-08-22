#include "command_interface.h"
#include "company_manager_engine.h"

namespace command {
	namespace file_io {
		template <class ConcreteCommand>
		class CommandBase : public AllocatedCommand<ConcreteCommand, CompanyManager> {
		public:
			using MyBase = AllocatedCommand<ConcreteCommand, CompanyManager>;
		public:
			using MyBase::MyBase;
			std::any Cancel() override {									//Дочерние классы все ещё могут переопределить операцию отмены
				throw std::logic_error("File IO operations can't be cancelled");
			}
		};

		class GetPath : public CommandBase<GetPath> {
		public:
			using MyBase = CommandBase<GetPath>;
		public:
			using MyBase::MyBase;
			std::any Execute() override;
			Type GetType() const noexcept override;
		};

		class SetPath : public CommandBase<SetPath> {
		public:
			using MyBase = CommandBase<SetPath>;
		public:
			SetPath(CompanyManager& cm, std::string path) noexcept;
			std::any Execute() override;
			Type GetType() const noexcept override;
			static command_holder make_instance(CompanyManager& cm, std::string path);
		private:
			std::string m_path;
		};

		class CreateDocument : public CommandBase<CreateDocument> {
		public:
			using MyBase = CommandBase<CreateDocument>;
		public:
			using MyBase::MyBase;
			std::any Execute() override;
			Type GetType() const noexcept override;
		};

		class Load : public CommandBase<Load> {
		public:
			using MyBase = CommandBase<Load>;
		public:
			using MyBase::MyBase;
			std::any Execute() override;
			Type GetType() const noexcept override;
		};

		class Save : public CommandBase<Save> {
		public:
			using MyBase = CommandBase<Save>;
		public:
			using MyBase::MyBase;
			std::any Execute() override;
			Type GetType() const noexcept override;
		};

		class CheckLoaded : public CommandBase<CheckLoaded> {
		public:
			using MyBase = CommandBase<CheckLoaded>;
		public:
			using MyBase::MyBase;
			std::any Execute() override;
			Type GetType() const noexcept override;
		};

		class CheckSaved : public CommandBase<CheckSaved> {
		public:
			using MyBase = CommandBase<CheckSaved>;
		public:
			using MyBase::MyBase;
			std::any Execute() override;
			Type GetType() const noexcept override;
		};

		class Reset : public CommandBase<Reset> {
		public:
			using MyBase = CommandBase<Reset>;
		public:
			using MyBase::MyBase;
			std::any Execute() override;
			Type GetType() const noexcept override;
		};
	}
}

