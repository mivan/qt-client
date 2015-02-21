/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "characteristicAssignment.h"

#include <QMessageBox>
#include <QSqlError>
#include <QVariant>
#include <QRegExpValidator>

#include "characteristic.h"

characteristicAssignment::characteristicAssignment(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  setupUi(this);

  connect(_buttonBox, SIGNAL(accepted()), this, SLOT(sSave()));
  connect(_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
  connect(_char, SIGNAL(currentIndexChanged(int)), this, SLOT(sHandleChar()));

  _listpriceLit->hide();
  _listprice->hide();
  _listprice->setValidator(omfgThis->priceVal());
  _template = false;

  adjustSize();
}

characteristicAssignment::~characteristicAssignment()
{
  // no need to delete child widgets, Qt does it all for us
}

void characteristicAssignment::languageChange()
{
  retranslateUi(this);
}

enum SetResponse characteristicAssignment::set(const ParameterList &pParams)
{
  XDialog::set(pParams);
  QVariant param;
  bool     valid;

  param = pParams.value("item_id", &valid);
  if (valid)
  {
    _targetId = param.toInt();
    _targetType = "I";
    handleTargetType();
  }

  param = pParams.value("cust_id", &valid);
  if (valid)
  {
    _targetId = param.toInt();
    _targetType = "C";
    handleTargetType();
  }

  param = pParams.value("crmacct_id", &valid);
  if (valid)
  {
    _targetId = param.toInt();
    _targetType = "CRMACCT";
    handleTargetType();
  }

  param = pParams.value("addr_id", &valid);
  if (valid)
  {
    _targetId = param.toInt();
    _targetType = "ADDR";
    handleTargetType();
  }

  param = pParams.value("cntct_id", &valid);
  if (valid)
  {
    _targetId = param.toInt();
    _targetType = "CNTCT";
    handleTargetType();
  }

  param = pParams.value("custtype_id", &valid);
  if (valid)
  {
    _targetId = param.toInt();
    _targetType = "CT";
    handleTargetType();
  }

  param = pParams.value("ls_id", &valid);
  if (valid)
  {
    _targetId = param.toInt();
    _targetType = "LS";
    handleTargetType();
  }

  param = pParams.value("lsreg_id", &valid);
  if (valid)
  {
    _targetId = param.toInt();
    _targetType = "LSR";
    handleTargetType();
  }

  param = pParams.value("ophead_id", &valid);
  if (valid)
  {
    _targetId = param.toInt();
    _targetType = "OPP";
    handleTargetType();
  }

  param = pParams.value("emp_id", &valid);
  if (valid)
  {
    _targetId = param.toInt();
    _targetType = "EMP";
    handleTargetType();
  }

  param = pParams.value("incdt_id", &valid);
  if (valid)
  {
    _targetId = param.toInt();
    _targetType = "INCDT";
    handleTargetType();
  }

  param = pParams.value("prj_id", &valid);
  if (valid)
  {
    _targetId = param.toInt();
    _targetType = "PROJ";
    handleTargetType();
  }

  param = pParams.value("prjtask_id", &valid);
  if (valid)
  {
    _targetId = param.toInt();
    _targetType = "TASK";
    handleTargetType();
  }

  param = pParams.value("quhead_id", &valid);
  if (valid)
  {
    _targetId = param.toInt();
    _targetType = "QU";
    handleTargetType();
  }
  
  param = pParams.value("cohead_id", &valid);
  if (valid)
  {
    _targetId = param.toInt();
    _targetType = "SO";
    handleTargetType();
  }
  
  param = pParams.value("invchead_id", &valid);
  if (valid)
  {
    _targetId = param.toInt();
    _targetType = "INV";
    handleTargetType();
  }
  
  param = pParams.value("vend_id", &valid);
  if (valid)
  {
    _targetId = param.toInt();
    _targetType = "V";
    handleTargetType();
  }
  
  param = pParams.value("pohead_id", &valid);
  if (valid)
  {
    _targetId = param.toInt();
    _targetType = "PO";
    handleTargetType();
  }
  
  param = pParams.value("vohead_id", &valid);
  if (valid)
  {
    _targetId = param.toInt();
    _targetType = "VCH";
    handleTargetType();
  }

  param = pParams.value("charass_id", &valid);
  if (valid)
  {
    _charassid = param.toInt();
    populate();
  }

  param = pParams.value("mode", &valid);
  if (valid)
  {
    if (param.toString() == "new")
    {
      _mode = cNew;
    }
    else if (param.toString() == "edit")
    {
      _mode = cEdit;
    }
    else if (param.toString() == "view")
    {
      _mode = cView;

      _char->setEnabled(FALSE);
      _value->setEnabled(FALSE);
      _buttonBox->setStandardButtons(QDialogButtonBox::Close);
    }
  }

  param = pParams.value("showPrices", &valid);
  if (valid)
  {
    _listpriceLit->show();
    _listprice->show();
  }

  param = pParams.value("char_id", &valid);
  if (valid)
  {
    for (int i = 0; i < _char->model()->rowCount(); i++)
    {
      QModelIndex idx = _char->model()->index(i, 0);
      if (_char->model()->data(idx) == param)
        _char->setCurrentIndex(i);
    }
  }

  return NoError;
}

void characteristicAssignment::sSave()
{
  if(_targetType == "I")
  {
    if ( ((_stackedWidget->currentIndex() == characteristic::Text) && (_value->text().trimmed() == "")) ||
         ((_stackedWidget->currentIndex() == characteristic::List) && (_listValue->currentText() == "")) ||
         ((_stackedWidget->currentIndex() == characteristic::Date) && (_dateValue->date().toString() == "")) )
      {
          QMessageBox::information( this, tr("No Value Entered"),
                                    tr("You must enter a value before saving this Item Characteristic.") );
          return;
      }
  }

  XSqlQuery characteristicSave;
  if (_char->model()->data(_char->model()->index(_char->currentIndex(), 0)) == -1)
  {
    QMessageBox::information( this, tr("No Characteristic Selected"),
                              tr("You must select a Characteristic before saving this Characteristic Assignment.") );
    _char->setFocus();
    return;
  }
  if (_mode == cNew &&
      _template &&
      _stackedWidget->currentIndex() == characteristic::Date)
  {
    characteristicSave.prepare("SELECT charass_id "
              "FROM charass "
              "WHERE ((charass_char_id=:charass_char_id) "
              "  AND (charass_target_id=:charass_target_id) "
              "  AND (charass_target_type=:charass_target_type));");
    characteristicSave.bindValue(":charass_target_id", _targetId);
    characteristicSave.bindValue(":charass_target_type", _targetType);
    characteristicSave.bindValue(":charass_char_id", _char->model()->data(_char->model()->index(_char->currentIndex(), 0)));
    characteristicSave.exec();
    if (characteristicSave.first())
    {
      QMessageBox::critical(this, tr("Error"), tr("You can not use the same characteristic "
                                                  "for date type characteristics more than "
                                                  "once in this context."));
      return;
    }
  }

  if (_mode == cNew)
  {
    characteristicSave.exec("SELECT NEXTVAL('charass_charass_id_seq') AS charass_id;");
    if (characteristicSave.first())
    {
      _charassid = characteristicSave.value("charass_id").toInt();

      characteristicSave.prepare( "INSERT INTO charass "
                 "( charass_id, charass_target_id, charass_target_type, charass_char_id, charass_value, charass_price, charass_default ) "
                 "VALUES "
                 "( :charass_id, :charass_target_id, :charass_target_type, :charass_char_id, :charass_value, :charass_price, :charass_default );" );
    }
  }
  else if (_mode == cEdit)
    characteristicSave.prepare( "UPDATE charass "
               "SET charass_char_id=:charass_char_id, charass_value=:charass_value, "
               "charass_price=:charass_price, charass_default=:charass_default "
               "WHERE (charass_id=:charass_id);" );

  characteristicSave.bindValue(":charass_id", _charassid);
  characteristicSave.bindValue(":charass_target_id", _targetId);
  characteristicSave.bindValue(":charass_target_type", _targetType);
  characteristicSave.bindValue(":charass_char_id", _char->model()->data(_char->model()->index(_char->currentIndex(), 0)));
  if (_stackedWidget->currentIndex() == characteristic::Text)
    characteristicSave.bindValue(":charass_value", _value->text());
  else if (_stackedWidget->currentIndex() == characteristic::List)
    characteristicSave.bindValue(":charass_value", _listValue->currentText());
  else if (_stackedWidget->currentIndex() == characteristic::Date)
    characteristicSave.bindValue(":charass_value", _dateValue->date());
  characteristicSave.bindValue(":charass_price", _listprice->toDouble());
  characteristicSave.bindValue(":charass_default", QVariant(_default->isChecked()));
  characteristicSave.exec();

  done(_charassid);
}

void characteristicAssignment::sCheck()
{
  XSqlQuery characteristicCheck;
  if ((_mode == cNew) || (_char->model()->data(_char->model()->index(_char->currentIndex(), 0)) == -1))
  {
    characteristicCheck.prepare( "SELECT charass_id "
               "FROM charass "
               "WHERE ( (charass_target_type=:charass_target_id)"
               " AND (charass_target_id=:charass_target_id)"
               " AND (charass_char_id=:char_id) );" );
    characteristicCheck.bindValue(":charass_target_type", _targetType);
    characteristicCheck.bindValue(":charass_target_id", _targetId);
    characteristicCheck.bindValue(":char_id", _char->model()->data(_char->model()->index(_char->currentIndex(), 0)));
    characteristicCheck.exec();
    if (characteristicCheck.first())
    {
      _charassid = characteristicCheck.value("charass_id").toInt();
      _mode = cEdit;
      populate();
    }
  }
}

void characteristicAssignment::populate()
{
  XSqlQuery characteristicpopulate;
  characteristicpopulate.prepare( "SELECT charass.*, char_type "
             "FROM charass "
             " JOIN char ON (charass_char_id=char_id) "
             "WHERE (charass_id=:charass_id);" );
  characteristicpopulate.bindValue(":charass_id", _charassid);
  characteristicpopulate.exec();
  if (characteristicpopulate.first())
  {
    _targetId = characteristicpopulate.value("charass_target_id").toInt();
    _targetType = characteristicpopulate.value("charass_target_type").toString();
    handleTargetType();

    for (int i = 0; i < _char->model()->rowCount(); i++)
    {
      QModelIndex idx = _char->model()->index(i, 0);
      if (_char->model()->data(idx) == characteristicpopulate.value("charass_char_id").toInt())
        _char->setCurrentIndex(i);
    }
    _listprice->setDouble(characteristicpopulate.value("charass_price").toDouble());
    _default->setChecked(characteristicpopulate.value("charass_default").toBool());
    int chartype = _char->model()->data(_char->model()->index(_char->currentIndex(),16)).toInt();
    if (chartype == characteristic::Text)
      _value->setText(characteristicpopulate.value("charass_value").toString());
    else if (chartype == characteristic::List)
    {
      int idx = _listValue->findText(characteristicpopulate.value("charass_value").toString());
      _listValue->setCurrentIndex(idx);
    }
    else if (chartype == characteristic::Date)
      _dateValue->setDate(characteristicpopulate.value("charass_value").toDate());
  }
  else if (characteristicpopulate.lastError().type() != QSqlError::NoError)
  {
    systemError(this, characteristicpopulate.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

void characteristicAssignment::sHandleChar()
{
  QModelIndex midx = _char->model()->index(_char->currentIndex(), 16);
  int sidx = _char->model()->data(midx).toInt();

  _stackedWidget->setCurrentIndex(sidx);

  if (sidx == characteristic::Text) // Handle input mask for text
  {
    XSqlQuery mask;
    mask.prepare( "SELECT COALESCE(char_mask, '') AS char_mask,"
                  "       COALESCE(char_validator, '.*') AS char_validator "
                  "FROM char "
                  "WHERE (char_id=:char_id);" );
    mask.bindValue(":char_id", _char->model()->data(_char->model()->index(_char->currentIndex(), 0)).toInt());
    mask.exec();
    if (mask.first())
    {
      _value->setInputMask(mask.value("char_mask").toString());
      QRegExp rx(mask.value("char_validator").toString());
      QValidator *validator = new QRegExpValidator(rx, this);
      _value->setValidator(validator);
    }
    else if (mask.lastError().type() != QSqlError::NoError)
    {
      systemError(this, mask.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }
  else if (sidx == characteristic::List) // Handle options for list
  {
    QSqlQuery qry;
    qry.prepare("SELECT charopt_id, charopt_value "
                 "FROM charopt "
                 "WHERE (charopt_char_id=:char_id) "
                 "ORDER BY charopt_order, charopt_value;");
    qry.bindValue(":char_id", _char->model()->data(_char->model()->index(_char->currentIndex(), 0)).toInt());
    qry.exec();
    _listValue->populate(qry);
  }

  if (sidx != characteristic::Date && _template)
    _default->setVisible(true);
  else
  {
    _default->setVisible(false);
    _default->setChecked(false);
  }

}

void characteristicAssignment::handleTargetType()
{
  if((_targetType == "I") || (_targetType == "CT"))
    _template=true;
  else
    _default->hide();

  if(_targetType != "I")
    _listprice->hide();

  QString boolColumn;
  if ((_targetType == "C") || (_targetType == "CT"))
  {
    setWindowTitle(tr("Customer Characteristic"));
    boolColumn = "char_customers";
  }
  else if (_targetType == "I")
  {
    setWindowTitle(tr("Item Characteristic"));
    boolColumn = "char_items";
  }
  else if (_targetType == "CNTCT")
  {
    setWindowTitle(tr("Contact Characteristic"));
    boolColumn = "char_contacts";
  }
  else if (_targetType == "ADDR")
  {
    setWindowTitle(tr("Address Characteristic"));
    boolColumn = "char_addresses";
  }
  else if (_targetType == "CRMACCT")
  {
    setWindowTitle(tr("Account Characteristic"));
    boolColumn = "char_crmaccounts";
  }
  else if (_targetType == "LS")
  {
    setWindowTitle(tr("Lot Serial Characteristic"));
    boolColumn = "char_lotserial";
  }
  else if (_targetType == "LSR")
  {
    setWindowTitle(tr("Lot/Serial Registration Characteristic"));
    boolColumn = "char_lotserial";
  }
  else if (_targetType == "OPP")
  {
    setWindowTitle(tr("Opportunity Characteristic"));
    boolColumn = "char_opportunity";
  }
  else if (_targetType == "EMP")
  {
    setWindowTitle(tr("Employee Characteristic"));
    boolColumn = "char_employees";
  }
  else if (_targetType == "INCDT")
  {
    setWindowTitle(tr("Incident Characteristic"));
    boolColumn = "char_incidents";
  }
  else if (_targetType == "PROJ")
  {
    setWindowTitle(tr("Project Characteristic"));
    boolColumn = "char_projects";
  }
  else if (_targetType == "TASK")
  {
    setWindowTitle(tr("Project Task Characteristic"));
    boolColumn = "char_tasks";
  }
  else if (_targetType == "QU")
  {
    setWindowTitle(tr("Quote Characteristic"));
    boolColumn = "char_quotes";
  }
  else if (_targetType == "SO")
  {
    setWindowTitle(tr("Sales Order Characteristic"));
    boolColumn = "char_salesorders";
  }
  else if (_targetType == "INV")
  {
    setWindowTitle(tr("Invoice Characteristic"));
    boolColumn = "char_invoices";
  }
  else if (_targetType == "V")
  {
    setWindowTitle(tr("Vendor Characteristic"));
    boolColumn = "char_vendors";
  }
  else if (_targetType == "PO")
  {
    setWindowTitle(tr("Purchase Order Characteristic"));
    boolColumn = "char_purchaseorders";
  }
  else if (_targetType == "VCH")
  {
    setWindowTitle(tr("Voucher Characteristic"));
    boolColumn = "char_vouchers";
  }

  QSqlTableModel *model = new QSqlTableModel;
  model->setTable("char");
  model->setFilter(boolColumn);
  model->setSort(1, Qt::AscendingOrder);
  model->setSort(17, Qt::AscendingOrder);
  model->select();
  _char->setModel(model);
  _char->setModelColumn(1);
  sHandleChar();
}

