/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "accountingPeriods.h"

#include <QAction>
#include <QMenu>
#include <QSqlError>
#include <QMessageBox>

#include <openreports.h>

#include "accountingPeriod.h"
#include "storedProcErrorLookup.h"
#include "unpostedGLTransactions.h"

accountingPeriods::accountingPeriods(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

  connect(_period, SIGNAL(populateMenu(QMenu*,QTreeWidgetItem*)), this, SLOT(sPopulateMenu(QMenu*, QTreeWidgetItem *)));
  connect(_edit, SIGNAL(clicked()), this, SLOT(sEdit()));
  connect(_view, SIGNAL(clicked()), this, SLOT(sView()));
  connect(_delete, SIGNAL(clicked()), this, SLOT(sDelete()));
  connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_new, SIGNAL(clicked()), this, SLOT(sNew()));
  connect(_closePeriod, SIGNAL(clicked()), this, SLOT(sClosePeriod()));
  connect(_freezePeriod, SIGNAL(clicked()), this, SLOT(sFreezePeriod()));
  connect(_year, SIGNAL(newID(int)), this, SLOT(sFillList()));
  
  _period->addColumn(tr("Name"),            -1, Qt::AlignLeft,   true, "period_name");
  _period->addColumn(tr("Start"),  _dateColumn, Qt::AlignCenter, true, "period_start");
  _period->addColumn(tr("End"),    _dateColumn, Qt::AlignCenter, true, "period_end");
  _period->addColumn(tr("Number"),   _ynColumn, Qt::AlignRight,  false, "period_number");
  _period->addColumn(tr("Qtr"),  _statusColumn, Qt::AlignCenter, true, "qtr");
  _period->addColumn(tr("Year"),   _dateColumn, Qt::AlignCenter, true, "year");
  _period->addColumn(tr("Closed"), _ynColumn+3, Qt::AlignCenter, true, "closed");
  _period->addColumn(tr("Frozen"), _ynColumn+3, Qt::AlignCenter, true, "frozen");

  if (_privileges->check("MaintainAccountingPeriods"))
  {
    connect(_period, SIGNAL(valid(bool)), _edit, SLOT(setEnabled(bool)));
    connect(_period, SIGNAL(valid(bool)), _delete, SLOT(setEnabled(bool)));
    connect(_period, SIGNAL(itemSelected(int)), _edit, SLOT(animateClick()));
  }
  else
  {
    _new->setEnabled(FALSE);
    connect(_period, SIGNAL(itemSelected(int)), _view, SLOT(animateClick()));
  }

  connect(_period, SIGNAL(valid(bool)), _closePeriod, SLOT(setEnabled(bool)));
  connect(_period, SIGNAL(valid(bool)), _freezePeriod, SLOT(setEnabled(bool)));

  sFillList();
}

accountingPeriods::~accountingPeriods()
{
  // no need to delete child widgets, Qt does it all for us
}

void accountingPeriods::languageChange()
{
  retranslateUi(this);
}

void accountingPeriods::sPopulateMenu(QMenu *pMenu, QTreeWidgetItem *pSelected)
{
  QAction *menuItem;
  int altId = ((XTreeWidgetItem *)pSelected)->altId();

  if (altId == 0)
  {
    menuItem = pMenu->addAction(tr("Edit..."), this, SLOT(sEdit()));
    menuItem->setEnabled(_privileges->check("MaintainAccountingPeriods"));
  }

  menuItem = pMenu->addAction(tr("View..."), this, SLOT(sView()));

  if (altId == 0)
  {
    menuItem = pMenu->addAction(tr("Delete..."), this, SLOT(sDelete()));
    menuItem->setEnabled(_privileges->check("MaintainAccountingPeriods"));
  }

  pMenu->addSeparator();

  if (altId == 0)
  {
    menuItem = pMenu->addAction(tr("Freeze..."), this, SLOT(sFreezePeriod()));

    menuItem = pMenu->addAction(tr("Close..."), this, SLOT(sClosePeriod()));
  }
  else if (altId == 1)
  {
    menuItem = pMenu->addAction(tr("Thaw..."), this, SLOT(sThawPeriod()));

    menuItem = pMenu->addAction(tr("Close..."), this, SLOT(sClosePeriod()));
  }
  else if (altId == 2 || altId == 3)
  {
    menuItem = pMenu->addAction(tr("Open..."), this, SLOT(sOpenPeriod()));
  }
}

