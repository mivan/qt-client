/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "sale.h"

#include <QVariant>
#include <QMessageBox>

sale::sale(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  setupUi(this);

  _saleid = -1;

  connect(_save, SIGNAL(clicked()), this, SLOT(sSave()));
  connect(_close, SIGNAL(clicked()), this, SLOT(reject()));

  _ipshead->populate( QString( "SELECT ipshead_id, ipshead_name "
                               "FROM ipshead "
                               "ORDER BY ipshead_name;" ) );
}

sale::~sale()
{
  // no need to delete child widgets, Qt does it all for us
}

void sale::languageChange()
{
  retranslateUi(this);
}

enum SetResponse sale::set(const ParameterList &pParams)
{
  XDialog::set(pParams);
  QVariant param;
  bool     valid;

  param = pParams.value("sale_id", &valid);
  if (valid)
  {
    _saleid = param.toInt();
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

      _name->setEnabled(FALSE);
      _description->setEnabled(FALSE);
      _dates->setEnabled(FALSE);
      _ipshead->setEnabled(FALSE);
      _close->setText(tr("&Close"));
      _save->hide();
    }
  }

  return NoError;
}

void sale::sSave()
{
  XSqlQuery saleSave;
  _name->setText(_name->text().trimmed());

  if (_name->text().length() == 0)
  {
    QMessageBox::critical( this, tr("Enter Sale Name"),
                           tr("You must enter a name for this Sale before saving it.") );
    _name->setFocus();
    return;
  }

  saleSave.prepare("SELECT sale_id"
            "  FROM sale"
            " WHERE((sale_name=:sale_name)"
            "   AND (sale_id != :sale_id))");
  saleSave.bindValue(":sale_id", _saleid);
  saleSave.bindValue(":sale_name", _name->text().trimmed());
  saleSave.exec();
  if(saleSave.first())
  {
    QMessageBox::critical( this, tr("Cannot Save Sale"),
                           tr("You cannot enter a duplicate name for this Sale before saving it.") );
    _name->setFocus();
    return;
  }

  if (!_dates->startDate().isValid())
  {
    QMessageBox::critical( this, tr("Enter Start Date"),
                           tr("You must enter a start date for this Sale.") );
    _dates->setFocus();
    return;
  }

  if (!_dates->endDate().isValid())
  {
    QMessageBox::critical( this, tr("Enter End Date"),
                           tr("You must enter an end date for this Sale.") );
    _dates->setFocus();
    return;
  }

  if (_dates->endDate() < _dates->startDate())
  {
    QMessageBox::critical( this, tr("Invalid End Date"),
                           tr("The start date cannot be earlier than the end date.") );
    _dates->setFocus();
    return;
  }

  if (_mode == cNew)
  {
    saleSave.exec("SELECT NEXTVAL('sale_sale_id_seq') AS _sale_id;");
    if (saleSave.first())
      _saleid = saleSave.value("_sale_id").toInt();
    else
    {
      systemError(this, tr("A System Error occurred at %1::%2.")
                        .arg(__FILE__)
                        .arg(__LINE__) );
      return;
    }

    saleSave.prepare( "INSERT INTO sale "
               "( sale_id, sale_name, sale_descrip,"
               "  sale_ipshead_id, sale_startdate, sale_enddate ) "
               "VALUES "
               "( :sale_id, :sale_name, :sale_descrip,"
               "  :sale_ipshead_id, :sale_startdate, :sale_enddate );" );
  }
  else
    saleSave.prepare( "UPDATE sale "
               "SET sale_name=:sale_name, sale_descrip=:sale_descrip,"
               "    sale_ipshead_id=:sale_ipshead_id,"
               "    sale_startdate=:sale_startdate, sale_enddate=:sale_enddate "
               "WHERE (sale_id=:sale_id);" );

  saleSave.bindValue(":sale_id", _saleid);
  saleSave.bindValue(":sale_name", _name->text());
  saleSave.bindValue(":sale_descrip", _description->text());
  saleSave.bindValue(":sale_ipshead_id", _ipshead->id());
  saleSave.bindValue(":sale_startdate", _dates->startDate());
  saleSave.bindValue(":sale_enddate", _dates->endDate());
  saleSave.exec();

  done(_saleid);
}

void sale::populate()
{
  XSqlQuery salepopulate;
  salepopulate.prepare( "SELECT sale_name, sale_descrip, sale_ipshead_id,"
             "       sale_startdate, sale_enddate "
             "FROM sale "
             "WHERE (sale_id=:sale_id);" );
  salepopulate.bindValue(":sale_id", _saleid);
  salepopulate.exec();
  if (salepopulate.first())
  {
    _name->setText(salepopulate.value("sale_name").toString());
    _description->setText(salepopulate.value("sale_descrip").toString());
    _dates->setStartDate(salepopulate.value("sale_startdate").toDate());
    _dates->setEndDate(salepopulate.value("sale_enddate").toDate());
    _ipshead->setId(salepopulate.value("sale_ipshead_id").toInt());
  }
}

