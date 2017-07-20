#include <QObject>
#include <QEvent>
#include <QSystemTrayIcon>
#include <QWheelEvent>
#include <QWidget>
#include "mixer.h"

class MixerTrayIcon : public QSystemTrayIcon
{
	Q_OBJECT

public:
	MixerTrayIcon(Mixer *mixer, const QIcon &icon, QObject *parent = 0);
	
	void setMixer(Mixer *mixer);

	bool event(QEvent* ev)
	{
		if (ev->type() == QEvent::Wheel) {
			QWheelEvent *we = static_cast<QWheelEvent *>(ev);
			if (mixer == 0)
				return (true);
			int vol = mixer->getMasterVol();
			if (we->angleDelta().y() < 0) {
				if ((vol -= 3) < 0)
					vol = 0;
			} else if ((vol += 3) > 100)
				vol = 100;
			printf("vol == %d\n", vol);
			mixer->setVol(DSBMIXER_MASTER, vol);
			return (true);
		}
	        return QSystemTrayIcon::event(ev);
	}

private:
	Mixer *mixer;
};