void accountingPeriods::sNew()
{
  ParameterList params;
  params.append("mode", "new");

  accountingPeriod newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void accountingPeriods::sEdit()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("period_id", _period->id());

  accountingPeriod newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void accountingPeriods::sView()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("period_id", _period->id());

  accountingPeriod newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void accountingPeriods::sDelete()
{
  XSqlQuery deleteAccounting;
  deleteAccounting.prepare("SELECT deleteAccountingPeriod(:period_id) AS result;");
  deleteAccounting.bindValue(":period_id", _period->id());
  deleteAccounting.exec();
  if (deleteAccounting.first())
  {
    int result = deleteAccounting.value("result").toInt();
    if (result < 0)
    {
      systemError(this, storedProcErrorLookup("deleteAccountingPeriod",
						       result));
      return;
    }
  }
  else if (deleteAccounting.lastError().type() != QSqlError::NoError)
  {
    systemError(this, deleteAccounting.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
  sFillList();
}

void accountingPeriods::sClosePeriod()
{
  XSqlQuery closeAccounting;
  closeAccounting.prepare("SELECT closeAccountingPeriod(:period_id) AS result;");
  closeAccounting.bindValue(":period_id", _period->id());
  closeAccounting.exec();
  if (closeAccounting.first())
  {
    int result = closeAccounting.value("result").toInt();
    if (result < 0)
    {
      systemError(this, storedProcErrorLookup("closeAccountingPeriod", result),
		  __FILE__, __LINE__);
      return;
    }
  }
  else if (closeAccounting.lastError().type() != QSqlError::NoError)
  {
    systemError(this, closeAccounting.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  sFillList();
}

void accountingPeriods::sOpenPeriod()
{
  bool reallyOpen = false;

  XSqlQuery openAccounting;
  openAccounting.prepare("SELECT COUNT(gltrans_sequence) AS count "
            "FROM gltrans, period "
            "WHERE ( (NOT gltrans_posted) "
            "AND (gltrans_date BETWEEN period_start AND period_end) "
            "AND (period_id=:period_id) );");
  openAccounting.bindValue(":period_id", _period->id());
  openAccounting.exec();
  if (openAccounting.first())
  {
    if (openAccounting.value("count").toInt() <= 0)
      reallyOpen = true;
    else
    {
      ParameterList params;

      unpostedGLTransactions newdlg(this, "", true);
      params.append("period_id", _period->id());
      newdlg.set(params);

      reallyOpen = (newdlg.exec() == XDialog::Accepted);
    }
  }
  else if (openAccounting.lastError().type() != QSqlError::NoError)
  {
    systemError(this, openAccounting.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  if (reallyOpen)
  {
    openAccounting.prepare("SELECT openAccountingPeriod(:period_id) AS result;");
    openAccounting.bindValue(":period_id", _period->id());
    openAccounting.exec();
    if (openAccounting.first())
    {
      int result = openAccounting.value("result").toInt();
      if (result < 0)
      {
	systemError(this, storedProcErrorLookup("openAccountingPeriod", result),
		    __FILE__, __LINE__);
	return;
      }
    }
    else if (openAccounting.lastError().type() != QSqlError::NoError)
    {
      systemError(this, openAccounting.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }

    sFillList();
  }
}

void accountingPeriods::sFreezePeriod()
{
  XSqlQuery freezeAccounting;
  freezeAccounting.prepare("SELECT freezeAccountingPeriod(:period_id) AS result;");
  freezeAccounting.bindValue(":period_id", _period->id());
  freezeAccounting.exec();
  if (freezeAccounting.first())
  {
    int result = freezeAccounting.value("result").toInt();
    if (result < 0)
    {
      systemError(this, storedProcErrorLookup("freezeAccountingPeriod", result),
		  __FILE__, __LINE__);
      return;
    }
  }
  else if (freezeAccounting.lastError().type() != QSqlError::NoError)
  {
    systemError(this, freezeAccounting.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  sFillList();
}

void accountingPeriods::sThawPeriod()
{
  XSqlQuery thawAccounting;
  thawAccounting.prepare("SELECT thawAccountingPeriod(:period_id) AS result;");
  thawAccounting.bindValue(":period_id", _period->id());
  thawAccounting.exec();
  if (thawAccounting.first())
  {
    int result = thawAccounting.value("result").toInt();
    if (result < 0)
    {
      systemError(this, storedProcErrorLookup("thawAccountingPeriod", result),
		  __FILE__, __LINE__);
      return;
    }
  }
  else if (thawAccounting.lastError().type() != QSqlError::NoError)
  {
    systemError(this, thawAccounting.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  sFillList();
}

void accountingPeriods::sPrint()
{
  orReport report("AccountingPeriodsMasterList");
  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}

void accountingPeriods::sFillList()
{

  XSqlQuery periodsList;
  periodsList.prepare("SELECT period_id,"
                     "       CASE WHEN ( (NOT period_freeze) AND (NOT period_closed) ) THEN 0"
                     "            WHEN ( (period_freeze) AND (NOT period_closed) ) THEN 1"
                     "            WHEN ( (NOT period_freeze) AND (period_closed) ) THEN 2"
                     "            WHEN ( (period_freeze) AND (period_closed) ) THEN 3"
                     "            ELSE 0"
                     "       END,"
                     "       period_number, period_name,"
                     "       period_start, period_end,"
                     "       COALESCE(to_char(period_quarter,'9'),'?') AS qtr,"
                     "       COALESCE(to_char(EXTRACT(year FROM yearperiod_end),'9999'),'?') AS year,"
                     "       formatBoolYN(period_closed) AS closed,"
                     "       formatBoolYN(period_freeze) AS frozen "
                     "  FROM period LEFT OUTER JOIN yearperiod "
                     "    ON (period_yearperiod_id=yearperiod_id) "
		     "  WHERE CASE WHEN (:fiscalYear > 0) THEN  "
		     "     	     (period_yearperiod_id = :fiscalYear) "
		     "	 	   ELSE (1=1) END"
                     " ORDER BY period_start;");
    periodsList.bindValue(":fiscalYear", _year->id());
    periodsList.exec();
    
    if (periodsList.lastError().type() != QSqlError::NoError)
    {
      systemError(this, periodsList.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }

  _period->clear();
  _period->populate(periodsList, TRUE );
}

