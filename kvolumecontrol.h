#ifndef KVOLUMECONTROL_H
#define KVOLUMECONTROL_H

#include <arts/kplayobject.h>
#include <arts/artsflow.h>
#include <kdebug.h>
#include <qobject.h>

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
