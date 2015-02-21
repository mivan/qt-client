/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#ifndef freightClassS_H
#define freightClassS_H

#include "xwidget.h"

#include "ui_freightClasses.h"

class freightClasses : public XWidget, public Ui::freightClasses
{
    Q_OBJECT

public:
    freightClasses(QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = Qt::Window);
    ~freightClasses();

public slots:
    virtual void sFillList(int pId);
    virtual void sNew();
    virtual void sEdit();
    virtual void sView();
    virtual void sDelete();
    virtual void sPrint();
    virtual void sDeleteUnused();

protected slots:
    virtual void languageChange();

};

#endif // freightClasses_H
