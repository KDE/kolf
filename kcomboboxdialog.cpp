#include <qcheckbox.h>
#include <qlabel.h>
#include <qlayout.h>

#include <klocale.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>

#include "kcomboboxdialog.h"

KComboBoxDialog::KComboBoxDialog( const QString &_text, const QStringList &_items, const QString& _value, bool showDontAskAgain, QWidget *parent )
	: KDialogBase( Plain, QString::null, Ok, Ok, parent, 0L, true, true )
{
	QVBoxLayout *topLayout = new QVBoxLayout( plainPage(), marginHint(), spacingHint() );
	QLabel *label = new QLabel(_text, plainPage() );
	topLayout->addWidget( label, 1 );

	combo = new KComboBox( plainPage() );
	combo->insertStringList( _items );
	topLayout->addWidget( combo, 1 );

	if (showDontAskAgain)
	{
		dontAskAgainCheckBox = new QCheckBox( i18n("&Do not ask again"), plainPage() );
		topLayout->addWidget( dontAskAgainCheckBox, 1 );
	}
	else
		dontAskAgainCheckBox = 0;

	if ( !_value.isNull() )
		combo->setCurrentText( _value );
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
	return getItem( _text, QString::null, _items, _value, dontAskAgainName, parent );
}

QString KComboBoxDialog::getItem( const QString &_text, const QString &_caption, const QStringList &_items, const QString& _value, const QString &dontAskAgainName, QWidget *parent )
{
	QString prevAnswer;
	if ( !dontAskAgainName.isEmpty() )
	{
		KConfig *config = KGlobal::config();
		config->setGroup( "Notification Messages" );
		prevAnswer = config->readEntry( dontAskAgainName );
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
			KConfig *config = KGlobal::config();
			config->setGroup ( "Notification Messages" );
			config->writeEntry( dontAskAgainName, text );
		}
	}

	return text;
}

#include "kcomboboxdialog.moc"
