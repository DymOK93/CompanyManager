#pragma once
#include "xml.h"
#include "xml_node_builders.h"
	
#include <iostream>	
#include <cctype>	//isspace


namespace xml {
	using byte = unsigned char;

	class Reader {
	public:
		Reader(std::istream& input) noexcept;
		Document Load(allocator_holder external_alloc = MakeDefaultAllocator());

		bool Fail() const noexcept;
		explicit operator bool() const noexcept;
	private:
		node_holder load_node();
		void load_node_value(Node& node, std::optional<int64_t> service_ch);
		container_t load_children();
		text_t load_text(char limiter);
		property_map load_attributes();
		attribute_holder load_attribute();
		text_t load_quoted_line();
		std::optional<service_block_t> load_service_block();

		byte get_next();
		void unget_character();
		byte peek_next();	
		bool left_strip();											//true при переходе на новую строку
		void close_line();

		template <class Predicate>
		text_t load_line(Predicate pred) {
			text_t buffer;
			while (pred(peek_next())) {
				buffer.push_back(get_next());
			}
			return buffer;
		}

		std::istream& get_stream();
	private:
		std::istream* m_input;									//Для возможности копирования и перемещения объектов Reader'a
		allocator_holder m_tree_allocator;
		DocumentBuilder m_builder;
	};
}