#include "editnodeorder.h"

#include <QTimer>

#include "dialog/signalblocker.h"
#include "global/resourcepool.h"
#include "ui_editnodeorder.h"

EditNodeOrder::EditNodeOrder(
    NodeShadow* node_shadow, Sqlite* sql, TableModel* order_table, TreeModel* stakeholder_model, CSettings& settings, int party_unit, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::EditNodeOrder)
    , node_shadow_ { node_shadow }
    , sql_ { sql }
    , party_unit_ { party_unit }
    , stakeholder_tree_ { static_cast<TreeModelStakeholder*>(stakeholder_model) }
    , settings_ { settings }
    , info_node_ { party_unit == UNIT_CUST ? SALES : PURCHASE }
    , node_id_ { *node_shadow->id }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    IniDialog();
    IniConnect();

    ui->tableViewOrder->setModel(order_table);
    ui->pBtnSaveOrder->setEnabled(false);
    ui->pBtnFinishOrder->setEnabled(false);

    ui->labelParty->setText(tr("Party"));
    ui->comboParty->setFocus();

    IniUnit(*node_shadow->unit);
}

EditNodeOrder::~EditNodeOrder() { delete ui; }

void EditNodeOrder::RUpdateComboModel()
{
    if (*node_shadow_->branch)
        return;

    const int party_id { ui->comboParty->currentData().toInt() };
    const int employee_id { ui->comboEmployee->currentData().toInt() };

    stakeholder_tree_->LeafPathSpecificUnitPS(combo_model_party_, party_unit_, UnitFilterMode::kIncludeUnitOnly);
    stakeholder_tree_->LeafPathSpecificUnitPS(combo_model_employee_, UNIT_EMP, UnitFilterMode::kIncludeUnitOnlyWithEmpty);

    QTimer::singleShot(50, this, [this, employee_id, party_id]() { IniDataCombo(party_id, employee_id); });
}

void EditNodeOrder::RUpdateData(int node_id, TreeEnumOrder column, const QVariant& value)
{
    if (node_id != node_id_)
        return;

    SignalBlocker blocker(this);

    switch (column) {
    case TreeEnumOrder::kDescription:
        ui->lineDescription->setText(value.toString());
        break;
    case TreeEnumOrder::kRule:
        ui->chkBoxRefund->setChecked(value.toBool());
        break;
    case TreeEnumOrder::kUnit:
        IniUnit(value.toInt());
        break;
    case TreeEnumOrder::kParty: {
        int party_index { ui->comboParty->findData(value.toInt()) };
        ui->comboParty->setCurrentIndex(party_index);
        break;
    }
    case TreeEnumOrder::kEmployee: {
        int employee_index { ui->comboEmployee->findData(value.toInt()) };
        ui->comboEmployee->setCurrentIndex(employee_index);
        break;
    }
    case TreeEnumOrder::kDateTime:
        ui->dateTimeEdit->setDateTime(QDateTime::fromString(value.toString(), DATE_TIME_FST));
        break;
    case TreeEnumOrder::kFinished: {
        bool finished { value.toBool() };

        ui->pBtnFinishOrder->setChecked(finished);
        ui->pBtnFinishOrder->setText(finished ? tr("Edit") : tr("Finish"));
        LockWidgets(finished, *node_shadow_->branch);

        if (finished) {
            ui->pBtnPrint->setFocus();
            ui->pBtnPrint->setDefault(true);
            ui->tableViewOrder->clearSelection();
        }
        break;
    }
    default:
        return;
    }
}

void EditNodeOrder::RUpdateLeafValueTO(int /*node_id*/, double diff) { ui->dSpinFirst->setValue(ui->dSpinFirst->value() + diff); }

