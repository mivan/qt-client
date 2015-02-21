/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#ifndef STATES_H
#define STATES_H

#include "guiclient.h"
#include "xwidget.h"
#include <parameter.h>
#include "ui_states.h"

class states : public XWidget, public Ui::states
{
    Q_OBJECT

public:
    states(QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = Qt::Window);
    ~states();

public slots:
    virtual void sFillList();
    virtual void sDelete();
    virtual void sNew();
    virtual void sEdit();
    virtual void sView();

protected slots:
    virtual void languageChange();

};

#endif // STATES_H
