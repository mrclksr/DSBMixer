/*-
 * Copyright (c) 2016 Marcel Kaiser. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>

#define PATH_SYSCTL		"/sbin/sysctl"

#define OID_AMPLIFY		"hw.snd.vpc_0db"
#define OID_DEFAULT_UNIT	"hw.snd.default_unit"
#define OID_FEEDER_RATE_QUALITY "hw.snd.feeder_rate_quality"

enum VAR_TYPE { TYPE_INT, TYPE_STRING };

void
execmd(const char *cmd)
{
	int ret;

	if ((ret = system(cmd)) == -1) {
		err(EXIT_FAILURE, "system(%s)", cmd);
	} else if (ret == 127) {
		err(EXIT_FAILURE, "Execution of shell failed");
	} else if (ret != 0) {
		errx(EXIT_FAILURE, "Command '%s' exited with status %d", cmd,
		    ret);
	}
}

void
setoid(const char *oid, const char *val, int type)
{
	char cmd[512];

	if (type == TYPE_INT) {
		(void)snprintf(cmd, sizeof(cmd), "%s %s=%d",
		    PATH_SYSCTL, oid, (int)strtol(val, NULL, 10));
		execmd(cmd);
		(void)snprintf(cmd, sizeof(cmd), "%s %s=%d",
		    PATH_DSBWRTSYSCTL, oid, (int)strtol(val, NULL, 10));
		execmd(cmd);
	} else {
		(void)snprintf(cmd, sizeof(cmd), "%s %s=%s",
		    PATH_SYSCTL, oid, val);
		execmd(cmd);
		(void)snprintf(cmd, sizeof(cmd), "%s %s=%s",
		    PATH_DSBWRTSYSCTL, oid, val);
		execmd(cmd);
	}
}

void
usage()
{
	(void)fprintf(stderr, "Usage: dsbmixer_backend [-u unit -d] " \
	    "[-a amplify] [-q quality]\n");
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	int   ch, aflag, dflag, qflag, uflag;
	char *unit, *vol, *quality;

	aflag = dflag = qflag = uflag = 0;
	while ((ch = getopt(argc, argv, "a:u:q:d")) != -1) {
		switch (ch) {
		case 'u':
			uflag++; unit = optarg;
			break;
		case 'd':
			dflag++;
			break;
		case 'a':
			aflag++; vol = optarg;
			break;
		case 'q':
			qflag++; quality = optarg;
			break;
		case '?':
		default:
			usage();
		}
	}
	if (dflag && !uflag)
		usage();
	if (dflag)
		setoid(OID_DEFAULT_UNIT, unit, TYPE_INT);
	if (qflag)
		setoid(OID_FEEDER_RATE_QUALITY, quality, TYPE_INT);
	if (aflag)
		setoid(OID_AMPLIFY, vol, TYPE_INT);
	return (EXIT_SUCCESS);
}

