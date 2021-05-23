#include "command_interface.h"
#include "engine.h"

namespace command {
namespace file_io {
template <class ConcreteCommand>
class CommandBase : public AllocatedCommand<ConcreteCommand, CompanyManager> {
 public:
  using MyBase = AllocatedCommand<ConcreteCommand, CompanyManager>;

 public:
  using MyBase::MyBase;
  std::any Cancel()
      override {  //Дочерние классы все ещё могут переопределить операцию отмены
    throw std::logic_error("File IO operations can't be cancelled");
  }
};

class CM_ENGINE_API GetPath : public CommandBase<GetPath> {
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
  CM_ENGINE_API SetPath(CompanyManager& cm, std::string path) noexcept;
  CM_ENGINE_API std::any Execute() override;
  CM_ENGINE_API Type GetType() const noexcept override;
  CM_ENGINE_API static command_holder make_instance(CompanyManager& cm,
                                                    std::string path);

 private:
  std::string m_path;
};

class CM_ENGINE_API CreateDocument : public CommandBase<CreateDocument> {
 public:
  using MyBase = CommandBase<CreateDocument>;

 public:
  using MyBase::MyBase;
  std::any Execute() override;
  Type GetType() const noexcept override;
};

class CM_ENGINE_API Load : public CommandBase<Load> {
 public:
  using MyBase = CommandBase<Load>;

 public:
  using MyBase::MyBase;
  std::any Execute() override;
  Type GetType() const noexcept override;
};

class CM_ENGINE_API Save : public CommandBase<Save> {
 public:
  using MyBase = CommandBase<Save>;

 public:
  using MyBase::MyBase;
  std::any Execute() override;
  Type GetType() const noexcept override;
};

class CM_ENGINE_API CheckLoaded : public CommandBase<CheckLoaded> {
 public:
  using MyBase = CommandBase<CheckLoaded>;

 public:
  using MyBase::MyBase;
  std::any Execute() override;
  Type GetType() const noexcept override;
};

class CM_ENGINE_API CheckSaved : public CommandBase<CheckSaved> {
 public:
  using MyBase = CommandBase<CheckSaved>;

 public:
  using MyBase::MyBase;
  std::any Execute() override;
  Type GetType() const noexcept override;
};

class CM_ENGINE_API Reset : public CommandBase<Reset> {
 public:
  using MyBase = CommandBase<Reset>;

 public:
  using MyBase::MyBase;
  std::any Execute() override;
  Type GetType() const noexcept override;
};
}  // namespace file_io
}  // namespace command
