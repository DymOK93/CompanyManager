#pragma once
#include "xml.h"
#include "builder_base.h"

namespace xml {
	template <class ConcreteBuilder>
	class NodeBuilderBase : public BuilderBase<NodeBuilderBase<ConcreteBuilder>> {
	public:
		using MyBase = BuilderBase<NodeBuilderBase<ConcreteBuilder>>;
	public:
		ConcreteBuilder& SetAllocator(allocator_holder alloc) {						//Для возврата из BuilderBase ссылки на наследующие NodeBuilderBase классы 
			MyBase::SetAllocator(move(alloc));
			return MyBase::template get_context<ConcreteBuilder>();
		}

		ConcreteBuilder& SetName(std::string name) {
			m_name = move(name);
			return MyBase::template get_context<ConcreteBuilder>();
		}

		ConcreteBuilder& SetAttribute(std::string name, std::string value) {
			m_attrs.emplace(move(name), move(value));
			return MyBase::template get_context<ConcreteBuilder>();
		}

		ConcreteBuilder& SetAttributes(property_map attrs) {
			m_attrs = move(attrs);
			return MyBase::template get_context<ConcreteBuilder>();
		}

		ConcreteBuilder& Reset() {
			m_name = "";
			m_attrs.clear();
			return MyBase::template get_context<ConcreteBuilder>();
		}

		node_holder Assemble() {
            allocator_holder alloc{ MyBase::take_allocator() };
			return make_node_holder(
				alloc,
				Node::Header{ move(m_name), move(m_attrs) },
				alloc												//Перемещать нельзя - порядок вычисления аргументов функции не регламентируется Стандартом!
			);
		}

		node_holder Build() {
            allocator_holder alloc{ get_allocator() };
			return make_node_holder(
				alloc,
				Node::Header{ m_name, m_attrs },
				alloc
			);
		}
	protected:
		template <class... Types>
		static node_holder make_node_holder(allocator_holder target_alloc, Types&&... args) {
			node_holder holder(target_alloc->allocate(), make_deleter(target_alloc));
			std::allocator_traits<allocator_t>::construct(
				*target_alloc,
				holder.get(),
				std::forward<Types>(args)...
			);
			return holder;
		}

		static std::function<void(Node*)> make_deleter(allocator_holder target_alloc) {
			return [alloc = move(target_alloc)](Node* ptr) {
				std::allocator_traits<allocator_t>::destroy(*alloc, ptr);
				alloc->deallocate(ptr);
			};
		}

	private:
		text_t m_name;
		property_map m_attrs;
	};

	class NodeBuilder : public NodeBuilderBase<NodeBuilder> {
	public:
		using MyBase = NodeBuilderBase<NodeBuilder>;
	public:
		static node_holder Duplicate(const Node& source) {									//Копирование ветви дерева
			auto attrs_range{ source.GetAttributes() };
			node_holder deep_copy {
				make_node_holder(
					source.GetAllocator(),
					Node::Header{
						source.GetName(),
						property_map(attrs_range.begin(), attrs_range.end())
					},
					source.GetAllocator()
				)
			};
			duplicate_value(*deep_copy, source);
			return deep_copy;
		}

		static node_holder Take(Node&& source) {											//Перемещение узла с принадлежащими ему ветвями дерева
			allocator_holder alloc{ source.GetAllocator() };
			return make_node_holder(
				alloc,
				std::move(source)
			);
		}

	private:
		static void duplicate_value(Node& target, const Node& source) {									
			switch (source.GetType()) {
			case Node::Type::Service: target.SetService(source.AsService()); break;
			case Node::Type::Element: target.SetText(source.AsText()); break;
			case Node::Type::Tree: target.SetContainer(duplicate_children(source.AsContainer())); break;
			default: break;
			}
		}

		static container_t duplicate_children(const container_t& source) {
			container_t storage;
			storage.reserve(source.size());
			for (const auto& child_holder : source) {
				storage.push_back(Duplicate(*child_holder));
			}
			return storage;
		}
	};

	class ServiceNodeBuilder : public NodeBuilderBase <ServiceNodeBuilder> {
	public:
		using MyBase = NodeBuilderBase<ServiceNodeBuilder>;
	public:
		ServiceNodeBuilder& AddServiceSymbols(
			service_block_t front,
			std::optional<service_block_t> back
		) {
			m_service = {front, back};
			return *this;
		}

		ServiceNodeBuilder& Reset() {
			m_service = {};
			return *this;
		}

		node_holder Assemble() {
			auto nodeholder{ MyBase::Assemble() };
			nodeholder->SetService(move(m_service));
			return nodeholder;
		}

		node_holder Build() {
			auto nodeholder{ MyBase::Build() };
			nodeholder->SetService(m_service);
			return nodeholder;
		}

	private:
		service_t m_service;
	};

	class DocumentNodeBuilder : public NodeBuilderBase<DocumentNodeBuilder>{
	public:
		using MyBase = NodeBuilderBase<DocumentNodeBuilder>;
	public:
		template <class... Nodeholders>
		DocumentNodeBuilder& SetChildren(Nodeholders&&... args) {
			(m_children.push_back(std::forward<Nodeholders>(args)), ...);
			return *this;
		}

		DocumentNodeBuilder& Reset() {
			m_children.clear();
			return *this;
		}

		node_holder Assemble() {
			auto nodeholder{ MyBase::Assemble() };
			nodeholder->SetContainer(move(m_children));
			return nodeholder;
		}

		node_holder Build() {
			auto nodeholder{ MyBase::Build() };
			nodeholder->SetContainer(move(m_children));
			return nodeholder;
		}

	private:
		container_t m_children;
	};

	class ElementNodeBuilder: public NodeBuilderBase<ElementNodeBuilder> {
	public:
		using MyBase = NodeBuilderBase<ElementNodeBuilder>;
	public:
		ElementNodeBuilder& SetText(std::string text) {
			m_text = move(text);
			return *this;
		}

		ElementNodeBuilder& Reset() {
			m_text = "";
			return *this;
		}

		node_holder Assemble() {
			auto nodeholder{ MyBase::Assemble() };
			nodeholder->SetText(move(m_text));
			return nodeholder;
		}

		node_holder Build() {
			auto nodeholder{ MyBase::Build() };
			nodeholder->SetText(m_text);
			return nodeholder;
		}

	private:
		text_t m_text;
	};
}
