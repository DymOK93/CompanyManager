#include "xml_parse.h"
#include "xml_exceptions.h"
using namespace std;

namespace xml {
	Reader::Reader(istream& input) noexcept
		: m_input(addressof(input))
	{
	}

	Document Reader::Load(allocator_holder external_alloc) {
		m_tree_allocator = move(external_alloc);				//Может использоваться внешнийс аллокатор
		return m_builder
			.SetDeclaration(load_node())						//Считываем XML-декларацию
			.SetRoot(load_node())								//Считываем DOM-дерево
			.SetAllocator(move(m_tree_allocator))				//Документ владеет аллокатором
			.Assemble();										//Собираем документ
	}

	bool Reader::Fail() const noexcept {
		return m_input->fail();
	}

	Reader::operator bool() const noexcept {
		return !Fail();
	}

	node_holder Reader::load_node() {
		left_strip();
		if (get_next() != '<') {
			throw parse_error("Ill-formed XML node");
		}
		auto first_service_block{ load_service_block() };
		text_t name{
			load_line(
					[](char ch) {
						return ch != '>' && !isspace(ch);  
				}
			)
		};

		node_holder node(
			NodeBuilder()
			.SetName(move(name))
			.SetAttributes(load_attributes())
			.SetAllocator(m_tree_allocator)
			.Assemble()
		);

		left_strip();
		load_node_value(*node, first_service_block);
		return node;
	}

	void Reader::load_node_value(Node& node, optional<int64_t> first_service_block) {
		auto second_servie_block{ load_service_block() };
		load_text('>');
		bool new_line{ left_strip() };

		if (first_service_block) {
			node.SetService(
				make_pair(
					*first_service_block,
					second_servie_block ? optional<int64_t> (*second_servie_block) : nullopt
				)
			);
		}
		else if (peek_next() == '<' && new_line) {	//</...> без перевода строки означает конец пустого текстового блока							
			node.SetContainer(load_children());
		}
		else {										
			node.SetText(load_text('<'));
			close_line();
		}
	}
	container_t Reader::load_children() {
		container_t children;

		while (get_stream()) {
			get_next();
			if (peek_next() == '/') {
				close_line();
				break;
			}
			else {
				unget_character();
				children.push_back(load_node());
			}
			left_strip();
		}
		return children;
	}

	text_t Reader::load_text(char limiter) {
		text_t buffer;
		getline(get_stream(), buffer, limiter);
		return buffer;
	}

	property_map Reader::load_attributes() {
		property_map attrs;
		left_strip();
		while (!ispunct(peek_next())) {
			attrs.insert(load_attribute());
			left_strip();
		}
		return attrs;
	}

	attribute_holder Reader::load_attribute() {
		text_t name{ load_text('=') };
		left_strip();
		if (get_next() != '\"') {											//Извлекаем кавычку
			throw parse_error("Attribute value must be quoted");
		}
		return { move(name), load_quoted_line() };
	}

	text_t Reader::load_quoted_line() {
		return load_text('\"');
	}

	optional<service_block_t> Reader::load_service_block() {
		string buffer{
				load_line(
					[](byte symbol) {
					return ispunct(symbol) && symbol != '<' && symbol != '>';
				})
		};
		if (buffer.empty()) {
			return nullopt;
		}
		service_block_t result;
		buffer.resize(sizeof(service_block_t), static_cast<char>(0));													//Дополняем нулям для корретного считывания в дальнейшем
		buffer.copy(reinterpret_cast<char*>(addressof(result)), sizeof(service_block_t), 0);		//Копируем содержимое строки
		return result;																					
	}

	void Reader::unget_character() {
		m_input->unget();
	}

	byte Reader::get_next() {
		return static_cast<byte>(m_input->get());
	}

	byte Reader::peek_next() {
		return static_cast<byte>(m_input->peek());
	}

	bool Reader::left_strip() {
		bool new_line{ false };
		while (get_stream() && isspace(peek_next())) {
			if (!new_line) {
				new_line = (peek_next() == '\n');
			}
			get_next();
		}
		return new_line;
	}

	void Reader::close_line() {
		load_text('\n');
	}

	istream& Reader::get_stream() {
		return *m_input;
	}
}