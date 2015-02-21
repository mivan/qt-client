/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "bankAdjustment.h"

#include <QVariant>
#include <QMessageBox>
#include <QSqlError>
#include <QValidator>

bankAdjustment::bankAdjustment(QWidget* parent, const char* name, Qt::WFlags fl)
  : XWidget(parent, name, fl)
{
  setupUi(this);


  // signals and slots connections
  connect(_close, SIGNAL(clicked()), this, SLOT(close()));
  connect(_save, SIGNAL(clicked()), this, SLOT(sSave()));
  connect(_bankaccnt, SIGNAL(newID(int)), this, SLOT(sBankAccount(int)));

  _bankaccnt->populate("SELECT bankaccnt_id,"
                       "       (bankaccnt_name || '-' || bankaccnt_descrip),"
		       "       bankaccnt_name "
                       "FROM bankaccnt "
                       "ORDER BY bankaccnt_name;");
  _bankadjtype->populate("SELECT bankadjtype_id,"
                         "       (bankadjtype_name || '-' || bankadjtype_descrip),"
			 "       bankadjtype_name "
                         "FROM bankadjtype "
                         "ORDER BY bankadjtype_name;");

  _bankadjid = -1;
}

bankAdjustment::~bankAdjustment()
{
  // no need to delete child widgets, Qt does it all for us
}

void bankAdjustment::languageChange()
{
  retranslateUi(this);
}

SetResponse bankAdjustment::set( const ParameterList & pParams )
{
  XWidget::set(pParams);
  QVariant param;
  bool     valid;

  param = pParams.value("bankaccnt_id", &valid);
  if (valid)
  {
    _bankaccnt->setId(param.toInt());
    //_bankaccnt->setEnabled(FALSE);
  }

  param = pParams.value("bankadj_id", &valid);
  if(valid)
  {
    _bankadjid = param.toInt();
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

      _bankaccnt->setEnabled(FALSE);
      _bankadjtype->setEnabled(FALSE);
      _date->setEnabled(FALSE);
      _docNumber->setEnabled(FALSE);
      _amount->setEnabled(FALSE);
      _notes->setReadOnly(TRUE);
      _save->hide();
    }
  }

  return NoError;
}

void bankAdjustment::sSave()
{
  XSqlQuery bankSave;
  if (!_date->isValid())
  {
    QMessageBox::information( this, tr("Cannot Post Bank Adjustment"),
                          tr("You must enter a date before posting this Bank Adjustment.") );
    _date->setFocus();
    return;
  }

  if (_amount->isZero())
  {
    QMessageBox::information( this, tr("Cannot Post Bank Adjustment"),
                          tr("You must enter an amount before posting this Bank Adjustment.") );
    _amount->setFocus();
    return;
  }

  if (_mode == cNew)
    bankSave.prepare( "INSERT INTO bankadj "
                   "(bankadj_bankaccnt_id, bankadj_bankadjtype_id,"
                   " bankadj_date, bankadj_docnumber, bankadj_amount, "
		   " bankadj_notes, bankadj_curr_id ) "
                   "VALUES "
                   "(:bankaccnt_id, :bankadjtype_id,"
                   " :date, :docnumber, :amount, :notes, :curr_id);" );
  else if (_mode == cEdit)
  {
    bankSave.prepare ( "UPDATE bankadj "
                    "SET bankadj_bankaccnt_id=:bankaccnt_id,"
                    " bankadj_bankadjtype_id=:bankadjtype_id,"
                    " bankadj_date=:date,"
                    " bankadj_docnumber=:docnumber,"
                    " bankadj_amount=:amount,"
                    " bankadj_notes=:notes, "
                    " bankadj_curr_id=:curr_id "
                    "WHERE ((bankadj_id=:bankadj_id)"
                    " AND (NOT bankadj_posted) ); ");
    bankSave.bindValue(":bankadj_id", _bankadjid);
  }
    
  bankSave.bindValue(":bankaccnt_id", _bankaccnt->id());
  bankSave.bindValue(":bankadjtype_id", _bankadjtype->id());
  bankSave.bindValue(":date", _date->date());
  bankSave.bindValue(":docnumber", _docNumber->text());
  bankSave.bindValue(":amount", _amount->localValue());
  bankSave.bindValue(":notes",   _notes->toPlainText());
  bankSave.bindValue(":curr_id", _amount->id());
  
  if(!bankSave.exec())
  {
    systemError(this, tr("A System Error occurred at %1::%2.")
                      .arg(__FILE__)
                      .arg(__LINE__) );
    return;
  }

  omfgThis->sBankAdjustmentsUpdated(_bankadjid, TRUE);

  close();
}

void bankAdjustment::populate()
{
  XSqlQuery bankpopulate;
  bankpopulate.prepare( "SELECT bankadj_bankaccnt_id, bankadj_bankadjtype_id,"
             "       bankadj_date, bankadj_docnumber, bankadj_amount,"
             "       bankadj_notes, bankadj_curr_id "
             "FROM bankadj "
             "WHERE (bankadj_id=:bankadj_id);" );
  bankpopulate.bindValue(":bankadj_id", _bankadjid);
  bankpopulate.exec();
  if(bankpopulate.first())
  {
    _bankaccnt->setId(bankpopulate.value("bankadj_bankaccnt_id").toInt());
    _bankadjtype->setId(bankpopulate.value("bankadj_bankadjtype_id").toInt());
    _date->setDate(bankpopulate.value("bankadj_date").toDate());
    _docNumber->setText(bankpopulate.value("bankadj_docnumber").toString());
    _amount->set(bankpopulate.value("bankadj_amount").toDouble(),
		 bankpopulate.value("bankadj_curr_id").toInt(),
		 bankpopulate.value("bankadj_date").toDate(), false);
    _notes->setText(bankpopulate.value("bankadj_notes").toString());
  }
}

void bankAdjustment::sBankAccount(int accountId)
{
    XSqlQuery bankQ;
    bankQ.prepare("SELECT bankaccnt_curr_id "
		  "FROM bankaccnt WHERE bankaccnt_id = :accntId;");
    bankQ.bindValue(":accntId", accountId);
    bankQ.exec();
    if (bankQ.first())
	_amount->setId(bankQ.value("bankaccnt_curr_id").toInt());
    if (bankQ.lastError().type() != QSqlError::NoError)
	QMessageBox::critical(this, tr("A System Error occurred at %1::%2.")
			      .arg(__FILE__)
			      .arg(__LINE__),
	bankQ.lastError().databaseText());
}
