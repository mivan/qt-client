/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "enterMiscCount.h"

#include <QMessageBox>
#include <QSqlError>
#include <QVariant>

#include "storedProcErrorLookup.h"

enterMiscCount::enterMiscCount(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  setupUi(this);

  connect(_post, SIGNAL(clicked()), this, SLOT(sPost()));

  _captive = FALSE;
  
  _item->setType(ItemLineEdit::cGeneralInventory | ItemLineEdit::cActive);
  _item->addExtraClause( QString("(item_type NOT IN ('R', 'F'))") );
  _item->addExtraClause( QString("(itemsite_costmethod != 'J')") );
  _warehouse->setType(WComboBox::AllActiveInventory);
  _qty->setValidator(omfgThis->qtyVal());

  if (!_metrics->boolean("MultiWhs"))
  {
    _warehouseLit->hide();
    _warehouse->hide();
  }
}

enterMiscCount::~enterMiscCount()
{
    // no need to delete child widgets, Qt does it all for us
}

void enterMiscCount::languageChange()
{
    retranslateUi(this);
}

enum SetResponse enterMiscCount::set(const ParameterList &pParams)
{
  XDialog::set(pParams);
  _captive = TRUE;

  QVariant param;
  bool     valid;

  param = pParams.value("itemsite_id", &valid);
  if (valid)
  {
    _item->setItemsiteid(param.toInt());
    _item->setEnabled(FALSE);
    _warehouse->setEnabled(FALSE);
  }

  return NoError;
}

void enterMiscCount::sPost()
{
  XSqlQuery enterPost;
  enterPost.prepare( "SELECT ( (itemsite_controlmethod IN ('L', 'S')) OR (itemsite_loccntrl) ) AS detailed,"
             "       COALESCE(invcnt_id, -1) AS cnttagid "
             "FROM itemsite LEFT OUTER JOIN invcnt ON ( (invcnt_itemsite_id=itemsite_id) AND (NOT invcnt_posted) ) "
             "WHERE ( (itemsite_warehous_id=:warehous_id)"
             " AND (itemsite_item_id=:item_id) );" );
  enterPost.bindValue(":warehous_id", _warehouse->id());
  enterPost.bindValue(":item_id", _item->id());
  enterPost.exec();
  if (enterPost.first())
  {
    if (enterPost.value("detailed").toBool())
    {
      QMessageBox::warning( this, tr("Cannot Enter Misc. Count"),
                            tr("<p>The selected Item Site is controlled via a "
                               "combination of Lot/Serial and/or Location "
                               "control. Misc. Counts can only be entered for "
                               "Item Sites that are not Lot/Serial or Location "
                               "controlled." ) );
      return;
    }

    if (enterPost.value("cnttagid") != -1)
    {
      QMessageBox::warning(this, tr("Count Tag Previously Created"),
                           tr("<p>An unposted Count Tag already exists for "
                              "this Item and Site. You may not a Misc. Count "
                              "until the current Count Tag has been posted."));
      return;
    }
  }
  else if (enterPost.lastError().type() != QSqlError::NoError)
  {
    systemError(this, enterPost.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  enterPost.prepare( "SELECT postMiscCount(itemsite_id, :qty, :comments) AS cnttag_id "
             "FROM itemsite "
             "WHERE ( (itemsite_item_id=:item_id)"
             " AND (itemsite_warehous_id=:warehous_id) );" );
  enterPost.bindValue(":qty", _qty->toDouble());
  enterPost.bindValue(":comments", _comments->toPlainText());
  enterPost.bindValue(":item_id", _item->id());
  enterPost.bindValue(":warehous_id", _warehouse->id());
  enterPost.exec();
  if (enterPost.first())
  {
    int result = enterPost.value("cnttag_id").toInt();
    if (result < 0)
    {
      systemError(this, storedProcErrorLookup("postMiscCount", result),
                  __FILE__, __LINE__);
      return;
    }
  }
  else if (enterPost.lastError().type() != QSqlError::NoError)
  {
    systemError(this, enterPost.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  if (_captive)
    done(enterPost.value("cnttag_id").toInt());
  else
  {
    _item->setId(-1);
    _qty->clear();
    _comments->clear();
    _item->setFocus();
  }
}
