#ifndef CANVASITEM_H
#define CANVASITEM_H

#include <qcanvas.h>
#include <qlabel.h>
#include <qlayout.h>

#include <klocale.h>

#include "config.h"

// internal
class DefaultConfig : public Config
{
	public:
		DefaultConfig(QWidget *parent)
			: Config(parent)
		{
			QVBoxLayout *layout = new QVBoxLayout(this, marginHint(), spacingHint());
			QHBoxLayout *hlayout = new QHBoxLayout(layout, spacingHint());
			hlayout->addStretch();
			hlayout->addWidget(new QLabel(i18n("No configuration options"), this));
			hlayout->addStretch();
		}
};

class Ball;
class KSimpleConfig;
class StateDB;
class KolfGame;

// this is mostly just internal -- don't use it

class CanvasItem
{
public:
	CanvasItem() { game = 0; }
	virtual ~CanvasItem() {}
	/**
	 * load your settings from the KSimpleConfig, which represents a course.
	 */
	virtual void load(KSimpleConfig *) {}
	/**
	 * load a point if you wish. Rarely necessary.
	 */
	virtual void loadState(StateDB * /*db*/) {}
	/**
	 * returns a bool that is true if your item needs to load after other items
	 */
	virtual bool loadLast() { return false; }
	/**
	 * called after the item is moved the very first time by the game
	 */
	virtual void firstMove(int /*x*/, int /*y*/) {}
	/**
	 * save your settings.
	 */
	virtual void save(KSimpleConfig *cfg);
	/**
	 * save a point if you wish. Rarely necessary.
	 */
	virtual void saveState(StateDB * /*db*/) {}
	/**
	 * called right before any items are saved.
	 */
	virtual void aboutToSave() {}
	/**
	 * called right after all items are saved.
	 */
	virtual void savingDone() {}
	/**
	 * called when the edit mode has been changed.
	 */
	virtual void editModeChanged(bool /*changed*/) {}
	/**
	 * the item should delete any other objects it's created.
	 * DO NOT DO THIS IN THE DESTRUCTOR!
	 */
	virtual void aboutToDie() {}
	/**
	 * called when user presses delete key while editing. This is very rarely reimplemented, and generally shouldn't be.
	 */
	virtual void aboutToDelete() {}
	/**
	 * returns whether this item should be able to be deleted by user while editing.
	 */
	virtual bool deleteable() { return true; }
	/**
	 * returns whether this item should get doAdvance called -- it is called in sync with ball advancing (which is twice as fast as the advance() calling rate)
	 */
	virtual bool fastAdvance() { return false; }
	/**
	 * called when all items have had their chance at a doAdvance
	 */
	virtual void fastAdvanceDone() {}
	/**
	 * called if fastAdvance is enabled
	 */
	virtual void doAdvance() {}
	/**
	 * returns whether or not this item lifts items on top of it.
	 */
	virtual bool vStrut() const { return false; }
	/**
	 * show extra item info
	 */
	virtual void showInfo() {};
	/**
	 * hide extra item info
	 */
	virtual void hideInfo() {};
	/**
	 * update your Z value (this is called by various things when perhaps the value should change) if this is called by a vStrut, it will pass 'this'.
	 */
	virtual void updateZ(QCanvasRectangle * /*vStrut*/ = 0) {};
	/**
	 * clean up for prettyness
	 */
	virtual void clean() {};
	/**
	 * returns whether this item can be moved by others (if you want to move an item, you should honor this!)
	 */
	virtual bool canBeMovedByOthers() const { return false; }
	/**
	 * returns a Config that can be used to configure this item by the user.
	 * The default implementation returns one that says 'No configuration options'.
	 */
	virtual Config *config(QWidget *parent) { return new DefaultConfig(parent); }
	/**
	 * returns other items that should be moveable (besides this one of course).
	 */
	virtual QPtrList<QCanvasItem> moveableItems() { return QPtrList<QCanvasItem>(); }
	/**
	 * returns whether this can be moved by the user while editing.
	 */
	virtual bool moveable() const { return true; }

	void setId(int newId) { id = newId; }
	int curId() const { return id; }

	/**
	 * call to play sound (ie, playSound("wall") plays kdedir/share/apps/kolf/sounds/wall.wav)
	 */
	void playSound(QString file);

	virtual void collision(Ball * /*ball*/, long int /*id*/) {};

	/**
	 * reimplement if you want extra items to have access to the game object.
	 * playSound() relies on having this.
	 */
	virtual void setGame(KolfGame *game) { this->game = game; }

	/**
	 * returns whether this resizes from south-east.
	 */
	virtual bool cornerResize() { return false; }

	QString name() { return m_name; }
	void setName(const QString &newname) { m_name = newname; }

protected:
	/**
	 * pointer to main KolfGame
	 */
	KolfGame *game;

	/**
	 * returns the highest vertical strut the item is on
	 */
	QCanvasRectangle *onVStrut();

private:
	QString m_name;
	int id;
};

#endif
