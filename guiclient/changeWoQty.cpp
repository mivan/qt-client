/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "changeWoQty.h"

#include <QMessageBox>
#include <QSqlError>
#include <QValidator>
#include <QVariant>

#include "storedProcErrorLookup.h"

changeWoQty::changeWoQty(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  setupUi(this);

  connect(_buttonBox, SIGNAL(accepted()), this, SLOT(sChangeQty()));
  connect(_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
  connect(_newQtyOrdered, SIGNAL(textChanged(const QString&)), this, SLOT(sQtyChanged(const QString&)));

  _captive = FALSE;

  _wo->setType(cWoOpen | cWoExploded | cWoIssued);
  _newQtyOrdered->setValidator(omfgThis->qtyVal());
  _newQtyReceived->setPrecision(omfgThis->qtyVal());
  _newQtyBalance->setPrecision(omfgThis->qtyVal());
  _currentQtyOrdered->setPrecision(omfgThis->qtyVal());
  _currentQtyReceived->setPrecision(omfgThis->qtyVal());
  _currentQtyBalance->setPrecision(omfgThis->qtyVal());
  _cmnttype->setType(XComboBox::AllCommentTypes);
  _commentGroup->setEnabled(_postComment->isChecked());
  adjustSize();
}

changeWoQty::~changeWoQty()
{
  // no need to delete child widgets, Qt does it all for us
}

void changeWoQty::languageChange()
{
  retranslateUi(this);
}

enum SetResponse changeWoQty::set(const ParameterList &pParams)
{
  XDialog::set(pParams);
  _captive = TRUE;

  QVariant param;
  bool     valid;

  param = pParams.value("wo_id", &valid);
  if (valid)
  {
    _wo->setId(param.toInt());
    _wo->setReadOnly(TRUE);
  }

  param = pParams.value("newQty", &valid);
  if (valid)
    _newQtyOrdered->setDouble(param.toDouble());

  return NoError;
}

void changeWoQty::sChangeQty()
{
  XSqlQuery changeChangeQty;
  if (_wo->status() == 'R')
  {
    QMessageBox::warning( this, tr("Cannot Reschedule Released W/O"),
                          tr( "<p>The selected Work Order has been Released. "
			  "Rescheduling it." ) );
    return;
  }

  double newQty = 0.0;
  if (_wo->method() == "D")
    newQty = _newQtyOrdered->toDouble() * -1.0;
  else
    newQty = _newQtyOrdered->toDouble();

  if(newQty == 0 && QMessageBox::question(this, tr("Zero Qty. Value"), tr("The current value specified is 0. Are you sure you want to continue?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No)
    return;

  if (newQty > 0.0)
  {
    changeChangeQty.prepare( "SELECT validateOrderQty(wo_itemsite_id, :qty, TRUE) AS qty "
               "FROM wo "
               "WHERE (wo_id=:wo_id);" );
    changeChangeQty.bindValue(":wo_id", _wo->id());
    changeChangeQty.bindValue(":qty", _newQtyOrdered->toDouble());
    changeChangeQty.exec();
    if (changeChangeQty.first())
    {
      if (changeChangeQty.value("qty").toDouble() != newQty)
      {
        if ( QMessageBox::warning( this, tr("Invalid Order Qty"),
                                   tr("<p>The new Order Quantity that you have "
                                   "entered does not meet the Order Parameters "
                                   "set for the parent Item Site for this Work "
                                   "Order.  In order to meet the Item Site "
                                   "Order Parameters the new Order Quantity "
                                   "must be increased to %1. Do you want to "
                                   "change the Order Quantity for this Work "
                                   "Order to %2?" )
                                   .arg(formatQty(changeChangeQty.value("qty").toDouble()))
                                   .arg(formatQty(changeChangeQty.value("qty").toDouble())),
                                   tr("&Yes"), tr("&No"), QString::null, 0, 1 ) == 1 )
          return;
        else
          newQty = changeChangeQty.value("qty").toDouble();
      }
    }
    else if (changeChangeQty.lastError().type() != QSqlError::NoError)
    {
      systemError(this, changeChangeQty.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }

  changeChangeQty.prepare("SELECT changeWoQty(:wo_id, :qty, TRUE);");
  changeChangeQty.bindValue(":wo_id", _wo->id());
  changeChangeQty.bindValue(":qty", newQty);
  changeChangeQty.exec();

  if (_postComment->isChecked())
  {
    changeChangeQty.prepare("SELECT postComment(:cmnttype_id, 'W', :wo_id, :comment) AS result");
    changeChangeQty.bindValue(":cmnttype_id", _cmnttype->id());
    changeChangeQty.bindValue(":wo_id", _wo->id());
    changeChangeQty.bindValue(":comment", _comment->toPlainText());
    changeChangeQty.exec();
    if (changeChangeQty.first())
    {
      int result = changeChangeQty.value("result").toInt();
      if (result < 0)
      {
        systemError(this, storedProcErrorLookup("postComment", result),
                    __FILE__, __LINE__);
        return;
      }
    }
    else if (changeChangeQty.lastError().type() != QSqlError::NoError)
    {
      systemError(this, changeChangeQty.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }

  omfgThis->sWorkOrdersUpdated(_wo->id(), TRUE);

  accept();
}

void changeWoQty::sQtyChanged(const QString &pNewQty)
{
  double qtyBalance = (pNewQty.toDouble() - _newQtyReceived->toDouble());
  if (qtyBalance < 0)
    qtyBalance = 0;

  _newQtyBalance->setDouble(qtyBalance);
}
