/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#ifndef __XCOMBOBOXPRIVATE_H__
#define __XCOMBOBOXPRIVATE_H__

#include <QList>
#include <QPair>
#include <QString>

#include "xcombobox.h"

class QLabel;
class QPushButton;
class XDataWidgetMapper;
class XComboBoxEditorDescrip;

class XComboBoxPrivate : public QObject
{
  Q_OBJECT

  public:
    XComboBoxPrivate(XComboBox *parent);
    virtual ~XComboBoxPrivate();

    int numberOfCurrencies();

  public slots:
    void sEdit();
    void setType(XComboBox::XComboBoxTypes ptype);

  public:
    QList<QString>                       _codes;
    enum XComboBox::Defaults             _default;
    QMap<int, XComboBoxEditorDescrip*>   _editorMap;
    QPushButton                         *_editButton;
    QList<int>                           _ids;
    QLabel                              *_label;
    int                                  _lastId;
    QString                              _nullStr;
    XComboBox                           *_parent;
    int                                  _popupCounter; // a real hack
    enum XComboBox::XComboBoxTypes       _type;

    // the following probably belong in an abstract interface superclass
    QString            _fieldName;
    QString            _listDisplayFieldName;
    QString            _listIdFieldName;
    QString            _listSchemaName;
    QString            _listTableName;
    XDataWidgetMapper *_mapper;
};

#endif
