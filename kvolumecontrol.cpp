#include <kdebug.h>

#include <arts/soundserver.h>
#include <arts/flowsystem.h>

#include "kvolumecontrol.h"

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
		kdError() << "Your OS is broken.  Get an OS that installs KDE decently." << endl;
		return;
	}
	manager.start();

	volumeControl = Arts::DynamicCast(server.createObject("Arts::StereoVolumeControl"));
	if (volumeControl.isNull())
	{
		kdError() << "Your OS is broken.  Get an OS that installs KDE decently." << endl;
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
