include(defs.inc)
QT += widgets
TEMPLATE = subdirs
SUBDIRS += src/src.pro lib/backend/backend.pro
TRANSLATIONS = locale/$${PROGRAM}_de.ts

target.path  = $${PREFIX}/bin
target.files = $${PROGRAM}

dtfile.path  = $${APPSDIR} 
dtfile.files = $${PROGRAM}.desktop 

locales.path  = $${DATADIR}
locales.files = locale/$${PROGRAM}_de.qm

INSTALLS = target dtfile locales