void EditNodeOrder::RUpdateLeafValueFPTO(int /*node_id*/, double first_diff, double second_diff, double amount_diff, double discount_diff, double settled_diff)
{
    if (*node_shadow_->branch)
        return;

    ui->dSpinFirst->setValue(ui->dSpinFirst->value() + first_diff);
    ui->dSpinSecond->setValue(ui->dSpinSecond->value() + second_diff);
    ui->dSpinAmount->setValue(ui->dSpinAmount->value() + amount_diff);
    ui->dSpinDiscount->setValue(ui->dSpinDiscount->value() + discount_diff);
    ui->dSpinSettled->setValue(ui->dSpinSettled->value() + (*node_shadow_->unit == UNIT_IM ? settled_diff : 0.0));
}

QTableView* EditNodeOrder::View() { return ui->tableViewOrder; }

void EditNodeOrder::IniDialog()
{
    combo_model_party_ = new QStandardItemModel(this);
    stakeholder_tree_->LeafPathSpecificUnitPS(combo_model_party_, party_unit_, UnitFilterMode::kIncludeUnitOnly);
    ui->comboParty->setModel(combo_model_party_);
    ui->comboParty->setCurrentIndex(-1);

    combo_model_employee_ = new QStandardItemModel(this);
    stakeholder_tree_->LeafPathSpecificUnitPS(combo_model_employee_, UNIT_EMP, UnitFilterMode::kIncludeUnitOnlyWithEmpty);
    ui->comboEmployee->setModel(combo_model_employee_);
    ui->comboEmployee->setCurrentIndex(-1);

    ui->dateTimeEdit->setDisplayFormat(DATE_TIME_FST);
    ui->dateTimeEdit->setDateTime(QDateTime::currentDateTime());
    *node_shadow_->date_time = ui->dateTimeEdit->dateTime().toString(DATE_TIME_FST);
    ui->comboParty->lineEdit()->setValidator(&LineEdit::kInputValidator);

    ui->dSpinDiscount->setRange(DMIN, DMAX);
    ui->dSpinAmount->setRange(DMIN, DMAX);
    ui->dSpinSettled->setRange(DMIN, DMAX);
    ui->dSpinSecond->setRange(DMIN, DMAX);
    ui->dSpinFirst->setRange(DMIN, DMAX);

    ui->dSpinDiscount->setDecimals(settings_.amount_decimal);
    ui->dSpinAmount->setDecimals(settings_.amount_decimal);
    ui->dSpinSettled->setDecimals(settings_.amount_decimal);
    ui->dSpinSecond->setDecimals(settings_.common_decimal);
    ui->dSpinFirst->setDecimals(settings_.common_decimal);

    ui->comboParty->setFocus();
}

void EditNodeOrder::accept()
{
    if (auto* focus_widget { this->focusWidget() })
        focus_widget->clearFocus();

    if (node_id_ == 0) {
        emit QDialog::accepted();
        node_id_ = *node_shadow_->id;

        if (!(*node_shadow_->branch))
            emit SUpdateNodeID(node_id_);

        ui->chkBoxBranch->setEnabled(false);
        ui->pBtnSaveOrder->setEnabled(false);
        ui->tableViewOrder->clearSelection();
    }
}

void EditNodeOrder::IniConnect() { connect(ui->pBtnSaveOrder, &QPushButton::clicked, this, &EditNodeOrder::accept); }

void EditNodeOrder::LockWidgets(bool finished, bool branch)
{
    bool basic_enable { !finished };
    bool not_branch_enable { !finished && !branch };

    ui->labelParty->setEnabled(basic_enable);
    ui->comboParty->setEnabled(basic_enable);

    ui->pBtnInsertParty->setEnabled(not_branch_enable);

    ui->labelSettled->setEnabled(not_branch_enable);
    ui->dSpinSettled->setEnabled(not_branch_enable);

    ui->dSpinAmount->setEnabled(not_branch_enable);

    ui->labelDiscount->setEnabled(not_branch_enable);
    ui->dSpinDiscount->setEnabled(not_branch_enable);

    ui->labelEmployee->setEnabled(not_branch_enable);
    ui->comboEmployee->setEnabled(not_branch_enable);
    ui->tableViewOrder->setEnabled(not_branch_enable);

    ui->rBtnCash->setEnabled(basic_enable);
    ui->rBtnMonthly->setEnabled(basic_enable);
    ui->rBtnPending->setEnabled(basic_enable);
    ui->dateTimeEdit->setEnabled(not_branch_enable);

    ui->dSpinFirst->setEnabled(not_branch_enable);
    ui->labelFirst->setEnabled(not_branch_enable);
    ui->dSpinSecond->setEnabled(not_branch_enable);
    ui->labelSecond->setEnabled(not_branch_enable);

    ui->chkBoxRefund->setEnabled(not_branch_enable);
    ui->lineDescription->setEnabled(basic_enable);

    ui->pBtnPrint->setEnabled(finished && !branch);
}

