/*
 * This file is part of the xTuple ERP: PostBooks Edition, a free and
 * open source Enterprise Resource Planning software suite,
 * Copyright (c) 1999-2014 by OpenMFG LLC, d/b/a xTuple.
 * It is licensed to you under the Common Public Attribution License
 * version 1.0, the full text of which (including xTuple-specific Exhibits)
 * is available at www.xtuple.com/CPAL.  By using this software, you agree
 * to be bound by its terms.
 */

#include "invoiceItem.h"

#include <QMessageBox>
#include <QSqlError>
#include <QValidator>
#include <QVariant>

#include <metasql.h>

#include "xdoublevalidator.h"
#include "priceList.h"
#include "taxDetail.h"
#include "errorReporter.h"
#include "guiErrorCheck.h"

/* TODO: connect(_warehouse id, SIGNAL(newId(int)), ...) to set _trackqoh?
         bug 16465 - seems too disruptive now as we're between 3.8.0RC and RC2
 */
invoiceItem::invoiceItem(QWidget* parent, const char * name, Qt::WindowFlags fl)
    : XDialog(parent, name, fl)
{
  setupUi(this);

  connect(_billed,  SIGNAL(editingFinished()),    this, SLOT(sCalculateExtendedPrice()));
  connect(_item,    SIGNAL(newId(int)),     this, SLOT(sPopulateItemInfo(int)));
  connect(_item,    SIGNAL(newId(int)),     this, SLOT(sHandleUpdateInv()));
  connect(_extended,SIGNAL(valueChanged()), this, SLOT(sLookupTax()));
  connect(_listPrices,   SIGNAL(clicked()), this, SLOT(sListPrices()));
  connect(_price,   SIGNAL(idChanged(int)), this, SLOT(sPriceGroup()));
  connect(_price,   SIGNAL(valueChanged()), this, SLOT(sCalculateExtendedPrice()));
  connect(_save,    SIGNAL(clicked()),      this, SLOT(sSave()));
  connect(_taxLit,  SIGNAL(leftClickedURL(QString)), this, SLOT(sTaxDetail()));
  connect(_taxtype, SIGNAL(newID(int)),     this, SLOT(sLookupTax()));
  connect(_qtyUOM, SIGNAL(newID(int)), this, SLOT(sQtyUOMChanged()));
  connect(_pricingUOM, SIGNAL(newID(int)), this, SLOT(sPriceUOMChanged()));
  connect(_miscSelected, SIGNAL(toggled(bool)), this, SLOT(sMiscSelected(bool)));

  _ordered->setValidator(omfgThis->qtyVal());
  _billed->setValidator(omfgThis->qtyVal());

  _altRevAccnt->setType(GLCluster::cRevenue);

  _taxtype->setEnabled(_privileges->check("OverrideTax"));
  
  _mode = cNew;
  _invcheadid	= -1;
  _custid	= -1;
  _invcitemid	= -1;
  _priceRatioCache = 1.0;
  _taxzoneid	= -1;
  _qtyinvuomratio = 1.0;
  _priceinvuomratio = 1.0;
  _invuomid = -1;
  _trackqoh = true;
  
  //If not multi-warehouse hide whs control
  if (!_metrics->boolean("MultiWhs"))
  {
    _warehouseLit->hide();
    _warehouse->hide();
  }
  _saved = false;
  
  adjustSize();
}

invoiceItem::~invoiceItem()
{
  // no need to delete child widgets, Qt does it all for us
}

void invoiceItem::languageChange()
{
  retranslateUi(this);
}

enum SetResponse invoiceItem::set(const ParameterList &pParams)
{
  XSqlQuery invoiceet;
  XDialog::set(pParams);
  QVariant param;
  bool     valid;

