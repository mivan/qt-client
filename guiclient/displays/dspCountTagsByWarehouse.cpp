/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "dspCountTagsByWarehouse.h"

dspCountTagsByWarehouse::dspCountTagsByWarehouse(QWidget* parent, const char*, Qt::WFlags fl)
  : dspCountTagsBase(parent, "dspCountTagsByWarehouse", fl)
{
  setWindowTitle(tr("Count Tags by Site"));
  setReportName("CountTagsByWarehouse");
  _parameter->hide();
  _itemGroup->hide();
}
