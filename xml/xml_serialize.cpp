#include "xml_serialize.h"
using namespace std;

namespace xml {
	Writer::Writer(ostream& output) noexcept
		: m_output(addressof(output))
	{
	}

	Writer& Writer::SetIndentType(indent_holder new_indent) noexcept {
		m_indent = move(new_indent);
		return *this;
	}

	Writer& Writer::SetBasicIndentCount(optional<size_t> indents_count) noexcept {
		m_base_indents_count = indents_count;
		return *this;
	}

	Writer& Writer::Save(const Document& doc) {
		serialize_node(doc.GetDeclaration(), m_base_indents_count);
		serialize_node(doc.GetRoot(), m_base_indents_count);
		return *this;
	}

	bool Writer::Fail() const noexcept {
		return m_output->fail();
	}

	Writer::operator bool() const noexcept {
		return !Fail();
	}

	void Writer::print_indents(optional<size_t> indents_count) {
		if (m_indent && indents_count) {
			m_indent->WriteIndents(get_stream(), *indents_count);					//В классе хранится начальное число отступов
		}																			//При спуске вниз по дереву оно увеличивается
	}

	void Writer::print_attributes(const Node& node) {
		for (const auto& [name, value] : node.GetAttributes()) {
			get_stream() << ' ' << name << '=';
			Writer::print_quoted(value);
		}
	}

	void Writer::print_quoted(const text_t& str) {
		get_stream() << '\"' << str << '\"';
	}

	void Writer::print_service_node_header(const Node& node) {
		get_stream() << '<' << read_service_block(node.AsService().first) << node.GetName();
		print_attributes(node);
		if (node.AsService().second) {
			get_stream() << read_service_block(*node.AsService().second);
		}
		get_stream() << '>';
	}

	void Writer::print_node_header(const Node& node) {
		get_stream() << '<' << node.GetName();
		Writer::print_attributes(node);
		get_stream() << '>';
	}

	void Writer::print_node_limiter(const Node& node) {
		get_stream() << "</" << node.GetName() << '>';
	}

	string Writer::read_service_block(service_block_t block) {
		std::string buffer(													//Копируем символы в буферную строку
			reinterpret_cast<const char*>(
				addressof(block)
				),
			sizeof(service_block_t)
		);
		buffer.resize(buffer.find_first_of(static_cast<char>(0)));
		return buffer;
	}

	void Writer::serialize_node(const Node& node, optional<size_t> indents_count) {
		print_indents(indents_count);
		if (node.GetType() == Node::Type::Service) {
			Writer::print_service_node_header(node);
		}
		else {
			Writer::print_node_header(node);
		}

		switch (node.GetType()) {
			case Node::Type::Tree: {
			get_stream() << '\n';
			serialize_container(node, increment_indents_count(indents_count));
			print_indents(indents_count);
		} break;
		case Node::Type::Element: get_stream() << node.AsText(); break;
		default: break;
		};

		if (node.GetType() != Node::Type::Service) {
			print_node_limiter(node);
		}
		get_stream() << '\n';
	}

	void Writer::serialize_container(const Node& node, std::optional<size_t> indents_count) {
		serialize_container_helper(node.AsContainer(), indents_count);
	}

	void Writer::serialize_container_helper(const container_t& container, optional<size_t> indents_count) {
		for (const auto& node : container) {
			serialize_node(*node, indents_count);
		}
	}

	optional<size_t> Writer::increment_indents_count(optional<size_t> indents_count) {
		return indents_count ? optional<size_t> (*indents_count + 1) : nullopt;
	}

	ostream& Writer::get_stream() {
		return *m_output;
	}

	void Tabulation::WriteIndents(std::ostream& output, size_t count) const {
		fill(output, '\t', count);
	}

	void Space::WriteIndents(std::ostream& output, size_t count) const {
		fill(output, ' ', count);
	}
}