/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "applyAPCreditMemo.h"

#include <QVariant>
#include <QMessageBox>
#include <QSqlError>

#include "apCreditMemoApplication.h"
#include "storedProcErrorLookup.h"

applyAPCreditMemo::applyAPCreditMemo(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  setupUi(this);

  connect(_apply,          SIGNAL(clicked()), this, SLOT(sApply()));
  connect(_applyToBalance, SIGNAL(clicked()), this, SLOT(sApplyBalance()));
  connect(_available, SIGNAL(idChanged(int)), this, SLOT(sPriceGroup()));
  connect(_clear,          SIGNAL(clicked()), this, SLOT(sClear()));
  connect(_buttonBox,      SIGNAL(accepted()), this, SLOT(sPost()));
  connect(_buttonBox,      SIGNAL(rejected()), this, SLOT(reject()));

  _buttonBox->button(QDialogButtonBox::Save)->setText(tr("Post"));

  _captive = FALSE;

  _apopen->addColumn(tr("Doc. Type"),   _docTypeColumn,  Qt::AlignCenter,true, "doctype");
  _apopen->addColumn(tr("Doc. Number"), -1,              Qt::AlignCenter,true, "apopen_docnumber");
  _apopen->addColumn(tr("Doc. Date"),   _dateColumn,     Qt::AlignCenter,true, "apopen_docdate");
  _apopen->addColumn(tr("Due Date"),    _dateColumn,     Qt::AlignCenter,true, "apopen_duedate");
  _apopen->addColumn(tr("Open"),        _moneyColumn,    Qt::AlignRight, true, "openamount");
  _apopen->addColumn(tr("Currency"),    _currencyColumn, Qt::AlignLeft,  true, "opencurrabbr");
  _apopen->addColumn(tr("Applied"),     _moneyColumn,    Qt::AlignRight, true, "apcreditapply_amount");
  _apopen->addColumn(tr("Currency"),    _currencyColumn, Qt::AlignLeft,  true, "appliedcurrabbr");

  if (_privileges->check("ApplyAPMemos"))
      connect(_apopen, SIGNAL(valid(bool)), _apply, SLOT(setEnabled(bool)));
  if (omfgThis->singleCurrency())
  {
    _apopen->hideColumn("opencurrabbr");
    _apopen->hideColumn("appliedcurrabbr");
  }

  _vend->setReadOnly(TRUE);
  sPriceGroup();
  adjustSize();
}

applyAPCreditMemo::~applyAPCreditMemo()
{
  // no need to delete child widgets, Qt does it all for us
}

void applyAPCreditMemo::languageChange()
{
  retranslateUi(this);
}

enum SetResponse applyAPCreditMemo::set(const ParameterList &pParams)
{
  XDialog::set(pParams);
  QVariant param;
  bool     valid;

  param = pParams.value("apopen_id", &valid);
  if (valid)
  {
    _captive = TRUE;
    _apopenid = param.toInt();
    populate();
  }

  return NoError;
}

