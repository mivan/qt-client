/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "transferTrans.h"

#include <QMessageBox>
#include <QSqlError>
#include <QValidator>
#include <QVariant>

#include "distributeInventory.h"
#include "inputManager.h"
#include "storedProcErrorLookup.h"
#include "warehouseCluster.h"

transferTrans::transferTrans(QWidget* parent, const char* name, Qt::WFlags fl)
    : XWidget(parent, name, fl)
{
  setupUi(this);

//  (void)statusBar();

  connect(_item, SIGNAL(newId(int)), this, SLOT(sHandleItem()));
  connect(_fromWarehouse, SIGNAL(newID(int)), this, SLOT(sPopulateFromQty(int)));
  connect(_post, SIGNAL(clicked()),                   this, SLOT(sPost()));
  connect(_qty,  SIGNAL(textChanged(const QString&)), this, SLOT(sUpdateQty(const QString&)));
  connect(_toWarehouse, SIGNAL(newID(int)), this, SLOT(sPopulateToQty(int)));

  _captive = FALSE;

  _item->setType(ItemLineEdit::cGeneralInventory | ItemLineEdit::cActive);
  _fromWarehouse->setType(WComboBox::AllActiveInventory);
  _toWarehouse->setType(WComboBox::AllActiveInventory);
  _qty->setValidator(omfgThis->qtyVal());
  _fromBeforeQty->setPrecision(omfgThis->qtyVal());
  _toBeforeQty->setPrecision(omfgThis->qtyVal());
  _fromAfterQty->setPrecision(omfgThis->qtyVal());
  _toAfterQty->setPrecision(omfgThis->qtyVal());

  omfgThis->inputManager()->notify(cBCItem, this, _item, SLOT(setItemid(int)));
  omfgThis->inputManager()->notify(cBCItemSite, this, _item, SLOT(setItemsiteid(int)));

}

transferTrans::~transferTrans()
{
  // no need to delete child widgets, Qt does it all for us
}

void transferTrans::languageChange()
{
  retranslateUi(this);
}

enum SetResponse transferTrans::set(const ParameterList &pParams)
{
  XSqlQuery transferet;
  XWidget::set(pParams);
  QVariant param;
  bool     valid;
  int      invhistid = -1;

  param = pParams.value("itemsite_id", &valid);
  if (valid)
  {
    _captive = TRUE;

    _item->setItemsiteid(param.toInt());
    _item->setEnabled(FALSE);
    _fromWarehouse->setEnabled(FALSE);
  }

  param = pParams.value("qty", &valid);
  if (valid)
  {
    _captive = TRUE;

    _qty->setText(formatQty(param.toDouble()));
    _qty->setEnabled(FALSE);
  }

  param = pParams.value("invhist_id", &valid);
  if (valid)
    invhistid = param.toInt();

