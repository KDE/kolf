#ifndef KCOMBOBOX_DIALOG_H
#define KCOMBOBOX_DIALOG_H

#include <qstringlist.h>

#include <kdialogbase.h>

class QCheckBox;
class KComboBox;

/**
 * Dialog for user to choose an item from a QStringList.
 */

class KComboBoxDialog : public KDialogBase
{
Q_OBJECT

public:
	/**
	 * Create a dialog that asks for a single line of text. _value is
	 * the initial value of the line. _text appears as the current text
	 * of the combobox.
	 *
	 * @param _items     Items in the combobox
	 * @param _text      Text of the label
	 * @param _value     Initial value of the combobox
	 */
	KComboBoxDialog( const QString &_text, const QStringList& _items, const QString& _value = QString::null, bool showDontAskAgain = false, QWidget *parent = 0 );
	virtual ~KComboBoxDialog();

	/**
	 * @return the value the user chose
	 */
	QString text() const;

	/**
	 * @return the line edit widget
	 */
	KComboBox *comboBox() const { return combo; }

	/**
	 * Static convenience function to get input from the user.
	 *
	 * @param _text            Text of the label
	 * @param _items           Items in the combobox
	 * @param _value           Initial value of the inputline
	 * @param dontAskAgainName Name for saving whether the user doesn't want to be asked again; use QString::null to disable
	 */
	static QString getItem( const QString &_text, const QStringList &_items, const QString& _value = QString::null, const QString &dontAskAgainName = QString::null, QWidget *parent = 0 );

	/**
	 * Static convenience function to get input from the user.
	 * This method includes a caption.
	 *
	 * @param _caption         Caption of the dialog
	 * @param _text            Text of the label
	 * @param _items           Items in the combobox
	 * @param _value           Initial value of the inputline
	 * @param dontAskAgainName Name for saving whether the user doesn't want to be asked again; use QString::null to disable
	 */
	static QString getItem( const QString &_text, const QString &_caption, const QStringList &_items, const QString& _value = QString::null, const QString &dontAskAgainName = QString::null, QWidget *parent = 0 );

protected:
	KComboBox *combo;
	QCheckBox *dontAskAgainCheckBox;
	bool dontAskAgainChecked();
};

#endif