void applyAPCreditMemo::sPost()
{
  XSqlQuery applyPost;
  applyPost.exec("BEGIN;");
  if (applyPost.lastError().type() != QSqlError::NoError)
  {
    systemError(this, applyPost.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  applyPost.prepare("SELECT postAPCreditMemoApplication(:apopen_id) AS result;");
  applyPost.bindValue(":apopen_id", _apopenid);
  applyPost.exec();
  if (applyPost.first())
  {
    int result = applyPost.value("result").toInt();
    if (result < 0)
    {
      applyPost.exec("ROLLBACK;");
      systemError(this, storedProcErrorLookup("postAPCreditMemoApplication",
                                              result), __FILE__, __LINE__);
      return;
    }
  }
  else if (applyPost.lastError().type() != QSqlError::NoError)
  {
    QString msg = applyPost.lastError().databaseText();
    applyPost.exec("ROLLBACK;");
    systemError(this, msg, __FILE__, __LINE__);
    return;
  }

  applyPost.exec("COMMIT;");
  if (applyPost.lastError().type() != QSqlError::NoError)
  {
    systemError(this, applyPost.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  accept();
}

void applyAPCreditMemo::sApplyBalance()
{
  XSqlQuery applyApplyBalance;
  applyApplyBalance.prepare("SELECT applyAPCreditMemoToBalance(:apopen_id) AS result;");
  applyApplyBalance.bindValue(":apopen_id", _apopenid);
  applyApplyBalance.exec();
  if (applyApplyBalance.first())
  {
    int result = applyApplyBalance.value("result").toInt();
    if (result < 0)
    {
      systemError(this, storedProcErrorLookup("applyAPCreditMemoToBalance",
                                              result), __FILE__, __LINE__);
      return;
    }
  }
  else if (applyApplyBalance.lastError().type() != QSqlError::NoError)
  {
    systemError(this, applyApplyBalance.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  populate();
}

void applyAPCreditMemo::sApply()
{
  ParameterList params;
  params.append("sourceApopenid", _apopenid);
  params.append("targetApopenid", _apopen->id());

  apCreditMemoApplication newdlg(this, "", TRUE);
  newdlg.set(params);
  if (newdlg.exec() != XDialog::Rejected)
    populate();
}

void applyAPCreditMemo::sClear()
{
  XSqlQuery applyClear;
  applyClear.prepare( "DELETE FROM apcreditapply "
             "WHERE ( (apcreditapply_source_apopen_id=:sourceApopenid) "
             " AND (apcreditapply_target_apopen_id=:targetApopenid) );" );
  applyClear.bindValue(":sourceApopenid", _apopenid);
  applyClear.bindValue(":targetApopenid", _apopen->id());
  applyClear.exec();
  if (applyClear.lastError().type() != QSqlError::NoError)
      systemError(this, applyClear.lastError().databaseText(), __FILE__, __LINE__);

  populate();
}

void applyAPCreditMemo::sClose()
{
  XSqlQuery applyClose;
  applyClose.prepare( "DELETE FROM apcreditapply "
             "WHERE (apcreditapply_source_apopen_id=:sourceApopenid);" );
  applyClose.bindValue(":sourceApopenid", _apopenid);
  applyClose.exec();
  if (applyClose.lastError().type() != QSqlError::NoError)
  {
      systemError(this, applyClose.lastError().databaseText(), __FILE__, __LINE__);
      return;
  }

  reject();
}

void applyAPCreditMemo::populate()
{
  XSqlQuery applypopulate;
  applypopulate.prepare( "SELECT apopen_vend_id, apopen_docnumber, apopen_docdate,"
             "       (apopen_amount - apopen_paid) AS available, apopen_curr_id, "
             "       COALESCE(SUM(currToCurr(apcreditapply_curr_id,"
	     "				apopen_curr_id, apcreditapply_amount, "
	     "				CURRENT_DATE)), 0) AS f_applied "
             "FROM apopen LEFT OUTER JOIN apcreditapply ON (apcreditapply_source_apopen_id=apopen_id) "
             "WHERE (apopen_id=:apopen_id) "
             "GROUP BY apopen_vend_id, apopen_docnumber, apopen_docdate,"
             "         apopen_curr_id, apopen_amount, apopen_paid;" );
  applypopulate.bindValue(":apopen_id", _apopenid);
  applypopulate.exec();
  if (applypopulate.first())
  {
    _vend->setId(applypopulate.value("apopen_vend_id").toInt());
    _docDate->setDate(applypopulate.value("apopen_docdate").toDate(), true);
    _available->setId(applypopulate.value("apopen_curr_id").toInt());
    _available->setLocalValue(applypopulate.value("available").toDouble());
    _applied->setLocalValue(applypopulate.value("f_applied").toDouble());
    _balance->setLocalValue(_available->localValue() - _applied->localValue());
    _docNumber->setText(applypopulate.value("apopen_docnumber").toString());
  
    _cachedAmount = applypopulate.value("available").toDouble();
  }
  else if (applypopulate.lastError().type() != QSqlError::NoError)
      systemError(this, applypopulate.lastError().databaseText(), __FILE__, __LINE__);

  applypopulate.prepare( "SELECT apopen_id,"
             "       CASE WHEN (apopen_doctype='V') THEN :voucher"
             "            WHEN (apopen_doctype='D') THEN :debitMemo"
             "       END AS doctype,"
             "       apopen_docnumber,"
             "       apopen_docdate, apopen_duedate,"
             "       (apopen_amount - apopen_paid - COALESCE(selected,0.0) -"
             "          COALESCE(prepared,0.0)) AS openamount,"
	           "       currConcat(apopen_curr_id) AS opencurrabbr, "
             "       apcreditapply_amount, "
	           "       currConcat(apcreditapply_curr_id) AS appliedcurrabbr,"
             "       'curr' AS openamount_xtnumericrole,"
             "       'curr' AS apcreditapply_amount_xtnumericrole"
             "  FROM apopen LEFT OUTER JOIN apcreditapply "
             "         ON ( (apcreditapply_source_apopen_id=:parentApopenid) AND (apcreditapply_target_apopen_id=apopen_id) ) "
             "       LEFT OUTER JOIN (SELECT apopen_id AS selected_apopen_id,"
             "                             SUM(currToCurr(apselect_curr_id, apopen_curr_id, apselect_amount + apselect_discount, apselect_date)) AS selected"
             "                        FROM apselect JOIN apopen ON (apselect_apopen_id=apopen_id)"
             "                       GROUP BY apopen_id) AS sub1"
             "         ON (apopen_id=selected_apopen_id)"
             "       LEFT OUTER JOIN (SELECT apopen_id AS prepared_apopen_id,"
             "                               SUM(checkitem_amount + checkitem_discount) AS prepared"
             "                          FROM checkhead JOIN checkitem ON (checkitem_checkhead_id=checkhead_id)"
             "                                     JOIN apopen ON (checkitem_apopen_id=apopen_id)"
             "                         WHERE ((NOT checkhead_posted)"
             "                           AND  (NOT checkhead_void))"
             "                         GROUP BY apopen_id) AS sub2"
             "         ON (prepared_apopen_id=apopen_id)"
             " WHERE ( (apopen_doctype IN ('V', 'D'))"
             "   AND   (apopen_open)"
             "   AND   ((apopen_amount - apopen_paid - COALESCE(selected,0.0) - COALESCE(prepared,0.0)) > 0.0)"
             "   AND   (apopen_vend_id=:vend_id) ) "
             " ORDER BY apopen_duedate, apopen_docnumber;" );
  applypopulate.bindValue(":parentApopenid", _apopenid);
  applypopulate.bindValue(":vend_id", _vend->id());
  applypopulate.bindValue(":voucher", tr("Voucher"));
  applypopulate.bindValue(":debitMemo", tr("Debit Memo"));
  applypopulate.exec();
  _apopen->populate(applypopulate);
  if (applypopulate.lastError().type() != QSqlError::NoError)
      systemError(this, applypopulate.lastError().databaseText(), __FILE__, __LINE__);
}

void applyAPCreditMemo::sPriceGroup()
{
  if (! omfgThis->singleCurrency())
    _priceGroup->setTitle(tr("In %1:").arg(_available->currAbbr()));
}
