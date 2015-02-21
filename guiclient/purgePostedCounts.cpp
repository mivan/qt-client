/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "purgePostedCounts.h"

purgePostedCounts::purgePostedCounts(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
  : XDialog(parent, name, modal, fl)
{
  setupUi(this);

  // signals and slots connections
  connect(_close, SIGNAL(clicked()), this, SLOT(reject()));
  connect(_purge, SIGNAL(clicked()), this, SLOT(sPurge()));
}

purgePostedCounts::~purgePostedCounts()
{
  // no need to delete child widgets, Qt does it all for us
}

void purgePostedCounts::languageChange()
{
  retranslateUi(this);
}

void purgePostedCounts::sPurge()
{
  XSqlQuery purgePurge;
  if (_cutOffDate->isValid())
  {
    purgePurge.prepare("SELECT purgePostedCountTags(:cutOffDate, :warehous_id) AS _result;");
    purgePurge.bindValue(":cutOffDate", _cutOffDate->date());
    purgePurge.bindValue(":warehous_id", ((_warehouse->isSelected()) ? _warehouse->id() : -1));
    purgePurge.exec();

    _cutOffDate->clear();
    _cutOffDate->setFocus();

    _close->setText(tr("&Cancel"));
  }
}

