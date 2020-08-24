#include "tree_model.h"

CompanyTreeModel::CompanyTreeModel(const CompanyManager& cm, QObject* parent)
    : QAbstractItemModel(parent) {
    SetCompany(cm);
}


CompanyTreeModel& CompanyTreeModel::SetCompany(const CompanyManager& cm) {
    beginResetModel();                                                  //Уведомляем все привязанные view'ы об инвалидации даных модели
    m_root_item = build_company_tree(cm.Read());
    endResetModel();
    return *this;
}


CompanyTreeModel& CompanyTreeModel::Reset() noexcept {
    beginResetModel();                                                          
    m_root_item.reset();    
    endResetModel();
    return *this;
}


int CompanyTreeModel::rowCount(const QModelIndex& parent) const {
    const auto* item{ get_item(parent) };
    if (item) {
        return static_cast<int>(item->childCount());
    }
    return m_root_item ?                                                //После сброса Reset() внешние объекты могут запрашивать кол-во дочерних узлов для перерисовки                         
        static_cast<int>(m_root_item->childCount()) : 0;
}

int CompanyTreeModel::columnCount(const QModelIndex& parent) const {
    (void)parent;                                                       //Неиспользуемый параметр
    return m_root_item ?
        static_cast<int>(m_root_item->columnCount()) : 1;
}

Qt::ItemFlags CompanyTreeModel::flags(const QModelIndex& index) const {
    return index.isValid() ?
        Qt::ItemIsEnabled | Qt::ItemIsSelectable | QAbstractItemModel::flags(index)
        : Qt::NoItemFlags;
}

QModelIndex CompanyTreeModel::parent(const QModelIndex& index) const {
    if (!index.isValid())
        return QModelIndex();
    CompanyTreeItem* child_item{ get_item(index) };
    CompanyTreeItem* parent_item{
        child_item ?
        child_item->parent() : nullptr
    };

    if (!parent_item || parent_item == m_root_item.get())
        return QModelIndex();

    return createIndex(static_cast<int>(parent_item->childNumber()), 0, parent_item);
}

QModelIndex CompanyTreeModel::DepartmentIndex(size_t row) const {
    return index(static_cast<int>(row), 0, QModelIndex());
}

QModelIndex CompanyTreeModel::EmployeeIndex(const QModelIndex& department, size_t row) const {
    if (auto item = get_item(department);
        !item || item->GetType() != ItemType::Department) {
        return QModelIndex();
    }
    return index(static_cast<int>(row), 0, department);
}

QModelIndex CompanyTreeModel::index(
    int row,
    int column,
    const QModelIndex& parent
) const {
    if (parent.isValid() && parent.column() != 0) {
        return QModelIndex();
    }
    CompanyTreeItem* parent_item{ get_item(parent) };
    if (!parent_item) {
        return QModelIndex();
    }
    CompanyTreeItem* child_item{ parent_item->child(row) };
    if (!child_item) {
        return QModelIndex();
    }
    return createIndex(row, column, child_item);
}

QVariant CompanyTreeModel::data(const QModelIndex& index, int role) const noexcept{
    auto* item{ get_item(index) };
    if (!item || role != Qt::DisplayRole) {
        return QVariant();
    }
    return item->GetName();
}

std::optional<CompanyTreeModel::ItemType> CompanyTreeModel::GetItemType(const QModelIndex& index) const noexcept {
    auto* item{ get_item(index) };
    if (!item) {
        return std::nullopt;
    }
    return item->GetType();
}

std::optional<QString> CompanyTreeModel::GetItemName(const QModelIndex& index) const noexcept {
    auto* item{ get_item(index) };
    if (!item || item == m_root_item.get()) {                                       //Имя компании в текущей модели не предусмотрено, а её data_storage
        return std::nullopt;                                                        //имеет размер 1! 
    }                                                                               //Проверка во избежание UB
    return item->GetName();
}

std::optional<QString> CompanyTreeModel::ExchangeItemName(const QModelIndex& index, QString new_name) noexcept {
    auto* item{ get_item(index) };
    if (!item || item == m_root_item.get()) {                                      
        return std::nullopt;                                                       
    }
    emit dataChanged(index, index);                                                 //Уведомление об изменении данных
    return item->ExchangeName(std::move(new_name));
}

CompanyTreeModel::wrapper_node_ptr CompanyTreeModel::GetItemNode(const QModelIndex& index) {
    auto* item{ get_item(index) };
    if (!item) {
        return {};
    }
    return item->GetWrapperNode();
}

