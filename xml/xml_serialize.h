#pragma once
#include "xml.h"

#include <iostream>

namespace xml {
class Indent {
 public:
  Indent() = default;
  Indent(size_t step) : m_step{step} {}
  virtual void WriteIndents(std::ostream& output, size_t count) const = 0;

 protected:
  template <typename Filler>
  void fill(std::ostream& output, Filler filler, size_t count) const {
    for (size_t i = 0; i < count * m_step; ++i) {
      output << filler;
    }
  }

 protected:
  size_t m_step{1};
};

using indent_holder = std::unique_ptr<Indent>;

class Tabulation : public Indent {
 public:
  using Indent::Indent;
  void WriteIndents(std::ostream& output, size_t count) const final;

 private:
};

struct Space : public Indent {
 public:
  using Indent::Indent;
  void WriteIndents(std::ostream& output, size_t count) const final;
};

class Writer {
 public:
  Writer(std::ostream& output) noexcept;
  Writer& SetIndentType(indent_holder new_indent) noexcept;
  Writer& SetBasicIndentCount(std::optional<size_t> indents_count) noexcept;
  Writer& Save(const Document& doc);

  bool Fail() const noexcept;
  explicit operator bool() const noexcept;

 private:
  void print_indents(std::optional<size_t> indents_count);
  void print_attributes(const Node& node);
  void print_quoted(const text_t& str);
  void print_service_node_header(const Node& node);
  void print_node_header(const Node& node);
  void print_node_limiter(const Node& node);

  std::string read_service_block(service_block_t block);

  void serialize_node(const Node& node, std::optional<size_t> indents_count);
  void serialize_container(const Node& node,
                           std::optional<size_t> indents_count);
  void serialize_container_helper(const container_t& container,
                                  std::optional<size_t> indents_count);

  static std::optional<size_t> increment_indents_count(
      std::optional<size_t> indents_count);

  std::ostream& get_stream();

 private:
  std::ostream* m_output;
  std::optional<size_t> m_base_indents_count;
  indent_holder m_indent;
};
}  // namespace xml