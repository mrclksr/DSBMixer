PROGRAM		  = dsbmixer
BACKEND		  = ${PROGRAM}_backend
PREFIX		 ?= /usr/local
LOCALBASE	 ?= ${PREFIX}
PATH_BACKEND	  = ${PREFIX}/libexec/${BACKEND}
PROGRAM_SOURCES	  = ${PROGRAM}.c gtk-helper/gtk-helper.c dsbcfg/dsbcfg.c
BACKEND_SOURCES	  = ${BACKEND}.c
TARGETS		  = ${PROGRAM} ${BACKEND}
CFLAGS		 += -Wall
PROGRAM_FLAGS	  = ${CFLAGS} ${CPPFLAGS} `pkg-config gtk+-2.0 --cflags --libs`
PROGRAM_FLAGS	 += -DPROGRAM=\"${PROGRAM}\"
PROGRAM_FLAGS	 += -DPATH_LOCALE=\"${PREFIX}/share/locale\"
PROGRAM_FLAGS	 += -DPATH_BACKEND=\"${PATH_BACKEND}\"
PROGRAM_LIBS	  = -ldevinfo
PATH_DSBWRTSYSCTL = ${LOCALBASE}/bin/dsbwrtsysctl
BACKEND_FLAGS	  = ${CFLAGS} ${CPPFLAGS}
BACKEND_FLAGS	 += -DPATH_DSBWRTSYSCTL=\"${PATH_DSBWRTSYSCTL}\"
INSTALL_DATA	  = install -m 0644
INSTALL_BACKEND	  = install -s -g wheel -m 4750 -o root
INSTALL_PROGRAM	  = install -s -m 0755

.if !defined(WITHOUT_GETTEXT)
PROGRAM_FLAGS	+= -DWITH_GETTEXT
TARGETS += nls
.endif

.if !defined(WITHOUT_DEVD)
PROGRAM_FLAGS	+= -DWITH_DEVD
.endif

all: ${TARGETS}

${PROGRAM}: ${PROGRAM_SOURCES}
	${CC} -o ${PROGRAM} ${PROGRAM_FLAGS} ${PROGRAM_SOURCES} ${LIBS}

${BACKEND}: ${BACKEND_SOURCES}
	${CC} -o ${BACKEND} ${BACKEND_FLAGS} ${BACKEND_SOURCES}

nls:
	for i in locale/*.po; do \
		msgfmt -c -v -o $${i%po}mo $$i; \
	done

install: ${TARGETS}
	${INSTALL_PROGRAM} ${PROGRAM} ${DESTDIR}${PREFIX}/bin
	${INSTALL_BACKEND} ${BACKEND} ${DESTDIR}${PREFIX}/libexec
	${INSTALL_DATA} ${PROGRAM}.desktop \
		${DESTDIR}${PREFIX}/share/applications

.if !defined(WITHOUT_GETTEXT)
	(cd locale && for i in *.mo; do \
		${INSTALL_DATA} $$i \
		${DESTDIR}${PREFIX}/share/locale/$${i%.mo}/LC_MESSAGES/${PROGRAM}.mo; \
	done)
.endif

clean:
	-rm -f ${PROGRAM} ${BACKEND}
	-rm -f locale/*.mo

