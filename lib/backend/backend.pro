include(../../defs.inc)

TEMPLATE = app
TARGET	 = $${BACKEND}
SOURCES += dsbmixer_backend.c

isEmpty(PORTS) {
	target.path	= $${BACKEND_INSTALL_DIR}
	target.commands = $${INSTALL_SETUID} $${BACKEND} $${BACKEND_INSTALL_DIR}
} else {
	target.path	= $${BACKEND_INSTALL_DIR}
	target.files	= $${BACKEND}
}
INSTALLS = target
DEFINES += PATH_DSBWRTSYSCTL=\\\"$${PATH_DSBWRTSYSCTL}\\\"
QMAKE_POST_LINK=$(STRIP) $(TARGET)

