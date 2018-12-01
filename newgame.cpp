/*
    Copyright (C) 2002-2005, Jason Katz-Brown <jasonkb@mit.edu>
    Copyright 2010 Stefan Majewsky <majewsky@gmx.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "newgame.h"
#include "game.h"

#include <QBoxLayout>
#include <QFileDialog>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QStandardPaths>
#include <QUrl>
#include <KLocalizedString>
#include <KMessageBox>
#include <KScoreDialog>
#include <KSeparator>
#include <KSharedConfig>

NewGameDialog::NewGameDialog(bool enableCourses)
	: KPageDialog()
{
	setWindowTitle(i18n("New Game"));
	buttonBox()->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	setMinimumSize(640,310);
	connect(buttonBox(), &QDialogButtonBox::accepted, this, &NewGameDialog::slotOk);

	setFaceType(KPageDialog::Tree);
	this->enableCourses = enableCourses;

	KSharedConfig::Ptr config = KSharedConfig::openConfig();
        // following use this group
        KConfigGroup configGroup(config->group(QString("New Game Dialog Mode")));

	// lots o' colors :)
	startColors << Qt::blue << Qt::red << Qt::yellow << Qt::lightGray << Qt::cyan << Qt::darkBlue << Qt::magenta << Qt::darkGray << Qt::darkMagenta << Qt::darkYellow;

	playerPage = new QFrame();
    	addPage(playerPage, i18n("Players"));

	QVBoxLayout *bigLayout = new QVBoxLayout(playerPage);

	addButton = new QPushButton(i18n("&New Player"), playerPage);
	bigLayout->addWidget(addButton);

	connect(addButton, &QPushButton::clicked, this, &NewGameDialog::addPlayer);

	scroller = new QScrollArea(playerPage);
	bigLayout->addWidget(scroller);
	playersWidget = new QWidget(playerPage);
	scroller->setWidget(playersWidget);
	new QVBoxLayout(playersWidget);

	QMap<QString, QString> entries = config->entryMap("New Game Dialog");
	int i = 0;
	for (QMap<QString, QString>::Iterator it = entries.begin(); it != entries.end(); ++it)
	{
		if (i > startColors.count())
			return;

		addPlayer();
		editors.last()->setName(it.key().right(it.key().length() - 1));
		editors.last()->setColor(QColor(it.value()));
		++i;
	}

	if (editors.isEmpty())
	{
		addPlayer();
		addPlayer();
	}

	enableButtons();
	KPageWidgetItem *pageItem =0L;
	if (enableCourses)
	{
		coursePage = new QFrame();
		pageItem = new KPageWidgetItem( coursePage, i18n("Choose Course to Play") );
		pageItem->setHeader(i18n("Course"));
		addPage(pageItem);
		QVBoxLayout *coursePageLayout = new QVBoxLayout(coursePage);

		QHBoxLayout *hlayout = new QHBoxLayout;
                coursePageLayout->addLayout( hlayout );


		// find other courses
		externCourses = configGroup.readEntry("extra",QStringList());

		/// course loading
		QStringList files;
		const QStringList dirs = QStandardPaths::locateAll(QStandardPaths::AppDataLocation, "courses", QStandardPaths::LocateDirectory);
		Q_FOREACH (const QString& dir, dirs) {
			const QStringList fileNames = QDir(dir).entryList(QStringList() << QStringLiteral("*"), QDir::Files);
			Q_FOREACH (const QString& file, fileNames) {
				files.append(dir + '/' + file);
			}
		}
		const QStringList items = externCourses + files;

		QStringList nameList;
		const QString lastCourse(configGroup.readEntry("course", ""));
		int curItem = 0;
		i = 0;
		for (QStringList::const_iterator it = items.begin(); it != items.end(); ++it, ++i)
		{
			QString file = *it;
			CourseInfo curinfo;
			KolfGame::courseInfo(curinfo, file);
			info[file] = curinfo;
			names.append(file);
			nameList.append(curinfo.name);

			if (lastCourse == file)
				curItem = i;
		}

		const QString newName(i18n("Create New"));
		info[QString()] = CourseInfo(newName, newName, i18n("You"), 0, 0);
		names.append(QString());
		nameList.append(newName);

		courseList = new QListWidget(coursePage);
		hlayout->addWidget(courseList);
		courseList->addItems(nameList);
		courseList->setCurrentRow(curItem);
		connect(courseList, &QListWidget::currentRowChanged, this, &NewGameDialog::courseSelected);
		connect(courseList, &QListWidget::itemSelectionChanged, this, &NewGameDialog::selectionChanged);

		QVBoxLayout *detailLayout = new QVBoxLayout;
                hlayout->addLayout( detailLayout );
		name = new QLabel(coursePage);
		detailLayout->addWidget(name);
		author = new QLabel(coursePage);
		detailLayout->addWidget(author);

		QHBoxLayout *minorLayout = new QHBoxLayout;
                detailLayout->addLayout( minorLayout );
		par = new QLabel(coursePage);
		minorLayout->addWidget(par);
		holes = new QLabel(coursePage);
		minorLayout->addWidget(holes);

		detailLayout->addStretch();
		QPushButton *scores = new QPushButton(i18n("Highscores"), coursePage);
		connect(scores, &QPushButton::clicked, this, &NewGameDialog::showHighscores);
		detailLayout->addWidget(scores);

		detailLayout->addStretch();
		detailLayout->addWidget(new KSeparator(coursePage));

		minorLayout = new QHBoxLayout;
                detailLayout->addLayout( minorLayout );

		QPushButton *addCourseButton = new QPushButton(i18n("Add..."), coursePage);
		minorLayout->addWidget(addCourseButton);
		connect(addCourseButton, &QPushButton::clicked, this, &NewGameDialog::addCourse);

		remove = new QPushButton(i18n("Remove"), coursePage);
		minorLayout->addWidget(remove);
		connect(remove, &QPushButton::clicked, this, &NewGameDialog::removeCourse);

		courseSelected(curItem);
		selectionChanged();
	}

	// options page
    optionsPage = new QFrame();
    pageItem = new KPageWidgetItem( optionsPage, i18n("Game Options") );
    pageItem->setHeader(i18n("Options"));
    addPage(pageItem);

	QVBoxLayout *vlayout = new QVBoxLayout(optionsPage);

	mode = new QCheckBox(i18n("&Strict mode"), optionsPage);
	vlayout->addWidget(mode);
	mode->setChecked(configGroup.readEntry("competition", false));

	QLabel *desc = new QLabel(i18n("In strict mode, undo, editing, and switching holes is not allowed. This is generally for competition. Only in strict mode are highscores kept."), optionsPage);
	desc->setTextFormat(Qt::RichText);
	desc->setWordWrap(true);
	vlayout->addWidget(desc);
}

NewGameDialog::~NewGameDialog()
{
	qDeleteAll(editors);
}

void NewGameDialog::slotOk()
{
	KSharedConfig::Ptr config = KSharedConfig::openConfig();
	KConfigGroup configGroup(config->group(QString("New Game Dialog Mode")));

	configGroup.writeEntry("competition", mode->isChecked());
	if (enableCourses)
	{
		configGroup.writeEntry("course", currentCourse);
		configGroup.writeEntry("extra", externCourses);
	}

	config->deleteGroup("New Game Dialog");

	PlayerEditor *curEditor = 0;
	int i = 0;
	for (; i < editors.count(); ++i) {
		curEditor = editors.at(i);
		configGroup.writeEntry(QString::number(i) + curEditor->name(), curEditor->color().name());
	}

	config->sync();
}

void NewGameDialog::courseSelected(int index)
{
	//BUG 274418: select first course if nothing selected (should not happen, but meh)
	if (index < 0)
		index = 0;
	currentCourse = names.at(index);

	CourseInfo &curinfo = info[currentCourse];

	name->setText(QString("<strong>%1</strong>").arg(curinfo.name));

	author->setText(i18n("By %1", curinfo.author));
	par->setText(i18n("Par %1", curinfo.par));
	holes->setText(i18n("%1 Holes", curinfo.holes));
}

void NewGameDialog::showHighscores()
{
	KScoreDialog *scoreDialog = new KScoreDialog(KScoreDialog::Name | KScoreDialog::Custom1 | KScoreDialog::Score, this);
	scoreDialog->addField(KScoreDialog::Custom1, i18n("Par"), "Par");
	scoreDialog->setConfigGroup(qMakePair(QByteArray(info[currentCourse].untranslatedName.toUtf8() + " Highscores"), i18n("High Scores for %1", info[currentCourse].name)));
	scoreDialog->setComment(i18n("High Scores for %1", info[currentCourse].name));
	scoreDialog->show();
}

void NewGameDialog::removeCourse()
{
	QListWidgetItem* curItem = courseList->currentItem();
	if (!curItem)
		return;

	QString file = curItem->text();
	if (!externCourses.contains(file))
		return;

	names.removeAll(file);
	externCourses.removeAll(file);
	delete courseList->takeItem(courseList->currentRow());

	selectionChanged();
}

void NewGameDialog::selectionChanged()
{
	QListWidgetItem* curItem = courseList->currentItem();
	remove->setEnabled(curItem && externCourses.contains(curItem->text()));
}

void NewGameDialog::addCourse()
{
	const QStringList files = QFileDialog::getOpenFileNames( this, i18n("Pick Kolf Course"),
			 QString(), i18n("Saved Kolf game (*.kolfgame);;All files (*.*)") );

	bool hasDuplicates = false;

	for (QStringList::const_iterator fileIt = files.begin(); fileIt != files.end(); ++fileIt)
	{
		if (names.contains(*fileIt) > 0)
		{
			hasDuplicates = true;
			continue;
		}

		CourseInfo curinfo;
		KolfGame::courseInfo(curinfo, *fileIt);
		info[*fileIt] = curinfo;
		names.prepend(*fileIt);
		externCourses.prepend(*fileIt);

		courseList->insertItem(0, new QListWidgetItem(curinfo.name));
	}

	if (hasDuplicates)
		KMessageBox::information(this, i18n("Chosen course is already on course list."));

	courseList->setCurrentItem(0);
	courseSelected(0);
	selectionChanged();
}

void NewGameDialog::addPlayer()
{
	if (editors.count() >= startColors.count())
		return;


	PlayerEditor *pe = new PlayerEditor(i18n("Player %1", editors.count() + 1), startColors.at(editors.count()), playersWidget);
	editors.append(pe);
	pe->show();
	playersWidget->layout()->addWidget(pe);
	connect(pe, &PlayerEditor::deleteEditor, this, &NewGameDialog::deleteEditor);

	enableButtons();
	playersWidget->setMinimumSize(playersWidget->sizeHint());
}

void NewGameDialog::deleteEditor(PlayerEditor *editor)
{
	if (editors.count() < 2)
		return;

	editors.removeAll(editor);
	delete editor;

	enableButtons();
	playersWidget->setMinimumSize(playersWidget->sizeHint());
	playersWidget->resize(playersWidget->sizeHint());
}

void NewGameDialog::enableButtons()
{
	addButton->setEnabled(!(editors.count() >= startColors.count()));
}

/////////////////////////

PlayerEditor::PlayerEditor(QString startName, QColor startColor, QWidget *parent)
	: QWidget(parent)
{
	QHBoxLayout *layout = new QHBoxLayout(this);

	editor = new KLineEdit(this);
	layout->addWidget(editor);
	editor->setFrame(false);
	editor->setText(startName);
	layout->addStretch();
	layout->addWidget(colorButton = new KColorButton(startColor, this));
	colorButton->hide();
	QPushButton *remove = new QPushButton(i18n("Remove"), this);
	layout->addWidget(remove);
	connect(remove, &QPushButton::clicked, this, &PlayerEditor::removeMe);
}

void PlayerEditor::removeMe()
{
	emit deleteEditor(this);
}