std::optional<wrapper::string_ref> CompanyTreeModel::GetDepartmentNameRef(const QModelIndex& index) {
    auto* item{ get_item(index) };
    if (!item) {
        return std::nullopt;
    }
    switch (item->GetType()) {
    case ItemType::Employee: return item->GetParentWrapperName();
    case ItemType::Department: return std::get<const wrapper::Department*>(item->GetWrapperNode())->GetName();
    default: return std::nullopt;
    };
}

QModelIndex CompanyTreeModel::GetPrevItem(const QModelIndex& old_index) {
    QModelIndex parent_idx(parent(old_index));
    if (!parent_idx.isValid()) {
        return QModelIndex();
    }
    return old_index.row() == 0 ?
        QModelIndex() : index(old_index.row() - 1, 0, parent_idx);
}

QModelIndex CompanyTreeModel::GetNextItem(const QModelIndex& old_index) {
    if (!old_index.isValid()) {
        return QModelIndex();
    }
    QModelIndex parent_idx(parent(old_index));                              //Родительский (возможно, корневой) индекс
   
    size_t row_count{ static_cast<size_t>(rowCount(parent_idx)) };
    return static_cast<size_t>(old_index.row()) + 1 >= row_count ?
        QModelIndex() : index(old_index.row() + 1, 0, parent_idx);
}

std::optional<size_t> CompanyTreeModel::FindChild(const QString& request, const QModelIndex& parent) {
    auto* item{ get_item(parent) };
    if (!item) {
        return std::nullopt;
    }
    return item->Find(request);
}

std::optional<size_t> CompanyTreeModel::UpperBoundChild(const QString& request, const QModelIndex& parent) {
    auto* item{ get_item(parent) };
    if (!item) {
        return std::nullopt;
    }
    return item->UpperBound(request);
}

std::optional<size_t> CompanyTreeModel::MoveItem(size_t row_from, size_t row_to, const QModelIndex& parent) {
    auto item{ get_item(parent) };
    if (!item || row_from == row_to)  {
        return std::nullopt;
    }
    std::optional<size_t> result{ (row_from > row_to ? row_to : row_to - 1) };
    if (beginMoveRows(                                                                      //Все необходимые проверки возложены на beginMoveRows()
        parent,
        static_cast<int>(row_from), static_cast<int>(row_from),                             //source
        parent,
        static_cast<int>(row_to)                                                            //dest   
    )) {
        item->MoveChild(row_from, row_to);
        endMoveRows();
    } 
    return result;
}

QModelIndex CompanyTreeModel::InsertDepartmentItem(const Department& source, size_t pos) {
    if (!m_root_item || pos > m_root_item->childCount()) {
        return QModelIndex();
    }   
    beginInsertRows(QModelIndex(), static_cast<int>(pos), static_cast<int>(pos));                               //root index
    bool success{
        m_root_item->insertChild(
            build_department_tree(source, m_root_item.get()),
            pos
        )
    };
    endInsertRows();
    if (!success) {
        return QModelIndex();
    }
    return DepartmentIndex(pos);
}

bool CompanyTreeModel::RemoveDepartmentItem(size_t pos) {
    if (!m_root_item) {
        return false;
    }
    beginRemoveRows(QModelIndex(), static_cast<int>(pos), static_cast<int>(pos));                  //root index
    bool success{ m_root_item->removeChild(pos) };
    endRemoveRows();
    return success;
}

CompanyTreeModel::Memento CompanyTreeModel::DumpDepartmentItem(size_t pos) {
    if (!m_root_item || pos > m_root_item->childCount()) {
        return Memento();
    }
    auto& company{ *m_root_item };
    Memento::Dump dump{
        pos,
        std::nullopt,
        std::move(company[pos])
    };
    beginRemoveRows(QModelIndex(), static_cast<int>(pos), static_cast<int>(pos));
    company.removeChild(pos);
    endRemoveRows();
    return std::move(dump);
}

bool CompanyTreeModel::RestoreDepartmentItem(Memento&& item, const Department* new_node) {
    Memento::Dump department_dump(item.Extract());
    if (new_node) {
        department_dump.branch->SetWrapperNode(new_node);
    }
    beginInsertRows(
        QModelIndex(),                                              //root index
        static_cast<int>(department_dump.pos),
        static_cast<int>(department_dump.pos)
    );
    bool success{
        m_root_item->insertChild(
             std::move(department_dump.branch),
             department_dump.pos
        )
    };
    endInsertRows();
    return success;
}

