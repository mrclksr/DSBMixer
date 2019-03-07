include(defs.inc)

QT += widgets
TEMPLATE  = subdirs
SUBDIRS	 += src lib/backend
LANGUAGES = de
INSTALLS  = target dtfile locales
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

