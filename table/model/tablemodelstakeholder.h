#ifndef TABLEMODELSTAKEHOLDER_H
#define TABLEMODELSTAKEHOLDER_H

#include "tablemodel.h"

class TableModelStakeholder final : public TableModel {
    Q_OBJECT

public:
    TableModelStakeholder(SPSqlite sql, bool rule, int node_id, CInfo& info, QObject* parent = nullptr);
    ~TableModelStakeholder() override = default;

public:
    // implemented functions
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

protected:
    bool RemoveMultiTrans(const QSet<int>& trans_id_list) override; // just remove trnas_shadow, keep trans
    bool AppendMultiTrans(int node_id, const QList<int>& trans_id_list) override;
};

#endif // TABLEMODELSTAKEHOLDER_H
