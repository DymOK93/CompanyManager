#include "tree_item.h"

CompanyTreeItem::CompanyTreeItem(
	wrapper::XmlWrapper::Type type,
	const NodeInfo& node_info,
	CompanyTreeItem* parent
)
	: m_node_type(type), m_node_info(std::move(node_info)),
	m_parent(parent)
{
}

size_t CompanyTreeItem::childCount() const {
	return m_child_items.size();
}

size_t CompanyTreeItem::columnCount() const {
	return FIXED_COLUMN_COUNT;
}

CompanyTreeItem* CompanyTreeItem::child(size_t idx) {
	return idx < m_child_items.size() ?
		m_child_items[idx].get() : nullptr;
}

CompanyTreeItem* CompanyTreeItem::parent() {
	return m_parent;
}

void CompanyTreeItem::appendChild(tree_item_holder&& child) {
	m_child_items.push_back(std::move(child));
}

bool CompanyTreeItem::insertChild(tree_item_holder&& child, size_t position) {
	if (position > m_child_items.size()) {													
		return false;
	}
	m_child_items.insert(
		m_child_items.begin() + position,
		std::move(child)
	);
	return true;
}


bool CompanyTreeItem::removeChild(size_t position) {
	if (position >= m_child_items.size()) {						
		return false;												
	}
	m_child_items.erase(m_child_items.begin() + position);
	return true;
}

size_t CompanyTreeItem::childNumber() const {
	if (!m_parent) {
		return 0;
	}
	return find_child(m_parent->m_child_items, this);
}

CompanyTreeItem::tree_item_holder& CompanyTreeItem::operator[](size_t idx) noexcept {
	return m_child_items[idx];
}


size_t CompanyTreeItem::find_child(const children_list& children, const CompanyTreeItem* sought_child) {
	auto it{
		std::find_if(
			 children.begin(),
			 children.end(),
			[sought_child](const tree_item_holder& child_holder) {
				return sought_child == child_holder.get();
			}
		)
	};
	if (it == children.end()) {
		throw std::logic_error("The parent has no information about the child");	//Защита от узлов с некорректным parent
	}
	return it - children.begin();													//Индекс элемента
}

CompanyTreeItem::Type CompanyTreeItem::GetType() const noexcept {
	return m_node_type;
}

const QString& CompanyTreeItem::GetName() const noexcept {
	return m_node_info.displayed_name;
}

QString CompanyTreeItem::ExchangeName(QString new_name) noexcept {
	return std::exchange(m_node_info.displayed_name, std::move(new_name));
}

const CompanyTreeItem::wrapper_node_ptr& CompanyTreeItem::GetWrapperNode() const noexcept {
	return m_node_info.wrapper_node;
}

CompanyTreeItem& CompanyTreeItem::SetWrapperNode(wrapper_node_ptr node_ptr) noexcept {
	m_node_info.wrapper_node = node_ptr;
	return *this;
}

wrapper::string_ref CompanyTreeItem::GetParentWrapperName() const {
	return m_node_info.parent_wrapper_name.value();
}

size_t CompanyTreeItem::Find(const QString& request) {
	auto it{
		std::find_if(
			m_child_items.begin(), m_child_items.end(),
			[&request](const tree_item_holder& item) {
				return item->GetName() == request;
			}
		)
	};
	return it - m_child_items.begin();
}

size_t CompanyTreeItem::UpperBound(const QString& request) {
	auto it{
		std::upper_bound(
			m_child_items.begin(), m_child_items.end(),
			request,
			[](const QString& value, const tree_item_holder& item) {
				return QString::compare(value, item->GetName(), Qt::CaseInsensitive) < 1;		//Сравнение "<" без учета регистра
			}
		)
	};
	return it - m_child_items.begin();
}

std::optional<size_t> CompanyTreeItem::MoveChild(size_t from, size_t to) {
	if (from == to											  //Проверка во избежание ненужного выполнения длительной - O(N) - операции сдвига элементов    
		|| from >= m_child_items.size()
		|| to > m_child_items.size()
		) {
		return std::nullopt;
	}
	tree_item_holder child{ std::move(m_child_items[from]) };
	if (from < to) {
		shift_left(m_child_items.begin() + from, m_child_items.begin() + to);
		m_child_items[to - 1] = std::move(child);
		return to - 1;
	}
	else {													//from > to
		shift_right(m_child_items.begin() + to, m_child_items.begin() + from + 1);
		m_child_items[to] = std::move(child);
		return to;
	}
}

CompanyTreeItem::tree_item_holder CompanyTreeItem::make_instance(
	wrapper::XmlWrapper::Type type,
	NodeInfo node_info,
	CompanyTreeItem* parent
) {
	return allocate_instance(type, std::move(node_info), parent);
}
