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

#ifndef KCOMBOBOX_DIALOG_H
#define KCOMBOBOX_DIALOG_H

#include <KConfig>
#include <KDialog>
#include <KGlobal>

class QCheckBox;
class KHistoryComboBox;

///Dialog for user to choose an item from a QStringList.
class KComboBoxDialog : public KDialog
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
	KComboBoxDialog( const QString &_text, const QStringList& _items, const QString& _value = QString(), bool showDontAskAgain = false, QWidget *parent = 0 );
	virtual ~KComboBoxDialog();

	///@return the value the user chose
	QString text() const;

	///@return the line edit widget
	KHistoryComboBox *comboBox() const { return combo; }

	/**
	 * Static convenience function to get input from the user.
	 *
	 * @param _text            Text of the label
	 * @param _items           Items in the combobox
	 * @param _value           Initial value of the inputline
	 * @param dontAskAgainName Name for saving whether the user doesn't want to be asked again; use QString() to disable
	 */
	static QString getItem( const QString &_text, const QStringList &_items, const QString& _value = QString(), const QString &dontAskAgainName = QString(), QWidget *parent = 0 );

	/**
	 * Static convenience function to get input from the user.
	 * This method includes a caption.
	 *
	 * @param _caption         Caption of the dialog
	 * @param _text            Text of the label
	 * @param _items           Items in the combobox
	 * @param _value           Initial value of the inputline
	 * @param dontAskAgainName Name for saving whether the user doesn't want to be asked again; use QString() to disable
	 */
	static QString getItem( const QString &_text, const QString &_caption, const QStringList &_items, const QString& _value = QString(), const QString &dontAskAgainName = QString(), QWidget *parent = 0 );

	/**
	 * Static convenience method.
	 * This method is meant as a replacement for KLineEditDlg::getText() for cases
	 * when a history and autocompletion are desired.
	 *
	 * @param _caption         Caption of the dialog
	 * @param _text            Text of the label
	 * @param _value           Initial value of the inputline
	 * @param ok               Variable to store whether the user hit OK
	 * @param parent           Parent widget for the dialog
	 * @param configName       Name of the dialog for saving the completion and history
	 * @parma config           KConfig for saving the completion and history
	 */
	static QString getText(const QString &_caption, const QString &_text,
	                       const QString &_value = QString(),
	                       bool *ok = 0, QWidget *parent = 0,
	                       const QString &configName = QString(),
	                       KSharedConfigPtr config = KGlobal::config());

protected:
	KHistoryComboBox *combo;
	QCheckBox *dontAskAgainCheckBox;
	bool dontAskAgainChecked();
};

#endif