void EditNodeOrder::IniUnit(int unit)
{
    switch (unit) {
    case UNIT_IM:
        ui->rBtnCash->setChecked(true);
        break;
    case UNIT_MS:
        ui->rBtnMonthly->setChecked(true);
        break;
    case UNIT_PEND:
        ui->rBtnPending->setChecked(true);
        break;
    default:
        break;
    }
}

void EditNodeOrder::IniDataCombo(int party, int employee)
{
    ui->comboEmployee->blockSignals(true);
    ui->comboParty->blockSignals(true);

    int party_index { ui->comboParty->findData(party) };
    ui->comboParty->setCurrentIndex(party_index);

    int employee_index { ui->comboEmployee->findData(employee) };
    ui->comboEmployee->setCurrentIndex(employee_index);

    ui->comboEmployee->blockSignals(false);
    ui->comboParty->blockSignals(false);
}

void EditNodeOrder::on_comboParty_editTextChanged(const QString& arg1)
{
    if (!*node_shadow_->branch || arg1.isEmpty())
        return;

    *node_shadow_->name = arg1;

    if (node_id_ == 0) {
        ui->pBtnSaveOrder->setEnabled(true);
        ui->pBtnFinishOrder->setEnabled(true);
    }

    if (node_id_ != 0)
        sql_->UpdateField(info_node_, arg1, NAME, node_id_);
}

void EditNodeOrder::on_comboParty_currentIndexChanged(int /*index*/)
{
    if (*node_shadow_->branch)
        return;

    int party_id { ui->comboParty->currentData().toInt() };
    if (party_id <= 0)
        return;

    *node_shadow_->party = party_id;
    emit SUpdateParty(node_id_, party_id);

    if (node_id_ == 0) {
        ui->pBtnSaveOrder->setEnabled(true);
        ui->pBtnFinishOrder->setEnabled(true);
    }

    if (node_id_ != 0)
        sql_->UpdateField(info_node_, party_id, PARTY, node_id_);

    if (ui->comboEmployee->currentIndex() != -1)
        return;

    int employee_index { ui->comboEmployee->findData(stakeholder_tree_->Employee(party_id)) };
    ui->comboEmployee->setCurrentIndex(employee_index);

    ui->rBtnCash->setChecked(stakeholder_tree_->Rule(party_id) == RULE_IM);
    ui->rBtnMonthly->setChecked(stakeholder_tree_->Rule(party_id) == RULE_MS);
}

void EditNodeOrder::on_chkBoxRefund_toggled(bool checked)
{
    *node_shadow_->rule = checked;

    if (node_id_ != 0)
        sql_->UpdateField(info_node_, checked, RULE, node_id_);
}

void EditNodeOrder::on_comboEmployee_currentIndexChanged(int /*index*/)
{
    *node_shadow_->employee = ui->comboEmployee->currentData().toInt();

    if (node_id_ != 0)
        sql_->UpdateField(info_node_, *node_shadow_->employee, EMPLOYEE, node_id_);
}

void EditNodeOrder::on_rBtnCash_toggled(bool checked)
{
    if (!checked)
        return;

    *node_shadow_->unit = UNIT_IM;

    *node_shadow_->final_total = *node_shadow_->initial_total - *node_shadow_->discount;
    ui->dSpinSettled->setValue(*node_shadow_->final_total);

    if (node_id_ != 0) {
        sql_->UpdateField(info_node_, UNIT_IM, UNIT, node_id_);
        sql_->UpdateField(info_node_, *node_shadow_->final_total, SETTLED, node_id_);
    }
}

