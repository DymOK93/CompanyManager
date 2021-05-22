#pragma once
#include "object_pool.h"
#include "xml_wrappers.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

#include <QVariant>
#include <QVector>

class CompanyTreeItem : public ObjectPool<CompanyTreeItem> {
 public:
  using MyBase = ObjectPool<CompanyTreeItem>;

 public:
  using Type = wrapper::XmlWrapper::Type;
  using tree_item_holder = MyBase::object_holder;
  using children_list = std::vector<tree_item_holder>;
  using Company = wrapper::Company;
  using Department = wrapper::Department;
  using Employee = wrapper::Employee;
  using wrapper_node_ptr =
      std::variant<std::monostate,
                   const Department*,
                   const Employee*>;  //Указатель, в отличие от итератора, не
                                      //инвалидируется при редактировании имени
  struct NodeInfo {
    QString displayed_name;
    wrapper_node_ptr wrapper_node;
    std::optional<wrapper::string_ref> parent_wrapper_name;
  };

 public:
  static constexpr size_t FIXED_COLUMN_COUNT{1}, DISPLAY_VIEW_ROLE{2};

 public:
  explicit CompanyTreeItem(wrapper::XmlWrapper::Type type,
                           const NodeInfo& node_info,
                           CompanyTreeItem* parent = nullptr);

  size_t childCount() const;
  size_t columnCount() const;  // always 1

  CompanyTreeItem* parent();
  CompanyTreeItem* child(size_t idx);
  size_t childNumber() const;  //Индекс в m_child_items родительского узла
  tree_item_holder& operator[](size_t idx) noexcept;

  void appendChild(
      tree_item_holder&& child);  // tree_item_holder некопируем -> избегаем
                                  // лишнего вызова move c-tor
  bool insertChild(tree_item_holder&& child, size_t pos);
  bool removeChild(size_t pos);

  Type GetType() const noexcept;
  const QString& GetName() const noexcept;
  QString ExchangeName(QString new_name) noexcept;
  const wrapper_node_ptr& GetWrapperNode() const noexcept;
  CompanyTreeItem& SetWrapperNode(wrapper_node_ptr node_ptr) noexcept;
  wrapper::string_ref GetParentWrapperName() const;  // throw if empty

  size_t Find(const QString& request);
  size_t UpperBound(
      const QString& request);  // Binary search. Behaviour is undefined if
                                // children list is unsorted

  std::optional<size_t> MoveChild(
      size_t from,
      size_t to);  //Перемещение потомка в списке. Возвращает новую позицию с
                   //учётом сдвига элементов

  static tree_item_holder make_instance(wrapper::XmlWrapper::Type type,
                                        NodeInfo node_info,
                                        CompanyTreeItem* parent = nullptr);

 private:
  static size_t find_child(const children_list& children,
                           const CompanyTreeItem* sought_child);

  template <class ForwardIt>  //При использовании C++20 были быдоступны
                              //одноименные стандартные алгоритмы
                              static ForwardIt shift_left(
                                  ForwardIt first,
                                  ForwardIt last) {  //Сдвиг на 1 позицию влево
    for (; first + 1 < last; ++first) {
      *first = std::move(*(first + 1));
    }
    return first;
  }
  template <class ForwardIt>
  ForwardIt shift_right(ForwardIt first,
                        ForwardIt last) {  //Сдвиг на 1 позицию вправо
    for (--last; last > first; --last) {
      *(last) = std::move(*(last - 1));
    }
    return last;
  }

 private:
  wrapper::XmlWrapper::Type m_node_type;
  NodeInfo m_node_info;
  CompanyTreeItem* m_parent;
  children_list m_child_items;
};