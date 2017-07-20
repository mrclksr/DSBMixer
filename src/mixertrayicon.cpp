#include "mixertrayicon.h"


MixerTrayIcon::MixerTrayIcon(Mixer *mixer, const QIcon &icon, QObject *parent) : QSystemTrayIcon(icon, parent) {
	this->mixer = mixer;
}

void
MixerTrayIcon::setMixer(Mixer *mixer)
{
	this->mixer = mixer;
}