void EditNodeOrder::on_rBtnMonthly_toggled(bool checked)
{
    if (!checked)
        return;

    *node_shadow_->unit = UNIT_MS;

    *node_shadow_->final_total = 0.0;
    ui->dSpinSettled->setValue(0.0);

    if (node_id_ != 0) {
        sql_->UpdateField(info_node_, UNIT_MS, UNIT, node_id_);
        sql_->UpdateField(info_node_, 0.0, SETTLED, node_id_);
    }
}

void EditNodeOrder::on_rBtnPending_toggled(bool checked)
{
    if (!checked)
        return;

    *node_shadow_->unit = UNIT_PEND;

    *node_shadow_->final_total = 0.0;
    ui->dSpinSettled->setValue(0.0);

    if (node_id_ != 0) {
        sql_->UpdateField(info_node_, UNIT_PEND, UNIT, node_id_);
        sql_->UpdateField(info_node_, 0.0, SETTLED, node_id_);
    }
}

void EditNodeOrder::on_pBtnInsertParty_clicked()
{
    const auto& name { ui->comboParty->currentText() };
    if (*node_shadow_->branch || name.isEmpty() || ui->comboParty->currentIndex() != -1)
        return;

    auto* node { ResourcePool<Node>::Instance().Allocate() };
    node->rule = stakeholder_tree_->Rule(-1);
    stakeholder_tree_->SetParent(node, -1);
    node->name = name;

    node->unit = party_unit_;

    stakeholder_tree_->InsertNode(0, QModelIndex(), node);

    int party_index { ui->comboParty->findData(node->id) };
    ui->comboParty->setCurrentIndex(party_index);
}

void EditNodeOrder::on_dateTimeEdit_dateTimeChanged(const QDateTime& date_time)
{
    *node_shadow_->date_time = date_time.toString(DATE_TIME_FST);

    if (node_id_ != 0)
        sql_->UpdateField(info_node_, *node_shadow_->date_time, DATE_TIME, node_id_);
}

void EditNodeOrder::on_pBtnFinishOrder_toggled(bool checked)
{
    accept();

    *node_shadow_->finished = checked;

    sql_->UpdateField(info_node_, checked, FINISHED, node_id_);
    if (!(*node_shadow_->branch))
        emit SUpdateFinished(node_id_, checked);

    ui->pBtnFinishOrder->setText(checked ? tr("Edit") : tr("Finish"));

    LockWidgets(checked, *node_shadow_->branch);

    if (checked) {
        ui->tableViewOrder->clearSelection();
        ui->pBtnPrint->setFocus();
        ui->pBtnPrint->setDefault(true);
    }
}

void EditNodeOrder::on_chkBoxBranch_checkStateChanged(const Qt::CheckState& arg1)
{
    bool enable { arg1 == Qt::Checked };
    *node_shadow_->branch = enable;
    LockWidgets(false, enable);

    ui->comboEmployee->setCurrentIndex(-1);
    ui->comboParty->setCurrentIndex(-1);

    ui->pBtnSaveOrder->setEnabled(false);
    ui->pBtnFinishOrder->setEnabled(false);

    *node_shadow_->party = 0;
    *node_shadow_->employee = 0;
    if (enable)
        node_shadow_->date_time->clear();
    else
        *node_shadow_->date_time = ui->dateTimeEdit->dateTime().toString(DATE_TIME_FST);

    ui->chkBoxRefund->setChecked(false);
    ui->tableViewOrder->clearSelection();
}

void EditNodeOrder::on_lineDescription_editingFinished()
{
    *node_shadow_->description = ui->lineDescription->text();

    if (node_id_ != 0)
        sql_->UpdateField(info_node_, *node_shadow_->description, DESCRIPTION, node_id_);
}
