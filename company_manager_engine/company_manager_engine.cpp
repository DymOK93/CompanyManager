#include "company_manager_engine.h"
using namespace std;
using worker::file_operation::Result;

CompanyManager& CompanyManager::Create() {
  Reset();
  m_xml_tree = build_default_tree();
  update_stats_after_create();
  return *this;
}

Result CompanyManager::Load() {
  ifstream input;
  xml::Reader reader(input);

  auto loader{worker::file_operation::PipelineBuilder()
                  .CheckPath(m_file.current_path)
                  .OpenForReading(input, m_file.current_path)
                  .ReadXml(reader, m_xml_tree.m_document)
                  .Assemble()};

  Result result;
  loader->Process(result);
  if (result == Result::Success) {
    update_stats_after_load();
  }
  return result;
}

Result CompanyManager::Save() {
  ofstream output;
  xml::Writer writer(output);
  tune_xml_writer(writer);  //Установка количества отступов
  m_xml_tree.company.Synchronize();  //Вызывать BuildXmlTree не требуется, т.к.
                                     //company уже принадлежит документу

  auto saver{worker::file_operation::PipelineBuilder()
                 .CheckPath(m_file.current_path)
                 .OpenForWriting(output, m_file.current_path)
                 .WriteXml(writer, m_xml_tree.m_document)
                 .Assemble()};

  Result result;
  saver->Process(result);
  m_file.is_saved = true;
  return result;
}

bool CompanyManager::IsSaved() const noexcept {
  return m_file.is_saved;
}

bool CompanyManager::IsLoaded() const noexcept {
  return m_file.is_loaded;
}

CompanyManager::operator bool() const noexcept {
  return IsLoaded();
}

std::string_view CompanyManager::GetPath() const noexcept {
  return m_file.current_path;
}

CompanyManager& CompanyManager::SetPath(string path) noexcept {
  m_file.current_path = move(path);
  return *this;
}

CompanyManager& CompanyManager::Reset() noexcept {
  m_file = {};
  return *this;
}

const wrapper::Company& CompanyManager::Read() const {
  if (!IsLoaded()) {
    throw logic_error("XML document not found");
  }
  return m_xml_tree.company;
}
wrapper::Company& CompanyManager::Modify() {
  if (!IsLoaded()) {
    throw logic_error("XML document not found");
  }
  m_file.is_saved = false;
  return m_xml_tree.company;
}

xml::allocator_holder CompanyManager::GetAllocator() const {
  if (!IsLoaded()) {
    throw logic_error("XML document not found");
  }
  return m_xml_tree.company.GetAllocator();
}

bool CompanyManager::empty_path() const noexcept {
  return m_file.current_path.empty();
}

const string& CompanyManager::get_path() const noexcept {
  return m_file.current_path;
}

void CompanyManager::update_stats_after_create() {
  m_file.is_loaded = true;
  m_file.is_saved = false;  //Файл не создается вместе с документом
}

void CompanyManager::update_stats_after_load() {
  m_xml_tree.company = build_wrappers_tree(m_xml_tree.m_document);
  m_file.is_loaded = true;
}

CompanyManager::XmlTree CompanyManager::build_default_tree() {
  xml::allocator_holder tree_alloc{xml::MakeDefaultAllocator()};
  wrapper::Company company{
      wrapper::CompanyBuilder().SetAllocator(tree_alloc).Assemble()};
  xml::Document doc{xml::DocumentBuilder()
                        .SetDeclaration(make_xml_declaration(tree_alloc))
                        .SetRoot(company.BuildXmlTree())
                        .SetAllocator(tree_alloc)
                        .Assemble()};
  return {move(doc), move(company)};
}

xml::node_holder CompanyManager::make_xml_declaration(
    xml::allocator_holder alloc) {
  return xml::ServiceNodeBuilder()
      .SetAllocator(move(alloc))
      .AddServiceSymbols('?', '?')
      .SetAttribute(u8"version", u8"1.0")
      .SetAttribute(u8"encoding", u8"UTF-8")
      .Assemble();
}

wrapper::Company CompanyManager::build_wrappers_tree(xml::Document& doc) {
  return wrapper::Company(addressof(doc.GetRoot()));
}

void CompanyManager::tune_xml_writer(xml::Writer& writer) {
  writer.SetIndentType(
      make_unique<xml::Space>(3));  //Шаг одного отступа (3 пробела)
  writer.SetBasicIndentCount(0);
}