  param = pParams.value("mode", &valid);
  if (valid)
  {
    if (param.toString() == "new")
    {
      _mode = cNew;

      _usernameLit->clear();
      _transDate->setEnabled(_privileges->check("AlterTransactionDates"));
      _transDate->setDate(omfgThis->dbDate());
    }
    else if (param.toString() == "view")
    {
      _mode = cView;

      _transDate->setEnabled(FALSE);
      _item->setEnabled(FALSE);
      _toWarehouse->setEnabled(FALSE);
      _fromWarehouse->setEnabled(FALSE);
      _qty->setEnabled(FALSE);
      _documentNum->setEnabled(FALSE);
      _notes->setReadOnly(TRUE);
      _close->setText(tr("&Close"));
      _post->hide();

      transferet.prepare( "SELECT invhist.*, "
                 "       ABS(invhist_invqty) AS transqty,"
                 "       CASE WHEN (invhist_invqty > 0) THEN invhist_qoh_before"
                 "            ELSE NULL"
                 "       END AS qohbefore,"
                 "       CASE WHEN (invhist_invqty > 0) THEN invhist_qoh_after"
                 "            ELSE NULL"
                 "       END AS qohafter,"
                 "       CASE WHEN (invhist_invqty > 0) THEN NULL"
                 "            ELSE invhist_qoh_before"
                 "       END AS fromqohbefore,"
                 "       CASE WHEN (invhist_invqty > 0) THEN NULL"
                 "            ELSE invhist_qoh_after"
                 "       END AS fromqohafter,"
                 "       CASE WHEN (invhist_invqty > 0) THEN itemsite_warehous_id"
                 "            ELSE invhist_xfer_warehous_id"
                 "       END AS toWarehouse,"
                 "       CASE WHEN (invhist_invqty > 0) THEN invhist_xfer_warehous_id"
                 "            ELSE itemsite_warehous_id"
                 "       END AS fromWarehouse "
                 "  FROM invhist, itemsite"
                 " WHERE ((invhist_itemsite_id=itemsite_id)"
                 "   AND  (invhist_id=:invhist_id)); " );
      transferet.bindValue(":invhist_id", invhistid);
      transferet.exec();
      if (transferet.first())
      {
        _transDate->setDate(transferet.value("invhist_transdate").toDate());
        _username->setText(transferet.value("invhist_user").toString());
        _qty->setText(formatQty(transferet.value("transqty").toDouble()));
        _fromBeforeQty->setText(formatQty(transferet.value("fromqohbefore").toDouble()));
        _fromAfterQty->setText(formatQty(transferet.value("fromqohafter").toDouble()));
        _toBeforeQty->setText(formatQty(transferet.value("qohbefore").toDouble()));
        _toAfterQty->setText(formatQty(transferet.value("qohafter").toDouble()));
        _documentNum->setText(transferet.value("invhist_docnumber"));
        _notes->setText(transferet.value("invhist_comments").toString());
        _item->setItemsiteid(transferet.value("invhist_itemsite_id").toInt());
        _toWarehouse->setId(transferet.value("toWarehouse").toInt());
        _fromWarehouse->setId(transferet.value("fromWarehouse").toInt());
      }

    }
  }

  return NoError;
}

void transferTrans::sHandleItem()
{
  if (_item->isFractional())
    _qty->setValidator(omfgThis->transQtyVal());
  else
    _qty->setValidator(new QIntValidator(this));
}

