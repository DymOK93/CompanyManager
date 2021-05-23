#include "file_io_command.h"
using namespace std;

namespace command {
namespace file_io {
/*GetPath*/
any GetPath::Execute() {
  return get_target().GetPath();
}

Type GetPath::GetType() const noexcept {
  return Type::File_GetPath;
}

/*SetPath*/
SetPath::SetPath(CompanyManager& cm, string path) noexcept
    : MyBase(cm), m_path{move(path)} {}

any SetPath::Execute() {
  get_target().SetPath(move(m_path));
  return make_default_value();
}

Type SetPath::GetType() const noexcept {
  return Type::File_SetPath;
}

SetPath::command_holder SetPath::make_instance(CompanyManager& cm,
                                               string path) {
  return allocate_instance(cm, std::move(path));
}

/*CreateDocument*/

any CreateDocument::Execute() {
  get_target().Create();
  return make_default_value();
}

Type CreateDocument::GetType() const noexcept {
  return Type::File_CreateDocument;
}

/*Load*/

any Load::Execute() {
  return get_target().Load();
}

Type Load::GetType() const noexcept {
  return Type::File_Load;
}

/*Save*/

any Save::Execute() {
  return get_target().Save();
}

Type Save::GetType() const noexcept {
  return Type::File_Save;
}

/*CheckLoaded*/

any CheckLoaded::Execute() {
  return get_target().IsLoaded();
}

Type CheckLoaded::GetType() const noexcept {
  return Type::File_CheckLoaded;
}

any CheckSaved::Execute() {
  return get_target().IsSaved();
}

Type CheckSaved::GetType() const noexcept {
  return Type::File_CheckSaved;
}

any Reset::Execute() {
  get_target().Reset();
  return make_default_value();
}

Type Reset::GetType() const noexcept {
  return Type::File_Reset;
}
}  // namespace file_io
}  // namespace command