/*
    Copyright (C) 2002-2005, Jason Katz-Brown <jasonkb@mit.edu>

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

#if 0
#ifndef KVOLUMECONTROL_H
#define KVOLUMECONTROL_H

#include <arts/kplayobject.h>
#include <arts/artsflow.h>
#include <kdebug.h>
#include <QObject>

class KVolumeControl : public QObject
{
Q_OBJECT

public:
	KVolumeControl(Arts::SoundServerV2 server, KPlayObject *parent);
	KVolumeControl(double vol, Arts::SoundServerV2 server, KPlayObject *parent);
	~KVolumeControl();

	void setVolume(double);
	double volume(void);

	void init(Arts::SoundServerV2 server);

private:
	Arts::StereoVolumeControl volumeControl;
	Arts::Synth_AMAN_PLAY manager;
};

#endif
#endif
