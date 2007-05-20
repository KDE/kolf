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
#include "kvolumecontrol.h"

#include <kdebug.h>

#include <arts/soundserver.h>
#include <arts/flowsystem.h>

KVolumeControl::KVolumeControl(Arts::SoundServerV2 server, KPlayObject *parent)
	: QObject(parent)
{
	init(server);
}

KVolumeControl::KVolumeControl(double vol, Arts::SoundServerV2 server, KPlayObject *parent)
	: QObject(parent)
{
	init(server);
	setVolume(vol);
}

KVolumeControl::~KVolumeControl()
{
	manager.stop();
	volumeControl.stop();
}

void KVolumeControl::init(Arts::SoundServerV2 server)
{
	manager = Arts::DynamicCast(server.createObject("Arts::Synth_AMAN_PLAY"));
	if (manager.isNull())
	{
		kError() << "Your OS is broken.  Get an OS that installs KDE decently." << endl;
		return;
	}
	manager.start();

	volumeControl = Arts::DynamicCast(server.createObject("Arts::StereoVolumeControl"));
	if (volumeControl.isNull())
	{
		kError() << "Your OS is broken.  Get an OS that installs KDE decently." << endl;
		return;
	}
	volumeControl.start();

	Arts::connect((static_cast<KPlayObject *>(parent()))->object(), "left", volumeControl, "inleft");
	Arts::connect((static_cast<KPlayObject *>(parent()))->object(), "right", volumeControl, "inright");

	Arts::connect(volumeControl, manager);
}

void KVolumeControl::setVolume(double d)
{
	if (volumeControl.isNull())
		return;

	volumeControl.scaleFactor(d);
}

double KVolumeControl::volume(void)
{
	if (volumeControl.isNull())
		return -1;

	return volumeControl.scaleFactor();
}

#include "kvolumecontrol.moc"
#endif
