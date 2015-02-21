/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "bankAccounts.h"

#include <QMenu>
#include <QMessageBox>
#include <QSqlError>
#include <QVariant>

#include <openreports.h>

#include "bankAccount.h"

bankAccounts::bankAccounts(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

  connect(_bankaccnt, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*,int)), this, SLOT(sPopulateMenu(QMenu*)));
  connect(_delete,        SIGNAL(clicked()), this, SLOT(sDelete()));
  connect(_edit,          SIGNAL(clicked()), this, SLOT(sEdit()));
  connect(_new,           SIGNAL(clicked()), this, SLOT(sNew()));
  connect(_print,         SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_view,          SIGNAL(clicked()), this, SLOT(sView()));
  
  _bankaccnt->addColumn(tr("Name"),        _itemColumn, Qt::AlignLeft, true, "bankaccnt_name"  );
  _bankaccnt->addColumn(tr("Description"),          -1, Qt::AlignLeft, true, "bankaccnt_descrip"  );
  _bankaccnt->addColumn(tr("Type"),        _dateColumn, Qt::AlignCenter,true, "type");
  _bankaccnt->addColumn(tr("A/P"),           _ynColumn, Qt::AlignCenter,true, "bankaccnt_ap");
  _bankaccnt->addColumn(tr("A/R"),           _ynColumn, Qt::AlignCenter,true, "bankaccnt_ar");
  _bankaccnt->addColumn(tr("Currency"),_currencyColumn, Qt::AlignCenter,true, "currabbr");

  if (omfgThis->singleCurrency())
      _bankaccnt->hideColumn("currabbr");

  if (_privileges->check("MaintainBankAccounts"))
  {
    connect(_bankaccnt, SIGNAL(valid(bool)), _edit, SLOT(setEnabled(bool)));
    connect(_bankaccnt, SIGNAL(valid(bool)), _delete, SLOT(setEnabled(bool)));
    connect(_bankaccnt, SIGNAL(itemSelected(int)), _edit, SLOT(animateClick()));
  }
  else
  {
    _new->setEnabled(FALSE);
    connect(_bankaccnt, SIGNAL(itemSelected(int)), _view, SLOT(animateClick()));
  }

  sFillList();
}

bankAccounts::~bankAccounts()
{
  // no need to delete child widgets, Qt does it all for us
}

void bankAccounts::languageChange()
{
  retranslateUi(this);
}

void bankAccounts::sNew()
{
  ParameterList params;
  params.append("mode", "new");

  bankAccount newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void bankAccounts::sEdit()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("bankaccnt_id", _bankaccnt->id());

  bankAccount newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void bankAccounts::sView()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("bankaccnt_id", _bankaccnt->id());

  bankAccount newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void bankAccounts::sFillList()
{
  XSqlQuery bankFillList;
  bankFillList.prepare( "SELECT bankaccnt_id, bankaccnt_name, bankaccnt_descrip,"
             "       CASE WHEN (bankaccnt_type='K') THEN :checking"
             "            WHEN (bankaccnt_type='C') THEN :cash"
             "            WHEN (bankaccnt_type='R') THEN :creditcard"
             "            ELSE '?'"
             "       END AS type,"
             "       bankaccnt_ap, bankaccnt_ar, "
	     "       currConcat(bankaccnt_curr_id) AS currabbr "
             "FROM bankaccnt "
             "ORDER BY bankaccnt_name;" );
  bankFillList.bindValue(":checking", tr("Checking"));
  bankFillList.bindValue(":cash", tr("Cash"));
  bankFillList.bindValue(":creditcard", tr("Credit Card"));
  bankFillList.exec();
  _bankaccnt->populate(bankFillList);
  if (bankFillList.lastError().type() != QSqlError::NoError)
  {
    systemError(this, bankFillList.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

void bankAccounts::sDelete()
{
  XSqlQuery bankDelete;
  // TODO: put all of this into a stored proc
  bankDelete.prepare( "SELECT cashrcpt_id "
             "FROM cashrcpt "
             "WHERE (cashrcpt_bankaccnt_id=:bankaccnt_id) "
             "LIMIT 1;" );
  bankDelete.bindValue(":bankaccnt_id", _bankaccnt->id());
  bankDelete.exec();
  if (bankDelete.first())
  {
    QMessageBox::critical( this, tr("Cannot Delete Bank Account"),
                           tr("<p>The selected Bank Account cannot be deleted "
			      "as Cash Receipts have been posted against it."));
    return;
  }

  bankDelete.prepare( "SELECT checkhead_id "
             "FROM checkhead "
             "WHERE (checkhead_bankaccnt_id=:bankaccnt_id) "
             "LIMIT 1;" );
  bankDelete.bindValue(":bankaccnt_id", _bankaccnt->id());
  bankDelete.exec();
  if (bankDelete.first())
  {
    QMessageBox::critical( this, tr("Cannot Delete Bank Account"),
                           tr("<p>The selected Bank Account cannot be delete "
			      "as Checks have been posted against it.") );
    return;
  }

  bankDelete.prepare( "DELETE FROM bankaccnt "
             "WHERE (bankaccnt_id=:bankaccnt_id);" );
  bankDelete.bindValue(":bankaccnt_id", _bankaccnt->id());
  bankDelete.exec();
  if (bankDelete.lastError().type() != QSqlError::NoError)
  {
    systemError(this, bankDelete.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
  sFillList();
  omfgThis->sBankAccountsUpdated();
}

void bankAccounts::sPopulateMenu( QMenu * )
{
}

void bankAccounts::sPrint()
{
  orReport report("BankAccountsMasterList");
  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}

