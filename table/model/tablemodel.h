/*
 * Copyright (C) 2023 YtxCash
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

#ifndef TABLEMODEL_H
#define TABLEMODEL_H

// default implementations are for finance.

#include <QAbstractItemModel>
#include <QMutex>

#include "database/sqlite/sqlite.h"

class TableModel : public QAbstractItemModel {
    Q_OBJECT

public:
    virtual ~TableModel();

protected:
    TableModel(Sqlite* sql, bool rule, int node_id, CInfo& info, QObject* parent = nullptr);

signals:
    // send to TreeModel
    void SUpdateLeafValueTO(int node_id, double diff, CString& node_field);
    void SUpdateLeafValueFPTO(
        int node_id, double initial_debit_diff, double initial_credit_diff, double final_debit_diff, double final_credit_diff, double settled_diff = 0.0);
    void SSearch();

    // send to SignalStation
    void SAppendOneTrans(Section section, const TransShadow* trans_shadow);
    void SRemoveOneTrans(Section section, int node_id, int trans_id);
    void SUpdateBalance(Section section, int node_id, int trans_id);

    void SAppendHelperTrans(Section section, const TransShadow* trans_shadow);
    void SRemoveHelperTrans(Section section, int node_id, int trans_id);

    // send to its table view
    void SResizeColumnToContents(int column);

public slots:
    // receive from Sqlite
    void RRemoveMultiTransFPT(const QMultiHash<int, int>& node_trans);
    void RMoveMultiTransFPTS(int old_node_id, int new_node_id, const QList<int>& trans_id_list);

    // receive from SignalStation
    void RAppendOneTrans(const TransShadow* trans_shadow);
    void RRemoveOneTrans(int node_id, int trans_id);
    void RUpdateBalance(int node_id, int trans_id);
    void RRule(int node_id, bool rule);

public:
    // implemented functions
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

    virtual int GetNodeRow(int node_id) const;

    QModelIndex GetIndex(int trans_id) const;
    QStringList* GetDocumentPointer(const QModelIndex& index) const;

    void UpdateAllState(Check state);
    int NodeID() { return node_id_; }

protected:
    // virtual functions
    virtual bool UpdateDebit(TransShadow* trans_shadow, double value);
    virtual bool UpdateCredit(TransShadow* trans_shadow, double value);
    virtual bool UpdateRatio(TransShadow* trans_shadow, double value);

    virtual bool RemoveMultiTrans(const QSet<int>& trans_id_list); // just remove trnas_shadow, keep trans
    virtual bool AppendMultiTrans(int node_id, const QList<int>& trans_id_list);

protected:
    Sqlite* sql_ {};
    bool rule_ {};

    CInfo& info_;
    int node_id_ {};
    QMutex mutex_ {};

    QList<TransShadow*> trans_shadow_list_ {};
};

using PTableModel = QPointer<TableModel>;

#endif // TABLEMODEL_H
