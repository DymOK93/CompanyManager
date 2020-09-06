#pragma once
#include "range.h"				//iterator range
#include "pool_allocator.h"		//allocator for nodes
#include "xml_exceptions.h"


#include <string>	
#include <string_view>
#include <array>
#include <vector>
#include <unordered_map>
#include <optional>		
#include <variant>
#include <utility>				//move, pair
#include <memory>				//unique_ptr, shared_ptr
#include <functional>			//custom deleter

namespace xml {
	class Node;

	using empty_t = std::monostate;
	using service_block_t = uintmax_t;														//2x4 или 2x8 байт
	using service_t = std::pair<service_block_t, std::optional<service_block_t>>;		
	using text_t = std::string;
	using property_map = std::unordered_map<text_t, text_t>;
	using deleter_t = std::function<void(Node*)>;
	using node_holder = std::unique_ptr<Node, deleter_t>;
	using container_t = std::vector<node_holder>;
	using attribute_holder = std::pair<text_t, text_t>;
	using allocator_t = utility::memory::PoolAllocator<Node>;
	using allocator_holder = std::shared_ptr<allocator_t>;
	using allocator_weak = std::weak_ptr<allocator_t>;

	class Node {
	public:
		struct Header {
			text_t name;
			property_map attributes;
		};
		using attribute_it = property_map::const_iterator;
		using attribute_range = Range<attribute_it>;

	public:
		enum class Type {
			Empty,
			Service,
			Element,
			Tree
		};

		static constexpr std::string_view TypeAsString(Type type) noexcept {
			return type_as_string[static_cast<size_t>(type)];
		}

	private:
		static constexpr std::array<std::string_view, 4> type_as_string{
			"empty", "service", "element", "tree"
		};

	public:
		Node(Header header, allocator_weak alloc);

		Type GetType() const noexcept;	

		const text_t& GetName() const noexcept;
		void ChangeName(text_t new_name) noexcept;

		size_t AttributesCount() const noexcept;
		attribute_range GetAttributes() const noexcept;

		void AddAttribute(text_t name, text_t value);
		void AddAttribute(attribute_holder attr);

		text_t& operator[](const text_t& attr_name);
		const text_t& at(const text_t& attr_name) const;

		void Reset();
		
		service_t& AsService();
		const service_t& AsService() const;
		void SetService(service_t new_nalue) noexcept;
		void ResetAsService();

		text_t& AsText();
		const text_t& AsText() const;
		void SetText(text_t new_nalue) noexcept;
		void ResetAsText();

		container_t& AsContainer();
		const container_t& AsContainer() const;	
		void SetContainer(container_t new_nalue) noexcept;
		void ResetAsContainer();

		void Clear() noexcept;
		allocator_holder GetAllocator() const noexcept;
	private:
		Header m_header;
		allocator_weak m_allocator;
		std::variant<
			empty_t,
			service_t,
			text_t,
			container_t> m_body;
	};

	class DocumentBuilder;

	class Document {
	public:
		Document() = default;												//Для возможности создания пустого объекта и дальнейшего вызова оператора присваивания 
		Node& GetRoot() noexcept;
		const Node& GetDeclaration() const noexcept;
		const Node& GetRoot() const noexcept;
		allocator_holder GetAllocator() const noexcept;
	private:
		friend class DocumentBuilder;
		Document(
			node_holder declaration, 
			node_holder root, 
			allocator_holder alloc
		);
	private:
		node_holder m_declaration, m_root;
		allocator_holder m_tree_allocator;
	};

	class DocumentBuilder {
	public:
		DocumentBuilder& SetDeclaration(node_holder new_declaration);
		DocumentBuilder& SetRoot(node_holder new_root);
		DocumentBuilder& SetAllocator(allocator_holder external_alloc);
		
		Document Assemble();
	private:
		std::optional<node_holder> m_declaration, m_root;
		allocator_holder m_tree_allocator;
	};
}

