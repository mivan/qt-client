/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "purgeClosedWorkOrders.h"

#include <QVariant>

/*
 *  Constructs a purgeClosedWorkOrders as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
purgeClosedWorkOrders::purgeClosedWorkOrders(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  setupUi(this);


  // signals and slots connections
  connect(_close, SIGNAL(clicked()), this, SLOT(reject()));
  connect(_purge, SIGNAL(clicked()), this, SLOT(sPurge()));
}

/*
 *  Destroys the object and frees any allocated resources
 */
purgeClosedWorkOrders::~purgeClosedWorkOrders()
{
  // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void purgeClosedWorkOrders::languageChange()
{
  retranslateUi(this);
}

void purgeClosedWorkOrders::sPurge()
{
  XSqlQuery purgePurge;
  // note that below is the code for performing this as a stored procedure
  // and we should implement this after version 2.1
  if (_cutOffDate->isValid())
  {
    if (_warehouse->isAll())
      purgePurge.prepare("SELECT MIN(deleteWo(wo_id, false)) AS result"
                "  FROM wo"
                " WHERE ((wo_status = 'C')"
                "   AND  (wo_duedate <= :cutoffDate))");
    else
      purgePurge.prepare("SELECT MIN(deleteWo(wo_id, false)) AS result"
                "  FROM wo, itemsite"
                " WHERE ((wo_status = 'C')"
                "   AND  (wo_duedate <= :cutoffDate)"
                "   AND  (wo_itemsite_id = itemsite_id)"
                "   AND  (itemsite_warehous_id = :whs_id))");
    purgePurge.bindValue(":cutoffDate", _cutOffDate->date());
    purgePurge.bindValue(":whs_id", _warehouse->id());
    purgePurge.exec();

    _cutOffDate->clear();
    _cutOffDate->setFocus();
  }
}

