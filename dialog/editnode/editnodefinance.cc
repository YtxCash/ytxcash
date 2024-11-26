#include "editnodefinance.h"

#include "component/enumclass.h"
#include "dialog/signalblocker.h"
#include "ui_editnodefinance.h"

EditNodeFinance::EditNodeFinance(
    Node* node, QStandardItemModel* unit_model, CString& parent_path, CStringList& name_list, bool type_enable, bool unit_enable, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::EditNodeFinance)
    , node_ { node }
    , parent_path_ { parent_path }
    , name_list_ { name_list }
{
    ui->setupUi(this);
    SignalBlocker blocker(this);

    IniDialog(unit_model);
    IniConnect();
    Data(node, type_enable, unit_enable);
}

EditNodeFinance::~EditNodeFinance() { delete ui; }

void EditNodeFinance::IniDialog(QStandardItemModel* unit_model)
{
    ui->lineName->setFocus();
    ui->lineName->setValidator(&LineEdit::kInputValidator);

    this->setWindowTitle(parent_path_ + node_->name);

    ui->comboUnit->setModel(unit_model);
}

void EditNodeFinance::IniConnect() { connect(ui->lineName, &QLineEdit::textEdited, this, &EditNodeFinance::RNameEdited); }

void EditNodeFinance::Data(Node* node, bool type_enable, bool unit_enable)
{
    int item_index { ui->comboUnit->findData(node->unit) };
    ui->comboUnit->setCurrentIndex(item_index);
    ui->comboUnit->setEnabled(unit_enable);

    ui->rBtnDDCI->setChecked(node->rule);
    ui->rBtnDICD->setChecked(!node->rule);
    ui->rBtnLeaf->setChecked(true);

    if (node->name.isEmpty())
        return ui->pBtnOk->setEnabled(false);

    ui->lineName->setText(node->name);
    ui->lineCode->setText(node->code);
    ui->lineDescription->setText(node->description);
    ui->plainNote->setPlainText(node->note);

    switch (node->type) {
    case kTypeBranch:
        ui->rBtnBranch->setChecked(true);
        break;
    case kTypeLeaf:
        ui->rBtnLeaf->setChecked(true);
        break;
    case kTypeSupport:
        ui->rBtnSupport->setChecked(true);
        break;
    default:
        break;
    }

    ui->rBtnBranch->setEnabled(type_enable);
    ui->rBtnLeaf->setEnabled(type_enable);
    ui->rBtnSupport->setEnabled(type_enable);
}

void EditNodeFinance::RNameEdited(const QString& arg1)
{
    const auto& simplified { arg1.simplified() };
    this->setWindowTitle(parent_path_ + simplified);
    ui->pBtnOk->setEnabled(!simplified.isEmpty() && !name_list_.contains(simplified));
}

void EditNodeFinance::on_lineName_editingFinished() { node_->name = ui->lineName->text(); }

void EditNodeFinance::on_lineCode_editingFinished() { node_->code = ui->lineCode->text(); }

void EditNodeFinance::on_lineDescription_editingFinished() { node_->description = ui->lineDescription->text(); }

void EditNodeFinance::on_comboUnit_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    node_->unit = ui->comboUnit->currentData().toInt();
}

void EditNodeFinance::on_rBtnDDCI_toggled(bool checked)
{
    if (node_->final_total != 0 && node_->rule != checked) {
        node_->final_total = -node_->final_total;
        node_->initial_total = -node_->initial_total;
    }
    node_->rule = checked;
}

void EditNodeFinance::on_plainNote_textChanged() { node_->note = ui->plainNote->toPlainText(); }

void EditNodeFinance::on_rBtnLeaf_toggled(bool checked)
{
    if (checked)
        node_->type = kTypeLeaf;
}

void EditNodeFinance::on_rBtnBranch_toggled(bool checked)
{
    if (checked)
        node_->type = kTypeBranch;
}

void EditNodeFinance::on_rBtnSupport_toggled(bool checked)
{
    if (checked)
        node_->type = kTypeSupport;
}
