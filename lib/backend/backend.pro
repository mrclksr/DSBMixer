include(../../defs.inc)

TEMPLATE = app
TARGET	 = $${BACKEND}
SOURCES += dsbmixer_backend.c

isEmpty(PORTS) {
	QMAKE_INSTALL_PROGRAM = $${INSTALL_SETUID}
}

DEFINES += PATH_DSBWRTSYSCTL=\\\"$${PATH_DSBWRTSYSCTL}\\\"

target.path  = $${BACKEND_INSTALL_DIR}
target.files = $${BACKEND}
INSTALLS = target