  param = pParams.value("invchead_id", &valid);
  if (valid)
  {
    _invcheadid = param.toInt();

    invoiceet.prepare("SELECT * "
                      "FROM invchead "
			  "WHERE (invchead_id = :invchead_id);");
    invoiceet.bindValue(":invchead_id", _invcheadid);
    invoiceet.exec();
    if (invoiceet.first())
    {
      _invoiceNumber->setText(invoiceet.value("invchead_invcnumber").toString());
      _custid = invoiceet.value("invchead_cust_id").toInt();
      _taxzoneid = invoiceet.value("invchead_taxzone_id").toInt();
      _tax->setId(invoiceet.value("invchead_curr_id").toInt());
      _price->setId(invoiceet.value("invchead_curr_id").toInt());
      _price->setEffective(invoiceet.value("invchead_invcdate").toDate());
      sPriceGroup();
    }
    else if (invoiceet.lastError().type() != QSqlError::NoError)
    {
      systemError(this, invoiceet.lastError().databaseText(), __FILE__, __LINE__);
      return UndefinedError;
    }
  }

  param = pParams.value("invcitem_id", &valid);
  if (valid)
  {
    _invcitemid = param.toInt();
    populate();
  }

  param = pParams.value("mode", &valid);
  if (valid)
  {
    if (param.toString() == "new")
    {
      _mode = cNew;

      invoiceet.exec("SELECT NEXTVAL('invcitem_invcitem_id_seq') AS invcitem_id;");
      if (invoiceet.first())
        _invcitemid = invoiceet.value("invcitem_id").toInt();
      else if (invoiceet.lastError().type() != QSqlError::NoError)
      {
            systemError(this, invoiceet.lastError().databaseText(), __FILE__, __LINE__);
            return UndefinedError;
      }

      invoiceet.prepare( "SELECT (COALESCE(MAX(invcitem_linenumber), 0) + 1) AS linenumber "
                         "FROM invcitem "
                         "WHERE (invcitem_invchead_id=:invchead_id);" );
      invoiceet.bindValue(":invchead_id", _invcheadid);
      invoiceet.exec();
      if (invoiceet.first())
        _lineNumber->setText(invoiceet.value("linenumber").toString());
      else if (invoiceet.lastError().type() != QSqlError::NoError)
      {
	    systemError(this, invoiceet.lastError().databaseText(), __FILE__, __LINE__);
	    return UndefinedError;
      }

      connect(_billed, SIGNAL(editingFinished()), this, SLOT(sDeterminePrice()));
      connect(_billed, SIGNAL(editingFinished()), this, SLOT(sCalculateExtendedPrice()));
      connect(_price, SIGNAL(editingFinished()), this, SLOT(sCalculateExtendedPrice()));
      _item->setType(ItemLineEdit::cSold);
      _salescat->setType(XComboBox::SalesCategoriesActive);
    }
    else if (param.toString() == "edit")
    {
      _mode = cEdit;

      connect(_billed, SIGNAL(editingFinished()), this, SLOT(sDeterminePrice()));
      connect(_billed, SIGNAL(editingFinished()), this, SLOT(sCalculateExtendedPrice()));
      connect(_price, SIGNAL(editingFinished()), this, SLOT(sCalculateExtendedPrice()));
    }
    else if (param.toString() == "view")
    {
      _mode = cView;

      _itemTypeGroup->setEnabled(FALSE);
      _custPn->setEnabled(FALSE);
      _ordered->setEnabled(FALSE);
      _billed->setEnabled(FALSE);
      _price->setEnabled(FALSE);
      _notes->setReadOnly(TRUE);
      _taxtype->setEnabled(false);
      _altRevAccnt->setEnabled(false);
      _qtyUOM->setEnabled(false);
      _pricingUOM->setEnabled(false);

      _save->hide();
      _close->setText(tr("&Cancel"));
    }
  }

  return NoError;
}

int invoiceItem::id() const
{
  return _invcitemid;
}

int invoiceItem::mode() const
{
  return _mode;
}

