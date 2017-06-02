include(defs.inc)

QT += widgets
TEMPLATE  = subdirs
SUBDIRS	 += src lib/backend
LANGUAGES = de
INSTALLS  = target dtfile locales
QMAKE_EXTRA_TARGETS += distclean cleanqm

target.path  = $${PREFIX}/bin
target.files = $${PROGRAM}

dtfile.path  = $${APPSDIR} 
dtfile.files = $${PROGRAM}.desktop 

locales.path = $${DATADIR}

qtPrepareTool(LRELEASE, lrelease)
for(a, LANGUAGES) {
	in = locale/$${PROGRAM}_$${a}.ts
	out = locale/$${PROGRAM}_$${a}.qm
	locales.files += $$out
	cmd = $$LRELEASE $$in -qm $$out
	system($$cmd)
}
cleanqm.commands  = rm -f $${locales.files}
distclean.depends = cleanqm

