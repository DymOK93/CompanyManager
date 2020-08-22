#pragma once
#include "tree_item.h"
#include "xml_wrappers.h"
#include "company_manager_engine.h"


#include <optional>
#include <utility>
#include <string_view>

#include <QtWidgets>
#include <QModelIndex>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QString>
#include <QStringList>


class CompanyTreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    using ItemType = CompanyTreeItem::Type;
    using tree_item_holder = CompanyTreeItem::tree_item_holder;
    using wrapper_node_ptr = CompanyTreeItem::wrapper_node_ptr;
    using Company = wrapper::Company;
    using Department = wrapper::Department;
    using Employee = wrapper::Employee;
public:
    class Memento {
    public:
        bool IsValid() const noexcept {
            return m_dump.has_value();
        }
    private:
        struct Dump {
            size_t pos;
            std::optional<size_t> parent_pos;                   //������ ��� Employee
            tree_item_holder branch;       
        };

        friend class CompanyTreeModel;
        Memento() = default;
        Memento(Dump dump)
            : m_dump(std::move(dump))
        {
        }
        Dump Extract() {
            return std::move(*m_dump);                          //������ ��� ����������� �������������
        }
    private:
        std::optional<Dump> m_dump;
    };
public:
    CompanyTreeModel() = default;
    CompanyTreeModel(const CompanyManager& cm, QObject* parent = nullptr);

    CompanyTreeModel& SetCompany(const CompanyManager& cm);
    CompanyTreeModel& Reset() noexcept;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QModelIndex parent(const QModelIndex& index) const override;
    QModelIndex DepartmentIndex(size_t row) const;
    QModelIndex EmployeeIndex(const QModelIndex& department, size_t row) const;

    QModelIndex index(
        int row,
        int column,
        const QModelIndex& parent = QModelIndex()
    ) const override;
    QVariant data(const QModelIndex& index, int role) const noexcept override;
    

    std::optional<ItemType> GetItemType(const QModelIndex& index) const noexcept;                      //nullopt, ���� ������ �� �������
    std::optional<QString> GetItemName(const QModelIndex& index) const noexcept;                       
    std::optional<QString> ExchangeItemName(const QModelIndex& index, QString new_name) noexcept;

    wrapper_node_ptr GetItemNode(const QModelIndex& index);
    std::optional<wrapper::string_ref> GetDepartmentNameRef(const QModelIndex& index);                    //string_view �� ������, ������� � ������� XML-����

    QModelIndex GetPrevItem(const QModelIndex& index);                                                  //��������� � ���������� ������
    QModelIndex GetNextItem(const QModelIndex& index);                                                  //��������� � ��������� ������

    std::optional<size_t> FindChild(const QString& request, const QModelIndex& parent = QModelIndex());          //linear search
    std::optional<size_t> UpperBoundChild(const QString& request, const QModelIndex& parent = QModelIndex());    //binary search for sorted ranges
 
                                                                                            
    std::optional<size_t> MoveItem(       //����������� ������� parent �� ����� �������. ���������� ���������� ������ ������� � ������ ������ �������� ���������
        size_t row_from,                                                                                //�������� ������
        size_t row_before,                                                                              //�������, ����� ������� ����� ����������� �������
        const QModelIndex& parent = QModelIndex()
    );   

    QModelIndex InsertDepartmentItem(const Department& source, size_t pos);
    bool RemoveDepartmentItem(size_t pos);
    Memento DumpDepartmentItem(size_t pos);
    bool RestoreDepartmentItem(Memento&& item, const Department* new_node = nullptr);

    QModelIndex InsertEmployeeItem(const Employee& source, const QModelIndex& department, size_t pos);
    bool RemoveEmployeeItem(const QModelIndex& department, size_t pos);
    Memento DumpEmployeeItem(const QModelIndex& department, size_t pos);
    bool RestoreEmployeeItem(Memento&& item, const Employee* new_node = nullptr);                       //���� ���� ������

    static QString FullNameRefToQString(const wrapper::FullNameRef& name);
private:
    static tree_item_holder build_company_tree(const Company& company);
    static tree_item_holder build_department_tree(const Department&  department, CompanyTreeItem* parent);
    static tree_item_holder build_employee_tree(
        const Employee& employee,
        wrapper::string_ref department_name,
        CompanyTreeItem* parent         
    );

    CompanyTreeItem* get_item(const QModelIndex& index) const;                              //��������� �������� �� ���������� �������
private:
    tree_item_holder m_root_item{ MakeDummyObjectHolder<CompanyTreeItem>() };               //�������� �������
};