void invoiceItem::sSave()
{
  QList<GuiErrorCheck> errors;
  errors << GuiErrorCheck(_itemSelected->isChecked() && !_item->isValid(), _item,
                          tr("<p>You must select an Item for this Invoice Item before you may save it."))
         << GuiErrorCheck(!_itemSelected->isChecked() && !_itemNumber->text().length(), _itemNumber,
                          tr("<p>You must enter an Item Number for this Miscellaneous Invoice Item before you may save it."))
         << GuiErrorCheck(!_itemSelected->isChecked() && !_itemDescrip->toPlainText().length(), _itemDescrip,
                          tr("<p>You must enter a Item Description for this Miscellaneous Invoice Item before you may save it."))
         << GuiErrorCheck(!_itemSelected->isChecked() && !_salescat->isValid(), _salescat,
                          tr("<p>You must select a Sales Category for this Miscellaneous Invoice Item before you may save it."))
  ;
  if (GuiErrorCheck::reportErrors(this, tr("Cannot Save Invoice Item"), errors))
    return;

  XSqlQuery invoiceSave;

  if (_mode == cNew)
  {
    invoiceSave.prepare( "INSERT INTO invcitem "
               "( invcitem_id, invcitem_invchead_id, invcitem_linenumber,"
               "  invcitem_item_id, invcitem_warehous_id,"
               "  invcitem_number, invcitem_descrip, invcitem_salescat_id,"
               "  invcitem_custpn,"
               "  invcitem_ordered, invcitem_billed, invcitem_updateinv,"
               "  invcitem_qty_uom_id, invcitem_qty_invuomratio,"
               "  invcitem_custprice, invcitem_price,"
               "  invcitem_price_uom_id, invcitem_price_invuomratio,"
               "  invcitem_notes, "
               "  invcitem_taxtype_id, invcitem_rev_accnt_id) "
               "VALUES "
               "( :invcitem_id, :invchead_id, :invcitem_linenumber,"
               "  :item_id, :warehous_id,"
               "  :invcitem_number, :invcitem_descrip, :invcitem_salescat_id,"
               "  :invcitem_custpn,"
               "  :invcitem_ordered, :invcitem_billed, :invcitem_updateinv,"
               "  :qty_uom_id, :qty_invuomratio,"
               "  :invcitem_custprice, :invcitem_price,"
               "  :price_uom_id, :price_invuomratio,"
               "  :invcitem_notes, "
               "  :invcitem_taxtype_id, :invcitem_rev_accnt_id);");
	       
    invoiceSave.bindValue(":invchead_id", _invcheadid);
    invoiceSave.bindValue(":invcitem_linenumber", _lineNumber->text());
  }
  else if (_mode == cEdit)
    invoiceSave.prepare( "UPDATE invcitem "
               "SET invcitem_item_id=:item_id, invcitem_warehous_id=:warehous_id,"
               "    invcitem_number=:invcitem_number, invcitem_descrip=:invcitem_descrip,"
               "    invcitem_salescat_id=:invcitem_salescat_id,"
               "    invcitem_custpn=:invcitem_custpn, invcitem_updateinv=:invcitem_updateinv,"
               "    invcitem_ordered=:invcitem_ordered, invcitem_billed=:invcitem_billed,"
               "    invcitem_qty_uom_id=:qty_uom_id, invcitem_qty_invuomratio=:qty_invuomratio,"
               "    invcitem_custprice=:invcitem_custprice, invcitem_price=:invcitem_price,"
               "    invcitem_price_uom_id=:price_uom_id, invcitem_price_invuomratio=:price_invuomratio,"
               "    invcitem_notes=:invcitem_notes,"
               "    invcitem_taxtype_id=:invcitem_taxtype_id,"
               "    invcitem_rev_accnt_id=:invcitem_rev_accnt_id "
	           "WHERE (invcitem_id=:invcitem_id);" );

  if (_itemSelected->isChecked())
  {
    invoiceSave.bindValue(":item_id", _item->id());
    invoiceSave.bindValue(":warehous_id", _warehouse->id());
  }
  else
  {
    invoiceSave.bindValue(":item_id", -1);
    invoiceSave.bindValue(":warehous_id", -1);
  }

  invoiceSave.bindValue(":invcitem_id", _invcitemid);
  invoiceSave.bindValue(":invcitem_number", _itemNumber->text());
  invoiceSave.bindValue(":invcitem_descrip", _itemDescrip->toPlainText());
  invoiceSave.bindValue(":invcitem_salescat_id", _salescat->id());
  invoiceSave.bindValue(":invcitem_custpn", _custPn->text());
  invoiceSave.bindValue(":invcitem_ordered", _ordered->toDouble());
  invoiceSave.bindValue(":invcitem_billed", _billed->toDouble());
  invoiceSave.bindValue(":invcitem_updateinv", QVariant(_updateInv->isChecked()));
  if(!_miscSelected->isChecked())
    invoiceSave.bindValue(":qty_uom_id", _qtyUOM->id());
  invoiceSave.bindValue(":qty_invuomratio", _qtyinvuomratio);
  invoiceSave.bindValue(":invcitem_custprice", _custPrice->localValue());
  invoiceSave.bindValue(":invcitem_price", _price->localValue());
  if(!_miscSelected->isChecked())
    invoiceSave.bindValue(":price_uom_id", _pricingUOM->id());
  invoiceSave.bindValue(":price_invuomratio", _priceinvuomratio);
  invoiceSave.bindValue(":invcitem_notes", _notes->toPlainText());
  if(_taxtype->isValid())
    invoiceSave.bindValue(":invcitem_taxtype_id",	_taxtype->id());
  if (_altRevAccnt->isValid())
    invoiceSave.bindValue(":invcitem_rev_accnt_id", _altRevAccnt->id());

  invoiceSave.exec();
  if (invoiceSave.lastError().type() != QSqlError::NoError)
  {
    systemError(this, invoiceSave.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
  _saved = true;
  emit saved(_invcitemid);

  done(_invcitemid);
}

void invoiceItem::populate()
{
  XSqlQuery invcitem;
  invcitem.prepare( "SELECT invcitem.*, invchead_invcnumber,"
                    "       CASE WHEN (item_id IS NULL) THEN :na"
                    "            ELSE item_listprice"
                    "       END AS f_listprice,"
                    "		taxzone_id,"
                    "       invchead_curr_id AS taxcurr_id,"
                    "       itemsite_costmethod"
                    " FROM invcitem JOIN "
                    "     invchead LEFT OUTER JOIN taxzone ON "
                    "       (invchead_taxzone_id = taxzone_id) "
                    "     ON (invcitem_invchead_id = invchead_id) LEFT OUTER JOIN "
                    "     item ON (invcitem_item_id = item_id) "
                    " LEFT OUTER JOIN invcitemtax ON (invcitem_id = taxhist_parent_id) "
                    " LEFT OUTER JOIN itemsite ON (itemsite_item_id=item_id "
                    "                          AND itemsite_warehous_id=invcitem_warehous_id)"
                    "WHERE (invcitem_id = :invcitem_id);" );
  invcitem.bindValue(":invcitem_id", _invcitemid);
  invcitem.exec();
  if (invcitem.first())
  {
    _invcheadid = invcitem.value("invcitem_invchead_id").toInt();
    _invoiceNumber->setText(invcitem.value("invchead_invcnumber").toString());
    _lineNumber->setText(invcitem.value("invcitem_linenumber").toString());

    // TODO: should this check itemsite_controlmethod == N?
    _trackqoh = (invcitem.value("invcitem_coitem_id").toInt() > 0 &&
                 invcitem.value("itemsite_costmethod").toString() != "J");

    if (invcitem.value("invcitem_item_id").toInt() != -1)
    {
      _itemSelected->setChecked(TRUE);
      _item->setId(invcitem.value("invcitem_item_id").toInt());
      _warehouse->setId(invcitem.value("invcitem_warehous_id").toInt());
    }
    else
    {
      _miscSelected->setChecked(TRUE);
      _itemNumber->setText(invcitem.value("invcitem_number"));
      _itemDescrip->setText(invcitem.value("invcitem_descrip").toString());
      _salescat->setId(invcitem.value("invcitem_salescat_id").toInt());
    }

    _qtyUOM->setId(invcitem.value("invcitem_qty_uom_id").toInt());
    _qtyinvuomratio = invcitem.value("invcitem_qty_invuomratio").toDouble();
    _pricingUOM->setId(invcitem.value("invcitem_price_uom_id").toInt());
    _priceinvuomratio = invcitem.value("invcitem_price_invuomratio").toDouble();

    // do tax stuff before invcitem_price and _tax_* to avoid signal cascade problems
    if (! invcitem.value("taxzone_id").isNull())
      _taxzoneid = invcitem.value("taxzone_id").toInt();
	_tax->setId(invcitem.value("taxcurr_id").toInt());
    _taxtype->setId(invcitem.value("invcitem_taxtype_id").toInt());
    _altRevAccnt->setId(invcitem.value("invcitem_rev_accnt_id").toInt());

    _ordered->setDouble(invcitem.value("invcitem_ordered").toDouble());
    _billed->setDouble(invcitem.value("invcitem_billed").toDouble());

    // TODO: why not setChecked then call sHandleUpdateInv
    if ( (invcitem.value("invcitem_coitem_id").toInt() > 0) ||
         (invcitem.value("itemsite_costmethod").toString() == "J") ||
         (invcitem.value("invcitem_item_id").toInt() == -1) )
    {
      _updateInv->setChecked(false);
      _updateInv->setEnabled(false);
    }
    else
    {
      _updateInv->setChecked(invcitem.value("invcitem_updateinv").toBool());
      _updateInv->setEnabled(true);
    }
    _price->setLocalValue(invcitem.value("invcitem_price").toDouble());
    _custPrice->setLocalValue(invcitem.value("invcitem_custprice").toDouble());
    _listPrice->setBaseValue(invcitem.value("f_listprice").toDouble() * (_priceinvuomratio / _priceRatioCache));

    _custPn->setText(invcitem.value("invcitem_custpn").toString());
    _notes->setText(invcitem.value("invcitem_notes").toString());
  }
  else if (invcitem.lastError().type() != QSqlError::NoError)
  {
    systemError(this, invcitem.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  invcitem.prepare( "SELECT SUM(COALESCE(taxhist_tax, 0.00)) AS lineTaxTotal "
                    "FROM invcitem LEFT OUTER JOIN invcitemtax "
                    "  ON (invcitem_id = taxhist_parent_id) "
                    "WHERE invcitem_id = :invcitem_id;" );
  invcitem.bindValue(":invcitem_id", _invcitemid);
  invcitem.exec();
  if (invcitem.first())
    _tax->setLocalValue(invcitem.value("lineTaxTotal").toDouble());
  else if (invcitem.lastError().type() != QSqlError::NoError)
  {
    systemError(this, invcitem.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  sCalculateExtendedPrice();

  _saved = true;
  emit populated();
}

void invoiceItem::sCalculateExtendedPrice()
{
  _extended->setLocalValue((_billed->toDouble() * _qtyinvuomratio) * (_price->localValue() / _priceinvuomratio));
}

void invoiceItem::sPopulateItemInfo(int pItemid)
{
  XSqlQuery invoicePopulateItemInfo;
  if ( (_itemSelected->isChecked()) && (pItemid != -1) )
  {
    XSqlQuery uom;
    uom.prepare("SELECT uom_id, uom_name"
                "  FROM item"
                "  JOIN uom ON (item_inv_uom_id=uom_id)"
                " WHERE(item_id=:item_id)"
                " UNION "
                "SELECT uom_id, uom_name"
                "  FROM item"
                "  JOIN itemuomconv ON (itemuomconv_item_id=item_id)"
                "  JOIN uom ON (itemuomconv_to_uom_id=uom_id)"
                " WHERE((itemuomconv_from_uom_id=item_inv_uom_id)"
                "   AND (item_id=:item_id))"
                " UNION "
                "SELECT uom_id, uom_name"
                "  FROM item"
                "  JOIN itemuomconv ON (itemuomconv_item_id=item_id)"
                "  JOIN uom ON (itemuomconv_from_uom_id=uom_id)"
                " WHERE((itemuomconv_to_uom_id=item_inv_uom_id)"
                "   AND (item_id=:item_id))"
                " ORDER BY uom_name;");
    uom.bindValue(":item_id", _item->id());
    uom.exec();
    _qtyUOM->populate(uom);
    _pricingUOM->populate(uom);

    invoicePopulateItemInfo.prepare( "SELECT item_inv_uom_id, item_price_uom_id,"
               "       iteminvpricerat(item_id) AS invpricerat,"
               "       item_listprice, item_fractional, "
               "       stdcost(item_id) AS f_unitcost,"
               "       getItemTaxType(item_id, :taxzone) AS taxtype_id,"
               "       itemsite_costmethod"
               "  FROM item"
               "  JOIN itemsite ON (item_id=itemsite_item_id)"
               " WHERE ((item_id=:item_id)"
               "    AND (itemsite_warehous_id=:whsid));" );
    invoicePopulateItemInfo.bindValue(":item_id", pItemid);
    invoicePopulateItemInfo.bindValue(":taxzone", _taxzoneid);
    invoicePopulateItemInfo.bindValue(":whsid",   _warehouse->id());
    invoicePopulateItemInfo.exec();
    if (invoicePopulateItemInfo.first())
    {
      _priceRatioCache = invoicePopulateItemInfo.value("invpricerat").toDouble();
      _listPrice->setBaseValue(invoicePopulateItemInfo.value("item_listprice").toDouble());

      _invuomid = invoicePopulateItemInfo.value("item_inv_uom_id").toInt();
      _qtyUOM->setId(invoicePopulateItemInfo.value("item_inv_uom_id").toInt());
      _pricingUOM->setId(invoicePopulateItemInfo.value("item_price_uom_id").toInt());
      _qtyinvuomratio = 1.0;
      _priceinvuomratio = invoicePopulateItemInfo.value("invpricerat").toDouble();
      _unitCost->setBaseValue(invoicePopulateItemInfo.value("f_unitcost").toDouble());
      _taxtype->setId(invoicePopulateItemInfo.value("taxtype_id").toInt());
      if (invoicePopulateItemInfo.value("item_fractional").toBool())
      {
        _ordered->setValidator(omfgThis->qtyVal());
        _billed->setValidator(omfgThis->qtyVal());
      }
      else
      {
        _ordered->setValidator(new XDoubleValidator(0, 999999, 0, this));
        _billed->setValidator(new XDoubleValidator(0, 999999, 0, this));
      }
      sDeterminePrice();

      // TODO: should this check itemsite_controlmethod == N?
      _trackqoh = (invoicePopulateItemInfo.value("itemsite_costmethod").toString() != "J");
    }
    else if (invoicePopulateItemInfo.lastError().type() != QSqlError::NoError)
    {
      systemError(this, invoicePopulateItemInfo.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }
  else
  {
    _priceRatioCache = 1.0;
    _qtyinvuomratio = 1.0;
    _priceinvuomratio = 1.0;
    _qtyUOM->clear();
    _pricingUOM->clear();
    _listPrice->clear();
    _unitCost->clear();
  }
}

void invoiceItem::sDeterminePrice()
{

  if ( (_itemSelected->isChecked()) && (_item->isValid()) && (_billed->toDouble()) && (_qtyUOM->id() > 0) && (_pricingUOM->id() > 0) )
  {
    XSqlQuery itemprice;
    itemprice.prepare( "SELECT itemPrice(item_id, :cust_id, -1, "
		       "		 :qty, :qtyUOM, :priceUOM, :curr_id, :effective) AS price "
                       "FROM item "
                       "WHERE (item_id=:item_id);" );
    itemprice.bindValue(":cust_id", _custid);
    itemprice.bindValue(":qty", _billed->toDouble());
    itemprice.bindValue(":qtyUOM", _qtyUOM->id());
    itemprice.bindValue(":priceUOM", _pricingUOM->id());
    itemprice.bindValue(":item_id", _item->id());
    itemprice.bindValue(":curr_id", _price->id());
    itemprice.bindValue(":effective", _price->effective());
    itemprice.exec();
    if (itemprice.first())
    {
      if (itemprice.value("price").toDouble() == -9999.0)
      {
        QMessageBox::critical( this, tr("Customer Cannot Buy at Quantity"),
                               tr("<p>This item is marked as exclusive and "
                                  "no qualifying price schedule was found. "
                                  "You may click on the price list button "
				  "(...) next to the Unit Price to determine "
                                  "if there is a minimum quantity the selected "
				  "Customer may purchase." ) );
        _custPrice->clear();
        _price->clear();
        _billed->clear();

        _billed->setFocus();
	return;
      }
      double price = itemprice.value("price").toDouble();
      //price = price * (_priceinvuomratio / _priceRatioCache);
      _custPrice->setLocalValue(price);
      _price->setLocalValue(price);
    }
    else if (itemprice.lastError().type() != QSqlError::NoError)
    {
      systemError(this, itemprice.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }
}

void invoiceItem::sPriceGroup()
{
  if (! omfgThis->singleCurrency())
    _priceGroup->setTitle(tr("In %1:").arg(_price->currAbbr()));
}

void invoiceItem::sTaxDetail()
{
  taxDetail newdlg(this, "", true);
  ParameterList params;
  params.append("taxzone_id", _taxzoneid);
  params.append("taxtype_id", _taxtype->id());
  params.append("date", _price->effective());
  params.append("subtotal",  CurrDisplay::convert(_extended->id(), _tax->id(),
						 _extended->localValue(),
						 _extended->effective()));
  params.append("curr_id",  _tax->id());
  
  if(cView == _mode)
    params.append("readOnly");
  
  if(_saved == true)
  {
	params.append("order_id", _invcitemid);
    params.append("order_type", "II");
  }

  newdlg.set(params);
  
  if (newdlg.set(params) == NoError && newdlg.exec())
  {
    if (_taxtype->id() != newdlg.taxtype())
      _taxtype->setId(newdlg.taxtype());
  }
}

void invoiceItem::sQtyUOMChanged()
{
  if(_qtyUOM->id() == _invuomid)
    _qtyinvuomratio = 1.0;
  else
  {
    XSqlQuery invuom;
    invuom.prepare("SELECT itemuomtouomratio(item_id, :uom_id, item_inv_uom_id) AS ratio"
                   "  FROM item"
                   " WHERE(item_id=:item_id);");
    invuom.bindValue(":item_id", _item->id());
    invuom.bindValue(":uom_id", _qtyUOM->id());
    invuom.exec();
    if(invuom.first())
      _qtyinvuomratio = invuom.value("ratio").toDouble();
    else
      systemError(this, invuom.lastError().databaseText(), __FILE__, __LINE__);
  }

  if(_qtyUOM->id() != _invuomid)
  {
    _pricingUOM->setId(_qtyUOM->id());
    _pricingUOM->setEnabled(false);
  }
  else
    _pricingUOM->setEnabled(true);
  sDeterminePrice();
  sCalculateExtendedPrice();
}

void invoiceItem::sPriceUOMChanged()
{
  if(_pricingUOM->id() == -1 || _qtyUOM->id() == -1)
    return;

  if(_pricingUOM->id() == _invuomid)
    _priceinvuomratio = 1.0;
  else
  {
    XSqlQuery invuom;
    invuom.prepare("SELECT itemuomtouomratio(item_id, :uom_id, item_inv_uom_id) AS ratio"
                   "  FROM item"
                   " WHERE(item_id=:item_id);");
    invuom.bindValue(":item_id", _item->id());
    invuom.bindValue(":uom_id", _pricingUOM->id());
    invuom.exec();
    if(invuom.first())
      _priceinvuomratio = invuom.value("ratio").toDouble();
    else
      systemError(this, invuom.lastError().databaseText(), __FILE__, __LINE__);
  }

  XSqlQuery item;
  item.prepare("SELECT item_listprice"
               "  FROM item"
               " WHERE(item_id=:item_id);");
  item.bindValue(":item_id", _item->id());
  item.exec();
  item.first();
  _listPrice->setBaseValue(item.value("item_listprice").toDouble() * (_priceinvuomratio / _priceRatioCache));
  sDeterminePrice();
  sCalculateExtendedPrice();
}

void invoiceItem::sMiscSelected(bool isMisc)
{
  if(isMisc)
    _item->setId(-1);
}

void invoiceItem::sHandleUpdateInv()
{
  if( (_item->isValid()) &&
      (_warehouse->isValid()) &&
      (_trackqoh) )
  {
    XSqlQuery invq;
    invq.prepare("SELECT itemsite_id FROM itemsite "
                 "WHERE (itemsite_item_id=:item_id) "
                 "  AND (itemsite_warehous_id=:warehous_id) "
                 "  AND (itemsite_controlmethod != 'N');");
    invq.bindValue(":item_id", _item->id());
    invq.bindValue(":warehous_id", _warehouse->id());
    invq.exec();
    if (invq.first())
    {
      _updateInv->setEnabled(true);
    }
    else if (invq.lastError().type() != QSqlError::NoError)
    {
      systemError(this, invq.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
    else
    {
      _updateInv->setChecked(false);
      _updateInv->setEnabled(false);
    }
  }
  else
  {
    _updateInv->setChecked(false);
    _updateInv->setEnabled(false);
  }
}

void invoiceItem::sListPrices()
{
  ParameterList params;
  params.append("cust_id",   _custid);
  //params.append("shipto_id", _shiptoid);
  params.append("item_id",   _item->id());
  params.append("warehous_id", _warehouse->id());
  params.append("qty",       _billed->toDouble() * _qtyinvuomratio);
  params.append("curr_id",   _price->id());
  params.append("effective", _price->effective());

  priceList newdlg(this);
  newdlg.set(params);
  if ( (newdlg.exec() == XDialog::Accepted) &&
       (_privileges->check("OverridePrice")) &&
       (!_metrics->boolean("DisableSalesOrderPriceOverride")) )
  {
    _price->setLocalValue(newdlg._selectedPrice * (_priceinvuomratio / _priceRatioCache));
  }
}

void invoiceItem::sLookupTax()
{
  XSqlQuery taxcal;
  taxcal.prepare("SELECT calculatetax(:taxzone_id, :taxtype_id, :date, :curr_id, :amount) AS taxamount;");
  taxcal.bindValue(":taxzone_id", _taxzoneid);
  taxcal.bindValue(":taxtype_id", _taxtype->id());
  taxcal.bindValue(":date", _price->effective());
  taxcal.bindValue(":curr_id", _tax->id());
  taxcal.bindValue(":amount", CurrDisplay::convert(_extended->id(), _tax->id(), _extended->localValue(), _extended->effective()));
  taxcal.exec();
  if (taxcal.first())
  {
    _tax->setLocalValue(taxcal.value("taxamount").toDouble());
	_saved = false;
  }
  else if (taxcal.lastError().type() != QSqlError::NoError)
  {
    systemError(this, taxcal.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }
}
