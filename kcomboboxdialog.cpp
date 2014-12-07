// Copyright (C) 2002 Jason Katz-Brown <jason@katzbrown.com>
// Copyright (C) 2002 Neil Stevens <neil@qualityassistant.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// THE AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// Except as contained in this notice, the name(s) of the author(s) shall not be
// used in advertising or otherwise to promote the sale, use or other dealings
// in this Software without prior written authorization from the author(s).

#include "kcomboboxdialog.h"

#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <KHistoryComboBox>
#include <KLocale>

KComboBoxDialog::KComboBoxDialog( const QString &_text, const QStringList &_items, const QString& _value, bool showDontAskAgain, QWidget *parent )
	: KDialog( parent)
{
	setButtons(Ok);
	setDefaultButton(Ok);
	setModal(true);
	showButtonSeparator(true);
	QFrame *frame = new QFrame(this);
	setMainWidget(frame);
	QVBoxLayout *topLayout = new QVBoxLayout( frame );
        topLayout->setMargin( marginHint() );
        topLayout->setSpacing( spacingHint() );
	QLabel *label = new QLabel(_text, frame );
	topLayout->addWidget( label, 1 );

	combo = new KHistoryComboBox( frame);
	combo->setEditable(false);
	combo->addItems( _items );
	topLayout->addWidget( combo, 1 );

	if (showDontAskAgain)
	{
		dontAskAgainCheckBox = new QCheckBox( i18n("&Do not ask again"), frame );
		topLayout->addWidget( dontAskAgainCheckBox, 1 );
	}
	else
		dontAskAgainCheckBox = 0;

	if ( !_value.isNull() )
		combo->setCurrentItem( _value );
	combo->setFocus();
}

KComboBoxDialog::~KComboBoxDialog()
{
}

QString KComboBoxDialog::text() const
{
	return combo->currentText();
}

bool KComboBoxDialog::dontAskAgainChecked()
{
	if (dontAskAgainCheckBox)
		return dontAskAgainCheckBox->isChecked();

	return false;
}

QString KComboBoxDialog::getItem( const QString &_text, const QStringList &_items, const QString& _value, const QString &dontAskAgainName, QWidget *parent )
{
	return getItem( _text, QString(), _items, _value, dontAskAgainName, parent );
}

QString KComboBoxDialog::getItem( const QString &_text, const QString &_caption, const QStringList &_items, const QString& _value, const QString &dontAskAgainName, QWidget *parent )
{
	QString prevAnswer;
	if ( !dontAskAgainName.isEmpty() )
	{
		KSharedConfig::Ptr config = KSharedConfig::openConfig();
		KConfigGroup *configGroup = new KConfigGroup(config->group("Notification Messages"));
		prevAnswer = configGroup->readEntry( dontAskAgainName,QString() );
		if ( !prevAnswer.isEmpty() )
			if ( _items.contains( prevAnswer ) > 0 )
				return prevAnswer;
	}

	KComboBoxDialog dlg( _text, _items, _value, !dontAskAgainName.isNull(), parent );
	if ( !_caption.isNull() )
		dlg.setCaption( _caption );

	dlg.exec();

	const QString text = dlg.text();

	if (dlg.dontAskAgainChecked())
	{
		if ( !dontAskAgainName.isEmpty() && !text.isEmpty() )
		{
			KSharedConfig::Ptr config = KSharedConfig::openConfig();
			KConfigGroup *configGroup = new KConfigGroup(config->group("Notification Messages"));
			configGroup->writeEntry( dontAskAgainName, text );
		}
	}

	return text;
}

QString KComboBoxDialog::getText(const QString &_caption, const QString &_text, const QString &_value, bool *ok, QWidget *parent, const QString &configName, KSharedConfigPtr config)
{
	KConfigGroup *configGroup = 0;
	KComboBoxDialog dlg(_text, QStringList(), _value, false, parent);
	if ( !_caption.isNull() )
		dlg.setCaption( _caption );

	KHistoryComboBox * const box = dlg.comboBox();
	box->setEditable(true);

	const QString historyItem = QString("%1History").arg(configName);
	const QString completionItem = QString("%1Completion").arg(configName);

	if(!configName.isNull())
	{
		configGroup = new KConfigGroup(config->group("KComboBoxDialog"));
		box->setHistoryItems(configGroup->readEntry(historyItem,QStringList()));
		box->completionObject()->setItems(configGroup->readEntry(completionItem,QStringList()));
	}

	bool result = dlg.exec();
	if(ok) *ok = result;

	if(!configName.isNull() && result)
	{
		box->addToHistory(dlg.text());
		box->completionObject()->addItem(dlg.text());
		configGroup = new KConfigGroup(config->group("KComboBoxDialog"));
		configGroup->writeEntry(historyItem, box->historyItems());
		configGroup->writeEntry(completionItem, box->completionObject()->items());
	}

	return dlg.text();
}


