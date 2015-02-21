/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "thawItemSitesByClassCode.h"

#include "errorReporter.h"

thawItemSitesByClassCode::thawItemSitesByClassCode(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
  : XDialog(parent, name, modal, fl)
{
  setupUi(this);

  // signals and slots connections
  connect(_close, SIGNAL(clicked()), this, SLOT(reject()));
  connect(_thaw, SIGNAL(clicked()), this, SLOT(sThaw()));

  _classCode->setType(ParameterGroup::ClassCode);

  //If not multi-warehouse hide whs control
  if (!_metrics->boolean("MultiWhs"))
  {
    _warehouseLit->hide();
    _warehouse->hide();
  }
}

thawItemSitesByClassCode::~thawItemSitesByClassCode()
{
  // no need to delete child widgets, Qt does it all for us
}

void thawItemSitesByClassCode::languageChange()
{
  retranslateUi(this);
}

void thawItemSitesByClassCode::sThaw()
{
  XSqlQuery thawThaw;
  QString sql( "SELECT thawItemsite(itemsite_id) "
               "FROM itemsite, item "
               "WHERE ( (itemsite_freeze)"
               " AND (itemsite_item_id=item_id)"
               " AND (itemsite_warehous_id=:warehous_id)" );

  if (_classCode->isSelected())
    sql += " AND (item_classcode_id=:classcode_id)";
  else if (_classCode->isPattern())
    sql += " AND (item_classcode_id IN (SELECT classcode_id FROM classcode WHERE (classcode_code ~ :classcode_pattern)))";

  sql += ");";

  thawThaw.prepare(sql);
  thawThaw.bindValue(":warehous_id", _warehouse->id());
  _classCode->bindValue(thawThaw);
  thawThaw.exec();
  if (ErrorReporter::error(QtCriticalMsg, this, tr("Thaw Item Site"),
                           thawThaw, __FILE__, __LINE__))
    return;

  accept();
}

