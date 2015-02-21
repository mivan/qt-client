/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#ifndef XUILOADER_H
#define XUILOADER_H

#include <QMap>
#include <QString>
#include <QUiLoader>
#include <QDesignerCustomWidgetInterface>

class XUiLoader : public QUiLoader
{
  Q_OBJECT

  public:
    XUiLoader(QObject* parent = 0);
    ~XUiLoader();

   virtual QWidget * createWidget ( const QString & className, QWidget * parent = 0, const QString & name = QString() );

  protected:
    QMap<QString, QDesignerCustomWidgetInterface*> m_customWidgets;
};

#endif // XUILOADER_H
