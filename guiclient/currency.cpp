/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "currency.h"

#include <QMessageBox>
#include <QSqlError>
#include <QVariant>

#include "currencySelect.h"

currency::currency(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
    setupUi(this);

    _select = _buttonBox->addButton(tr("Select"), QDialogButtonBox::ActionRole);

    connect(_buttonBox, SIGNAL(accepted()), this, SLOT(sSave()));
//    connect(_currBase, SIGNAL(toggled(bool)), this, SLOT(sConfirmBaseFlag()));
    connect(_buttonBox, SIGNAL(rejected()), this, SLOT(sClose()));
    connect(_select,	SIGNAL(clicked()),	this, SLOT(sSelect()));

    // avoid sConfirmBaseFlag() when calling populate for editing base currency
    // baseOrig gets set in set() for cNew and in populate() for cEdit and cView
    baseOrig = TRUE;
}

currency::~currency()
{
    // no need to delete child widgets, Qt does it all for us
}

void currency::languageChange()
{
    retranslateUi(this);
}

bool currency::isBaseSet()
{
  XSqlQuery currencyisBaseSet;
    int numSet = 0;
    
    currencyisBaseSet.prepare("SELECT count(*) AS numSet "
              "FROM curr_symbol WHERE curr_base = TRUE");
    currencyisBaseSet.exec();
    if (currencyisBaseSet.first())
    {
	numSet = currencyisBaseSet.value("numSet").toInt();
    }
    else if (currencyisBaseSet.lastError().type() != QSqlError::NoError)
    {
	systemError(this, currencyisBaseSet.lastError().databaseText(), __FILE__, __LINE__);
    }
    return numSet != 0;
}

enum SetResponse currency::set(const ParameterList &pParams)
{
  XDialog::set(pParams);
  QVariant param;
  bool     valid;
  
  param = pParams.value("curr_id", &valid);
  if (valid)
  {
    _currid = param.toInt();
    populate();
  }
  else
    baseOrig = FALSE; // see comments in constructor

   
  param = pParams.value("mode", &valid);
  if (valid)
  {
    if (param.toString() == "new")
    {
      _mode = cNew;
      _currBase->setEnabled(! isBaseSet());
    }
    else if (param.toString() == "edit")
    {
      _mode = cEdit;
      _currBase->setEnabled(! isBaseSet());
    }
    else if (param.toString() == "view")
    {
      _mode = cView;
      
      _currName->setEnabled(FALSE);
      _currSymbol->setEnabled(FALSE);
      _currAbbr->setEnabled(FALSE);
      _currBase->setEnabled(FALSE);
      _buttonBox->clear();
      _buttonBox->addButton(QDialogButtonBox::Close);
    }
  }

  return NoError;
}

void currency::sSave()
{
  XSqlQuery currencySave;
  sConfirmBaseFlag();

  if (_currName->text().isEmpty())
  {
    QMessageBox::critical(this, tr("Name Required")
                      .arg(__FILE__)
                      .arg(__LINE__),
		      tr("Currency name is required."));
    _currName->setFocus();
    return;
  }

  if (_currAbbr->text().isEmpty() && _currSymbol->text().isEmpty())
  {
    QMessageBox::critical(this, tr("Symbol or Abbreviation Required")
                      .arg(__FILE__)
                      .arg(__LINE__),
		      tr("Either the currency symbol or abbreviation must be "
		         "supplied.\n(Both would be better.)")
		      );
    _currSymbol->setFocus();
    return;
  }
  
  if (_currAbbr->text().length() > 3)
  {
    QMessageBox::critical(this, tr("Abbreviation Too Long")
                      .arg(__FILE__)
                      .arg(__LINE__),
		      tr("The currency abbreviation must have "
		         "3 or fewer characters.\n"
			 "ISO abbreviations are exactly 3 characters long.")
		      );
    return;
  }

  
  if (_mode == cNew)
  {
      currencySave.prepare( "INSERT INTO curr_symbol "
		 "(  curr_name,  curr_symbol,  curr_abbr,  curr_base ) "
		 "VALUES "
		 "( :curr_name, :curr_symbol, :curr_abbr, :curr_base );" );
  }
  else if (_mode == cEdit)
  {
    currencySave.prepare( "UPDATE curr_symbol "
               "SET curr_name=:curr_name, curr_symbol=:curr_symbol, "
	       "    curr_abbr=:curr_abbr, curr_base = :curr_base "
               "WHERE (curr_id=:curr_id);" );
    currencySave.bindValue(":curr_id", _currid);
   }
  
  currencySave.bindValue(":curr_name", _currName->text());
  currencySave.bindValue(":curr_symbol", _currSymbol->text());
  currencySave.bindValue(":curr_abbr", _currAbbr->text());
  currencySave.bindValue(":curr_base", QVariant(_currBase->isChecked()));
  currencySave.exec();

  if (currencySave.lastError().type() != QSqlError::NoError)
  {
    systemError(this, currencySave.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
  
  done(_currid);
}

void currency::populate()
{
  XSqlQuery currencypopulate;
  currencypopulate.prepare( "SELECT curr_name, curr_symbol, curr_base, curr_abbr "
             "FROM curr_symbol "
             "WHERE (curr_id=:curr_id);" );
  currencypopulate.bindValue(":curr_id", _currid);
  currencypopulate.exec();
  if (currencypopulate.first())
  {
    _currName->setText(currencypopulate.value("curr_name").toString());
    _currSymbol->setText(currencypopulate.value("curr_symbol").toString());
    _currBase->setChecked(currencypopulate.value("curr_base").toBool());
    _currAbbr->setText(currencypopulate.value("curr_abbr").toString());

    baseOrig = _currBase->isChecked();
  }
  else if (currencypopulate.lastError().type() != QSqlError::NoError)
  {
    systemError(this, currencypopulate.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

void currency::sConfirmBaseFlag()
{
    if (_currBase->isChecked() && !baseOrig)
    {
	int response = QMessageBox::warning (this, tr("Set Base Currency?"),
				tr("You cannot change the base currency "
				  "after it is set.  Are you sure you want "
				  "%1 to be the base currency?")
				  .arg(_currName->text()),
				QMessageBox::Yes | QMessageBox::Escape,
				QMessageBox::No | QMessageBox::Default);
	if (response != QMessageBox::Yes)
	{
	    _currBase->setChecked(FALSE);
	}
    }
}

void currency::sClose()
{
  close();
}

void currency::sSelect()
{
  XSqlQuery cs;
  currencySelect *newdlg = new currencySelect(this, "", true);
  int country_id = newdlg->exec();
  if (country_id > 0)
  {
    cs.prepare("SELECT * FROM country WHERE country_id = :country_id;");
    cs.bindValue(":country_id", country_id);
    cs.exec();
    if (cs.first())
    {
      _currName->setText(cs.value("country_curr_name").toString());
      _currAbbr->setText(cs.value("country_curr_abbr").toString());
      _currSymbol->setText(cs.value("country_curr_symbol").toString());
    }
    else if (cs.lastError().type() != QSqlError::NoError)
    {
      systemError(this, cs.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }
}
