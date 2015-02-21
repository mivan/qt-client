/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "arAccountAssignments.h"

#include <QMessageBox>
#include <QVariant>

#include <parameter.h>
#include <openreports.h>
#include "arAccountAssignment.h"
#include "guiclient.h"

arAccountAssignments::arAccountAssignments(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

  connect(_print, SIGNAL(clicked()), this, SLOT(sPrint()));
  connect(_new, SIGNAL(clicked()), this, SLOT(sNew()));
  connect(_edit, SIGNAL(clicked()), this, SLOT(sEdit()));
  connect(_delete, SIGNAL(clicked()), this, SLOT(sDelete()));
  connect(_view, SIGNAL(clicked()), this, SLOT(sView()));
  
  _araccnt->addColumn(tr("Customer Type"),   -1,  Qt::AlignCenter, true, "custtypecode");
  _araccnt->addColumn(tr("A/R Account"),     120, Qt::AlignLeft,  true, "araccnt");
  _araccnt->addColumn(tr("Prepaid Account"), 120, Qt::AlignLeft,  true, "prepaidaccnt");
  _araccnt->addColumn(tr("Freight Account"), 120, Qt::AlignLeft,  true, "freightaccnt");
  _araccnt->addColumn(tr("Discount Account"),120, Qt::AlignLeft,  true, "discountaccnt");

  if(_metrics->boolean("EnableCustomerDeposits"))
    _araccnt->addColumn(tr("Deferred Rev. Account"), 120, Qt::AlignLeft, true, "deferredaccnt");

  if (_privileges->check("MaintainSalesAccount"))
  {
    connect(_araccnt, SIGNAL(valid(bool)), _edit, SLOT(setEnabled(bool)));
    connect(_araccnt, SIGNAL(valid(bool)), _delete, SLOT(setEnabled(bool)));
    connect(_araccnt, SIGNAL(itemSelected(int)), _edit, SLOT(animateClick()));
  }
  else
  {
    _new->setEnabled(FALSE);
    connect(_araccnt, SIGNAL(itemSelected(int)), _view, SLOT(animateClick()));
  }

  sFillList();
}

arAccountAssignments::~arAccountAssignments()
{
  // no need to delete child widgets, Qt does it all for us
}

void arAccountAssignments::languageChange()
{
  retranslateUi(this);
}

void arAccountAssignments::sPrint()
{
  orReport report("FreightAccountAssignmentsMasterList");
  if (report.isValid())
    report.print();
  else
    report.reportError(this);
}

void arAccountAssignments::sNew()
{
  ParameterList params;
  params.append("mode", "new");

  arAccountAssignment newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void arAccountAssignments::sEdit()
{
  ParameterList params;
  params.append("mode", "edit");
  params.append("araccnt_id", _araccnt->id());

  arAccountAssignment newdlg(this, "", TRUE);
  newdlg.set(params);

  if (newdlg.exec() != XDialog::Rejected)
    sFillList();
}

void arAccountAssignments::sView()
{
  ParameterList params;
  params.append("mode", "view");
  params.append("araccnt_id", _araccnt->id());

  arAccountAssignment newdlg(this, "", TRUE);
  newdlg.set(params);
  newdlg.exec();
}

void arAccountAssignments::sDelete()
{
  XSqlQuery arDelete;
  arDelete.prepare( "DELETE FROM araccnt "
             "WHERE (araccnt_id=:araccnt_id);" );
  arDelete.bindValue(":araccnt_id", _araccnt->id());
  arDelete.exec();
  sFillList();
}

void arAccountAssignments::sFillList()
{
  _araccnt->populate( "SELECT araccnt_id,"
                      "       CASE WHEN araccnt_custtype_id=-1 THEN araccnt_custtype"
                      "            ELSE (SELECT custtype_code FROM custtype WHERE (custtype_id=araccnt_custtype_id))"
                      "       END AS custtypecode,"
                      "       CASE WHEN (araccnt_ar_accnt_id = -1) THEN 'N/A' "
                      "            ELSE formatGLAccount(araccnt_ar_accnt_id) END AS araccnt,"
                      "       CASE WHEN (araccnt_prepaid_accnt_id = -1) THEN 'N/A' "
                      "            ELSE formatGLAccount(araccnt_prepaid_accnt_id) END AS prepaidaccnt,"
                      "       CASE WHEN (araccnt_freight_accnt_id = -1) THEN 'N/A' "
                      "            ELSE formatGLAccount(araccnt_freight_accnt_id) END AS freightaccnt,"
                      "       CASE WHEN (araccnt_deferred_accnt_id = -1) THEN 'N/A' "
                      "            ELSE formatGLAccount(araccnt_deferred_accnt_id) END AS deferredaccnt, "
                      "       CASE WHEN (araccnt_discount_accnt_id = -1) THEN 'N/A' "
                      "            ELSE formatGLAccount(araccnt_discount_accnt_id) END AS discountaccnt  "
                      "  FROM araccnt "
                      " ORDER BY custtypecode;" );
}
