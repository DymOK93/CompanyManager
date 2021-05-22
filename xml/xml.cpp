#include "xml.h"
using namespace std;

namespace xml {
Node::Node(Header header, allocator_weak alloc)
    : m_header(move(header)), m_allocator(move(alloc)) {}

Node::Type Node::GetType() const noexcept {
  return static_cast<Type>(m_body.index());
}

void Node::ChangeName(text_t new_name) noexcept {
  m_header.name = move(new_name);
}

const text_t& Node::GetName() const noexcept {
  return m_header.name;
}

void Node::AddAttribute(text_t name, text_t value) {
  m_header.attributes.emplace(move(name), move(value));
}

void Node::AddAttribute(attribute_holder attr) {
  m_header.attributes.insert(move(attr));
}

size_t Node::AttributesCount() const noexcept {
  return m_header.attributes.size();
}

Node::attribute_range Node::GetAttributes() const noexcept {
  return {m_header.attributes.begin(), m_header.attributes.end()};
}

text_t& Node::operator[](const text_t& attr_name) {
  return m_header.attributes[attr_name];
}

const text_t& Node::at(const text_t& attr_name) const {
  return m_header.attributes.at(attr_name);
}

void Node::Reset() {
  m_body = empty_t{};
}

service_t& Node::AsService() {
  return get<service_t>(m_body);
}

const service_t& Node::AsService() const {
  return get<service_t>(m_body);
}

void Node::SetService(service_t new_nalue) noexcept {
  m_body = new_nalue;
}

void Node::ResetAsService() {
  m_body = service_t{};
}

text_t& Node::AsText() {
  return get<text_t>(m_body);
}

const text_t& Node::AsText() const {
  return get<text_t>(m_body);
}

void Node::SetText(text_t new_nalue) noexcept {
  m_body = move(new_nalue);
}

void Node::ResetAsText() {
  m_body = ""s;
}

container_t& Node::AsContainer() {
  return get<container_t>(m_body);
}

const container_t& Node::AsContainer() const {
  return get<container_t>(m_body);
}

void Node::SetContainer(container_t new_nalue) noexcept {
  m_body = move(new_nalue);
}

void Node::ResetAsContainer() {
  m_body = container_t{};
}

void Node::Clear() noexcept {
  m_body = service_t{};
}

allocator_holder Node::GetAllocator() const noexcept {
  return m_allocator.lock();
}

Document::Document(node_holder declaration,
                   node_holder root,
                   allocator_holder alloc)
    : m_declaration(move(declaration)),
      m_root(move(root)),
      m_tree_allocator(move(alloc)) {}

Node& Document::GetRoot() noexcept {
  return *m_root;
}

const Node& Document::GetDeclaration() const noexcept {
  return *m_declaration;
}

const Node& Document::GetRoot() const noexcept {
  return *m_root;
}

allocator_holder Document::GetAllocator() const noexcept {
  return m_tree_allocator;
}

DocumentBuilder& DocumentBuilder::SetDeclaration(node_holder new_declaration) {
  m_declaration = move(new_declaration);
  return *this;
}

DocumentBuilder& DocumentBuilder::SetRoot(node_holder new_root) {
  m_root = move(new_root);
  return *this;
}

DocumentBuilder& DocumentBuilder::SetAllocator(
    allocator_holder external_alloc) {
  m_tree_allocator = move(external_alloc);
  return *this;
}

Document DocumentBuilder::Assemble() {
  if (!m_declaration || !m_root) {
    throw document_builder_error("Not enough parameters");
  }
  if (!m_tree_allocator) {
    throw document_builder_error("The document must own the tree allocator");
  }
  return Document(move(*m_declaration), move(*m_root), move(m_tree_allocator));
}
}  // namespace xml