QModelIndex CompanyTreeModel::InsertEmployeeItem(const Employee& source, const QModelIndex& department, size_t pos) {
    auto* item{ get_item(department) };
    if (!item
        || item->GetType() != ItemType::Department
        || pos > item->childCount()) {
        return QModelIndex();
    }
    tree_item_holder child{
        build_employee_tree(
            source,
            std::get<const wrapper::Department*>(item->GetWrapperNode())->GetName(),
            item
        )
    };
    beginInsertRows(department, static_cast<int>(pos), static_cast<int>(pos));
    if(pos == item->childCount()) {
         item->appendChild(std::move(child));
    }
    else {
        item->insertChild(std::move(child), pos);
    }
    endInsertRows();
    return EmployeeIndex(department, pos);
}

bool CompanyTreeModel::RemoveEmployeeItem(const QModelIndex& department, size_t pos) {
    auto* item{ get_item(department) };
    if (!item
        || item->GetType() != ItemType::Department
        || pos >= item->childCount()) {
        return false;
    }
    beginRemoveRows(department, static_cast<int>(pos), static_cast<int>(pos));
    bool success{ item->removeChild(pos) };
    endRemoveRows();
    return success;
}

CompanyTreeModel::Memento CompanyTreeModel::DumpEmployeeItem(const QModelIndex& department, size_t pos) {
    auto* item{ get_item(department) };
    if (!item
        || item->GetType() != ItemType::Department          //Родительский узел - подразделение
        || pos >= item->childCount()) {
        return Memento();
    }
    Memento::Dump dump{
        pos,
        item->childNumber(),
        std::move((*item)[pos])
    };
    beginRemoveRows(department, static_cast<int>(pos), static_cast<int>(pos));
    item->removeChild(pos);
    endRemoveRows();
    return std::move(dump);                                     //Перемещаем в конструктор Memento
}

bool CompanyTreeModel::RestoreEmployeeItem(Memento&& item, const Employee* new_node) {
    Memento::Dump employee_dump(item.Extract());
    if (new_node) {
        employee_dump.branch->SetWrapperNode(new_node);
    }
    QModelIndex department_q_idx{ DepartmentIndex(employee_dump.parent_pos.value()) };
    auto* department{ get_item(department_q_idx) };
    if (!department) {
        return false;
    }
    beginInsertRows(
        department_q_idx,
        static_cast<int>(employee_dump.pos),
        static_cast<int>(employee_dump.pos)
    );
    bool success{
        department->insertChild(
             std::move(employee_dump.branch),
             employee_dump.pos
        )
    };
    endInsertRows();
    return success;
}


CompanyTreeModel::tree_item_holder CompanyTreeModel::build_company_tree(const wrapper::Company& company) {
    tree_item_holder root_item{
       CompanyTreeItem::make_instance(
           ItemType::Company,
           {}
       )
    };
    auto range{ company.GetDepartments()};                                               //Диапазон итераторов с сотрудниками
    for (auto it = range.begin(); it != range.end(); ++it) {
        root_item->appendChild(
            build_department_tree(*it, root_item.get())
        );
    }
    return root_item;
}

CompanyTreeModel::tree_item_holder 
CompanyTreeModel::build_department_tree(const Department& department, CompanyTreeItem* parent) {
    tree_item_holder item{
        CompanyTreeItem::make_instance(
            ItemType::Department,
            CompanyTreeItem::NodeInfo{
                QString::fromStdString(department.GetName()),
                std::addressof(department),
                std::nullopt
            },
            parent
        )
    };
    auto range{ department.GetEmployees() };                                               //Диапазон итераторов с сотрудниками
    for (auto it = range.begin(); it != range.end(); ++it) {
        item->appendChild(
            build_employee_tree(
                it->second,
                department.GetName(),    
                item.get()
            )
        );
    }
    return item;
}

CompanyTreeModel::tree_item_holder 
CompanyTreeModel::build_employee_tree(
    const Employee& employee,
    wrapper::string_ref department_name,
    CompanyTreeItem* parent
) {
    return CompanyTreeItem::make_instance(
        ItemType::Employee,
        CompanyTreeItem::NodeInfo{
           FullNameRefToQString(employee.GetFullName()),
            std::addressof(employee),
            department_name
        },
        parent
    );
}

QString CompanyTreeModel::FullNameRefToQString(const wrapper::FullNameRef& name) {
    QString full_name(name.surname.get().data());
    full_name += ' ';
    full_name += name.name.get().data();
    full_name += ' ';
    full_name += name.middle_name.get().data();
    return full_name;
}

CompanyTreeItem* CompanyTreeModel::get_item(const QModelIndex& index) const {
    if (index.isValid()) {
        if (CompanyTreeItem* item = static_cast<CompanyTreeItem*>(index.internalPointer());
            item) {
            return item;
        }
    }
    return m_root_item.get();
}
