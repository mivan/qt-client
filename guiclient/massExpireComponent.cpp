/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "massExpireComponent.h"

#include <QMessageBox>
#include <QSqlQuery>

massExpireComponent::massExpireComponent(QWidget* parent, const char* name, Qt::WFlags fl)
  : XWidget(parent, name, fl)
{
  setupUi(this);

  _captive = FALSE;

  _original->setType(ItemLineEdit::cGeneralComponents);

  _expireAsOf->setNullString(tr("Immediate"));
  _expireAsOf->setNullDate(omfgThis->dbDate());
  _expireAsOf->setAllowNullDate(true);
  _expireAsOf->setNull();

  connect(_expire, SIGNAL(clicked()), this, SLOT(sExpire()));
  connect(_close, SIGNAL(clicked()), this, SLOT(close()));
}

massExpireComponent::~massExpireComponent()
{
  // no need to delete child widgets, Qt does it all for us
}

void massExpireComponent::languageChange()
{
  retranslateUi(this);
}

enum SetResponse massExpireComponent::set(const ParameterList &pParams)
{
  XWidget::set(pParams);
  _captive = TRUE;

  QVariant param;
  bool     valid;

  param = pParams.value("item_id", &valid);
  if (valid)
    _original->setId(param.toInt());

  return NoError;
}

void massExpireComponent::sExpire()
{
  XSqlQuery massExpire;
  if ( (_original->isValid()) && (_expireAsOf->isValid()) )
  {
    if (_metrics->boolean("RevControl"))
    {
      massExpire.prepare("SELECT * "
	  	      "FROM bomitem, rev "
	 		  "WHERE ( (bomitem_rev_id=rev_id) "
			  "AND (rev_status='P') "
			  "AND (bomitem_item_id=:item_id) ) "
			  "LIMIT 1;");
	  massExpire.bindValue(":item_id", _original->id());
	  massExpire.exec();
	  if (massExpire.first())
        QMessageBox::information( this, tr("Mass Expire"),
                          tr("<p>This process will only affect active revisions. "
						  "Items on pending revisions must be expired manually.")  );
    }
    QSqlQuery expire;

    if (_expireAsOf->isNull())
      expire.prepare("SELECT massExpireBomitem(:item_id, CURRENT_DATE, :ecn);");
    else
      expire.prepare("SELECT massExpireBomitem(:item_id, :expire_date, :ecn);");

    expire.bindValue(":item_id", _original->id());
    expire.bindValue(":expire_date", _expireAsOf->date());
    expire.bindValue(":ecn", _ecn->text());

    expire.exec();

    if (_captive)
      close();
    {
      _close->setText(tr("&Close"));
      _original->setId(-1);
      _ecn->clear();
      _original->setFocus();
    }
  }
}
