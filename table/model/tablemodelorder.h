/*
 * Copyright (C) 2023 YtxErp
 *
 * This file is part of YTX.
 *
 * YTX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YTX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with YTX. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef TABLEMODELORDER_H
#define TABLEMODELORDER_H

#include "database/sqlite/sqlitestakeholder.h"
#include "tablemodel.h"
#include "tree/model/treemodel.h"
#include "tree/model/treemodelproduct.h"

class TableModelOrder final : public TableModel {
    Q_OBJECT

public:
    TableModelOrder(Sqlite* sql, bool rule, int node_id, CInfo& info, const NodeShadow* node_shadow, CTreeModel* product_tree, Sqlite* sqlite_stakeholder,
        QObject* parent = nullptr);
    ~TableModelOrder() override;

public slots:
    void RSyncOneValue(int node_id, int column, const QVariant& value) override;

public:
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    void sort(int column, Qt::SortOrder order) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    bool removeRows(int row, int, const QModelIndex& parent = QModelIndex()) override;

private:
    bool UpdateInsideProduct(TransShadow* trans_shadow, int value);
    bool UpdateOutsideProduct(TransShadow* trans_shadow, int value);

    bool UpdateUnitPrice(TransShadow* trans_shadow, double value);
    bool UpdateDiscountPrice(TransShadow* trans_shadow, double value);
    bool UpdateSecond(TransShadow* trans_shadow, double value);
    void PurifyTransShadow(int lhs_node_id = 0);

    void CrossSearch(TransShadow* trans_shadow, int product_id, bool is_inside) const;

    void UpdateLhsNode(int node_id);
    void UpdateFinished(int node_id, bool checked);
    void UpdateParty(int node_id, int party_id);
    void UpdatePrice();

private:
    const TreeModelProduct* product_tree_ {};
    SqliteStakeholder* sqlite_stakeholder_ {};
    QHash<int, double> sync_price_ {}; // inside_product_id, exclusive_price
    const NodeShadow* node_shadow_ {};
    int party_id_ {};
};

#endif // TABLEMODELORDER_H