void transferTrans::sPost()
{
  XSqlQuery transferPost;
  struct {
    bool        condition;
    QString     msg;
    QWidget     *widget;
  } error[] = {
    { ! _item->isValid(),
      tr("You must select an Item before posting this transaction."), _item },
    { _qty->text().length() == 0 || _qty->toDouble() <= 0,
      tr("<p>You must enter a positive Quantity before posting this Transaction."),
      _qty },
    { _fromWarehouse->id() == _toWarehouse->id(),
      tr("<p>The Target Site is the same as the Source Site. "
         "You must select a different Site for each before "
         "posting this Transaction"), _fromWarehouse },
    { true, "", NULL }
  };

  int errIndex;
  for (errIndex = 0; ! error[errIndex].condition; errIndex++)
    ;
  if (! error[errIndex].msg.isEmpty())
  {
    QMessageBox::critical(this, tr("Cannot Post Transaction"),
                          error[errIndex].msg);
    error[errIndex].widget->setFocus();
    return;
  }

  XSqlQuery rollback;
  rollback.prepare("ROLLBACK;");

  transferPost.exec("BEGIN;");	// because of possible distribution cancelations
  transferPost.prepare( "SELECT interWarehouseTransfer(:item_id, :from_warehous_id,"
             "                              :to_warehous_id, :qty, 'Misc', "
             "                              :docNumber, :comments, 0, :date ) AS result;");
  transferPost.bindValue(":item_id", _item->id());
  transferPost.bindValue(":from_warehous_id", _fromWarehouse->id());
  transferPost.bindValue(":to_warehous_id",   _toWarehouse->id());
  transferPost.bindValue(":qty",              _qty->toDouble());
  transferPost.bindValue(":docNumber", _documentNum->text());
  transferPost.bindValue(":comments", _notes->toPlainText());
  transferPost.bindValue(":date",        _transDate->date());
  transferPost.exec();
  if (transferPost.first())
  {
    int result = transferPost.value("result").toInt();
    if (result < 0)
    {
      rollback.exec();
      systemError(this, storedProcErrorLookup("interWarehouseTransfer", result),
                  __FILE__, __LINE__);
      return;
    }
    else if (transferPost.lastError().type() != QSqlError::NoError)
    {
      rollback.exec();
      systemError(this, transferPost.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }

    if (distributeInventory::SeriesAdjust(transferPost.value("result").toInt(), this) == XDialog::Rejected)
    {
      rollback.exec();
      QMessageBox::information(this, tr("Transfer Transaction"),
                               tr("Transaction Canceled") );
      return;
    }

    transferPost.exec("COMMIT;");

    if (_captive)
      close();
    else
    {
      _close->setText(tr("&Close"));
      _item->setId(-1);
      _qty->clear();
      _documentNum->clear();
      _notes->clear();
      _fromBeforeQty->clear();
      _fromAfterQty->clear();
      _toBeforeQty->clear();
      _toAfterQty->clear();

      _item->setFocus();
    }
  }
  else
  {
    rollback.exec();
    systemError( this, tr("A System Error occurred at transferTrans::%1, Item Site ID #%2, To Site ID #%3, From Site #%4.")
                       .arg(__LINE__)
                       .arg(_item->id())
                       .arg(_toWarehouse->id())
                       .arg(_fromWarehouse->id()) );
    return;
  }
}

void transferTrans::sPopulateFromQty(int pWarehousid)
{
  XSqlQuery transferPopulateFromQty;
  if (_mode != cView)
  {
    transferPopulateFromQty.prepare( "SELECT itemsite_qtyonhand "
               "FROM itemsite "
               "WHERE ( (itemsite_item_id=:item_id)"
               " AND (itemsite_warehous_id=:warehous_id) );" );
    transferPopulateFromQty.bindValue(":item_id", _item->id());
    transferPopulateFromQty.bindValue(":warehous_id", pWarehousid);
    transferPopulateFromQty.exec();
    if (transferPopulateFromQty.first())
    {
      _cachedFromBeforeQty = transferPopulateFromQty.value("itemsite_qtyonhand").toDouble();
      _fromBeforeQty->setText(formatQty(transferPopulateFromQty.value("itemsite_qtyonhand").toDouble()));

      if (_qty->text().length())
        _fromAfterQty->setText(formatQty(transferPopulateFromQty.value("itemsite_qtyonhand").toDouble() - _qty->toDouble()));
    }
    else if (transferPopulateFromQty.lastError().type() != QSqlError::NoError)
    {
      systemError(this, transferPopulateFromQty.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }
}

void transferTrans::sPopulateToQty(int pWarehousid)
{
  XSqlQuery transferPopulateToQty;
  if (_mode != cView)
  {
    transferPopulateToQty.prepare( "SELECT itemsite_qtyonhand "
               "FROM itemsite "
               "WHERE ( (itemsite_item_id=:item_id)"
               " AND (itemsite_warehous_id=:warehous_id) );" );
    transferPopulateToQty.bindValue(":item_id", _item->id());
    transferPopulateToQty.bindValue(":warehous_id", pWarehousid);
    transferPopulateToQty.exec();
    if (transferPopulateToQty.first())
    {
      _cachedToBeforeQty = transferPopulateToQty.value("itemsite_qtyonhand").toDouble();
      _toBeforeQty->setText(formatQty(transferPopulateToQty.value("itemsite_qtyonhand").toDouble()));

      if (_qty->text().length())
        _toAfterQty->setText(formatQty(transferPopulateToQty.value("itemsite_qtyonhand").toDouble() + _qty->toDouble()));
    }
  }
}

void transferTrans::sUpdateQty(const QString &pQty)
{
  if (_mode != cView)
  {
    _fromAfterQty->setText(formatQty(_cachedFromBeforeQty - pQty.toDouble()));
    _toAfterQty->setText(formatQty(_cachedToBeforeQty + pQty.toDouble()));
  }
}
