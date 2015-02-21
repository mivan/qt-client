/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "taxRegistration.h"

#include <QMessageBox>
#include <QSqlError>
#include <QVariant>

taxRegistration::taxRegistration(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
  : XDialog(parent, name, modal, fl)
{
  setupUi(this);

  _save = _buttonBox->button(QDialogButtonBox::Save);
  _save->setEnabled(false);

  connect(_number,  SIGNAL(textChanged(QString)), this, SLOT(sHandleButtons()));
  connect(_buttonBox,    SIGNAL(accepted()),		  this, SLOT(sSave()));
  connect(_taxauth, SIGNAL(newID(int)),		  this, SLOT(sHandleButtons()));
 
  _taxregid = -1;
  _reltype  = "";
  _relid    = -1;
  _mode     = cNew;

  _dates->setStartNull(tr("Always"), omfgThis->startOfTime(), TRUE); 
  _dates->setEndNull(tr("Never"), omfgThis->endOfTime(), TRUE); 

}

taxRegistration::~taxRegistration() 
{
  // no need to delete child widget, Qt does it all for us
}

void taxRegistration::languageChange()
{
  retranslateUi(this);
}

enum SetResponse taxRegistration::set(const ParameterList pParams)
{
  XSqlQuery taxet;
  XDialog::set(pParams);
  QVariant param;
  bool	   valid;

  param = pParams.value("taxreg_id", &valid);
  if (valid)
  {
    _taxregid = param.toInt();
    sPopulate();
  }

  param = pParams.value("taxreg_rel_type", &valid);
  if (valid)
    _reltype = param.toString();

  param = pParams.value("taxreg_rel_id", &valid);
  if (valid)
    _relid = param.toInt();

  param = pParams.value("mode", &valid);
  if (valid)
  {
    if (param.toString() == "new") 
    {
      _mode = cNew; 

      taxet.exec("SELECT NEXTVAL('taxreg_taxreg_id_seq') AS _taxreg_id;");
      if (taxet.first())
		{
          _taxregid = taxet.value("_taxreg_id").toInt();
		}
      else if (taxet.lastError().type() != QSqlError::NoError)
      {
        systemError(this, taxet.lastError().databaseText(), __FILE__, __LINE__);
        return UndefinedError;
      }
    }
    else if (param.toString() == "edit") 
    {
      _mode = cEdit; 
    }
    else if (param.toString() == "view") 
    {
      _mode = cView;

      _cust->setEnabled(false);
      _vend->setEnabled(false);
      _taxauth->setEnabled(false);
      _number->setEnabled(false);
      _taxZone->setEnabled(false);
      _dates->setEnabled(false);
      _notes->setEnabled(false);

      _buttonBox->clear();
      _buttonBox->addButton(QDialogButtonBox::Close);
    }
  }

  if (handleReltype() < 0)
    return UndefinedError;

  sHandleButtons();

  return NoError;
}

void taxRegistration::sSave() 
{
  XSqlQuery taxSave;
 if( _dates->startDate() > _dates->endDate()) 
	{
	  QMessageBox::critical(this, tr("Incorrect Date Entry"),
	   tr("The start date should be earlier than the end date.") );
	  return;
	}
  taxSave.prepare("SELECT taxreg_id"
            "  FROM taxreg"
            " WHERE((taxreg_id != :taxreg_id)"
            "   AND (taxreg_rel_id = :taxreg_rel_id)"
            "   AND (taxreg_rel_type = :taxreg_rel_type)"
            "   AND (taxreg_taxauth_id = :taxreg_taxauth_id)"
            "   AND (COALESCE(taxreg_taxzone_id, -1) = :taxreg_taxzone_id)" 
            "   AND (taxreg_number = :taxreg_number))");
  taxSave.bindValue(":taxreg_id", _taxregid);
  taxSave.bindValue(":taxreg_taxauth_id", _taxauth->id());
  taxSave.bindValue(":taxreg_number", _number->text());
  if (_taxZone->isValid())	
	 taxSave.bindValue(":taxreg_taxzone_id", _taxZone->id()); 
  else
	 taxSave.bindValue(":taxreg_taxzone_id", -1);
  taxSave.bindValue(":taxreg_rel_id", _relid); //_reltype == "C" ? _cust->id() : _vend->id());
  if(!_reltype.isEmpty())
    taxSave.bindValue(":taxreg_rel_type", _reltype);
  taxSave.exec();
  if(taxSave.first())
  {
    QMessageBox::critical(this, tr("Duplicate Tax Registration"),
      tr("A Tax Registration already exists for the parameters specified.") );
    _taxZone->setFocus();
    return;
  }


  if (cNew == _mode) 
  {
    taxSave.prepare("INSERT INTO taxreg (taxreg_id, "
	      "    taxreg_rel_id, taxreg_rel_type, "
	      "    taxreg_taxauth_id, taxreg_number, taxreg_taxzone_id, taxreg_effective, taxreg_expires, taxreg_notes) "
	      "    VALUES (:taxreg_id,"
	      "    :taxreg_rel_id, :taxreg_rel_type, "
	      "    :taxreg_taxauth_id, :taxreg_number, :taxreg_taxzone_id, :taxreg_taxreg_effective, :taxreg_taxreg_expires, :taxreg_notes "
	      " );");
  }
  else 
  {
    taxSave.prepare("UPDATE taxreg SET "
	      "    taxreg_rel_id=:taxreg_rel_id, "
	      "    taxreg_rel_type=:taxreg_rel_type, "
	      "    taxreg_taxauth_id=:taxreg_taxauth_id, "
	      "    taxreg_number=:taxreg_number, "
		  "    taxreg_taxzone_id=:taxreg_taxzone_id, "
		  "    taxreg_effective=:taxreg_taxreg_effective, "
		  "	   taxreg_expires=:taxreg_taxreg_expires, "
		  "	   taxreg_notes=:taxreg_notes "
	      "WHERE (taxreg_id=:taxreg_id);");
  }
  taxSave.bindValue(":taxreg_id", _taxregid);
  taxSave.bindValue(":taxreg_rel_id", _relid); //_reltype == "C" ? _cust->id() : _vend->id());
  if(!_reltype.isEmpty())
    taxSave.bindValue(":taxreg_rel_type", _reltype);
  taxSave.bindValue(":taxreg_taxauth_id", _taxauth->id());
  taxSave.bindValue(":taxreg_number", _number->text());
  if (_taxZone->isValid())
    taxSave.bindValue(":taxreg_taxzone_id", _taxZone->id()); 
  taxSave.bindValue(":taxreg_taxreg_effective", _dates->startDate()); 
  taxSave.bindValue(":taxreg_taxreg_expires", _dates->endDate()); 
  taxSave.bindValue(":taxreg_notes", _notes->toPlainText());
	
  taxSave.exec();
  if (taxSave.lastError().type() != QSqlError::NoError)
  {
    systemError(this, taxSave.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
  accept();
}

void taxRegistration::sPopulate() 
{
  XSqlQuery taxPopulate;
  taxPopulate.prepare("SELECT * FROM taxreg WHERE (taxreg_id=:taxreg_id);");
  taxPopulate.bindValue(":taxreg_id", _taxregid);
  taxPopulate.exec();
  if (taxPopulate.first())
  { 
    _taxregid	= taxPopulate.value("taxreg_id").toInt();
    _reltype	= taxPopulate.value("taxreg_rel_type").toString();
    _relid	= taxPopulate.value("taxreg_rel_id").toInt();
    _number->setText(taxPopulate.value("taxreg_number").toString());
    _taxauth->setId(taxPopulate.value("taxreg_taxauth_id").toInt());
	_taxZone->setId(taxPopulate.value("taxreg_taxzone_id").toInt());  
	_dates->setStartDate(taxPopulate.value("taxreg_effective").toDate()); 
	_dates->setEndDate(taxPopulate.value("taxreg_expires").toDate()); 
	_notes->setText(taxPopulate.value("taxreg_notes").toString()); 

	if (handleReltype() < 0)
      return;
  }
  else if (taxPopulate.lastError().type() != QSqlError::NoError)
  {
    systemError(this, taxPopulate.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}

void taxRegistration::sHandleButtons()
{
  if (cView != _mode)
    _save->setEnabled( (! _number->text().isEmpty()) && _taxauth->isValid() );
}

int taxRegistration::handleReltype()
{
  if (_reltype == "C")
  {
    _cust->setId(_relid);
    _cust->setVisible(true);
    _vend->setVisible(false);
  }
  else if (_reltype == "V")
  {
    _vend->setId(_relid);
    _cust->setVisible(false);
    _vend->setVisible(true);
  }
  else
  {
    _cust->setVisible(false);
    _vend->setVisible(false);
  }

  return 0;
}
