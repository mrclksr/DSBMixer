include(defs.inc)

TEMPLATE     = subdirs
SUBDIRS	    += src lib/backend
TRANSLATIONS = locale/$${PROGRAM}_de.ts \
               locale/$${PROGRAM}_fr.ts
INSTALLS     = target dtfile locales scripts
QMAKE_EXTRA_TARGETS += distclean cleanqm readme readmemd

target.path  = $${PREFIX}/bin
target.files = $${PROGRAM}

dtfile.path  = $${APPSDIR} 
dtfile.files = $${PROGRAM}.desktop 

readme.target = readme
readme.files = readme.mdoc
readme.commands = mandoc -mdoc readme.mdoc | perl -e \'foreach (<STDIN>) { \
		\$$_ =~ s/(.)\x08\1/\$$1/g; \$$_ =~ s/_\x08(.)/\$$1/g; \
		print \$$_ \
	}\' | sed '1,1d' > README

readmemd.target = readmemd
readmemd.files = readme.mdoc
readmemd.commands = mandoc -mdoc -Tmarkdown readme.mdoc | \
			sed -e \'1,1d; \$$,\$$d\' > README.md

scripts.files += scripts/$${RESTART_PA}
scripts.path   = $${SCRIPTS_DIR}
scripts.CONFIG = nostrip

locales.path = $${DATADIR}

qtPrepareTool(LRELEASE, lrelease)
for(a, TRANSLATIONS) {
	cmd = $$LRELEASE $${a}
	system($$cmd)
}
locales.files += locale/*.qm
cleanqm.commands  = rm -f $${locales.files}
distclean.depends = cleanqm

