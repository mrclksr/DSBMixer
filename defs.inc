PROGRAM    = dsbmixer
BACKEND    = $${PROGRAM}_backend
RESTART_PA = $${PROGRAM}_restart_pa

isEmpty(PREFIX) {
	PREFIX=/usr/local
}

isEmpty(DATADIR) {
	DATADIR=$${PREFIX}/share/$${PROGRAM}
}

SCRIPTS_DIR	    = $${PREFIX}/libexec
BACKEND_INSTALL_DIR = $${PREFIX}/libexec
APPSDIR		    = $${PREFIX}/share/applications
PATH_BACKEND	    = $${BACKEND_INSTALL_DIR}/$${BACKEND}
PATH_DSBWRTSYSCTL   = $${PREFIX}/bin/dsbwrtsysctl
PATH_RESTART_PA	    = $${SCRIPTS_DIR}/$${RESTART_PA}
INSTALL_SETUID	    = install -s -g wheel -m 4750 -o root

DEFINES += PATH_BACKEND=\\\"$${PATH_BACKEND}\\\"
DEFINES += PROGRAM=\\\"$${PROGRAM}\\\"
DEFINES += LOCALE_PATH=\\\"$${DATADIR}\\\"
DEFINES += PATH_RESTART_PA=\\\"$${PATH_RESTART_PA}\\\"

