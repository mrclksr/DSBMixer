
# ABOUT

**DSBMixer**
is a tabbed Qt (GTK+) mixer for FreeBSD&#174;. For each installed mixer
device,
**DSBMixer**
opens a tab. It adds a new mixer tab at runtime when you
plug in an USB sound device.
**DSBMixer**
allows you set amplification, to select recording sources, and to choose
your default audio device.

# INSTALLATION

## Dependencies

**DSBMixer**
depends on
*devel/qt5-buildtools*, *devel/qt5-core*, *devel/qt5-linguisttools*,
*devel/qt5-qmake*, *x11-toolkits/qt5-gui*,
and
*x11-toolkits/qt5-widgets*

In order to save settings,
**DSBMixer**
requires
*sysutils/dsbwrtsysctl*

## Getting the source code

	# git clone https://github.com/mrclksr/DSBMixer.git
	# git clone https://github.com/mrclksr/dsbcfg.git

## Building and installation

\# cd DSBMixer && qmake

\# make && make install

or if you don't want devd support:

	# make -DWITHOUT_DEVD && make install

# USAGE

	Usage: dsbmixer [-hi]
	   -i: Start dsbmixer as tray icon

