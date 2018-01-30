/**
   @file startup.c

   This file implements a policy that is used to load
   all other policies and do startup tasks for DSME.
   <p>
   Copyright (C) 2004-2009 Nokia Corporation.

   @author Ari Saastamoinen
   @author Ismo Laitinen <ismo.laitinen@nokia.com>
   @author Semi Malinen <semi.malinen@nokia.com>

   This file is part of Dsme.

   Dsme is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License
   version 2.1 as published by the Free Software Foundation.

   Dsme is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with Dsme.  If not, see <http://www.gnu.org/licenses/>.
*/


/**
 * @defgroup modules DSME Modules
 */

/**
 * @defgroup startup Startup
 * @ingroup modules
 *
 * Startup module loads other modules on DSME startup. 
 */

#include "dsme/messages.h"
#include "dsme/modulebase.h"
#include "dsme/logging.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>

#define STRINGIFY(x)  STRINGIFY2(x)
#define STRINGIFY2(x) #x

/**
 * @ingroup startup
 * Configuration file that has the list of modules that are loaded on startup.
 * DSME tries to load the modules from the same directory where startup-module
 * was loaded.
 */
#define MODULES_CONF "/etc/dsme/modules.conf"


/**
 * @ingroup startup
 * This array defines which modules are started on startup in case
 * /etc/dsme/modules.conf is not readable.
 */
const char *modules[] = {
    "libheartbeat.so",
    "librunlevel.so",
    "libstate.so",
    "liblifeguard.so",
    "libprocesswd.so",
    "libdbusproxy.so",
    "libalarmtracker.so",
    "libthermalmanager.so",
#ifdef DSME_MEMORY_THERMAL_MGMT
    "libthermalobject_memory.so",
#endif
#ifdef DSME_BMEIPC
    "libthermalobject_surface.so",
#endif
    "libemergencycalltracker.so",
    NULL
};


DSME_HANDLER(DSM_MSGTYPE_GET_VERSION, client, ind)
{
	static const char*       version = STRINGIFY(PRG_VERSION);
	DSM_MSGTYPE_DSME_VERSION msg     =
          DSME_MSG_INIT(DSM_MSGTYPE_DSME_VERSION);

        dsme_log(LOG_DEBUG, "version requested, sending '%s'", version);
	endpoint_send_with_extra(client, &msg, strlen(version) + 1, version);
}


module_fn_info_t message_handlers[] = {
  DSME_HANDLER_BINDING(DSM_MSGTYPE_GET_VERSION),
  {0}
};

void module_init(module_t *handle)
{
    dsme_log(LOG_CRIT, "DSME %s starting up", STRINGIFY(PRG_VERSION));

    char * modulename;
    char * path;
    char name[1024];  /* TODO more dynamic length */
    const char **names;
	FILE *conffile = fopen(MODULES_CONF, "r");

        modulename = strdup(module_name(handle));
	if (!modulename) {
		dsme_log(LOG_CRIT, "strdup failed");
		exit(EXIT_FAILURE);
	}

    path = dirname(modulename);

	if (!conffile) {
		dsme_log(LOG_DEBUG, "Unable to read conffile (%s), using compiled-in startup list", MODULES_CONF);
		for (names = modules ; *names ; names++) {
			snprintf(name, sizeof(name), "%s/%s", path, *names);
			if(load_module(name, 0) == NULL) {
				dsme_log(LOG_ERR, "error loading module %s", name);
				/* TODO: Change to MALF?? */
			}
		}
	} else {
		size_t len = 0;
		char * line = NULL;
		
		dsme_log(LOG_DEBUG, "Conf file exists, reading modulenames from %s", MODULES_CONF);

		while (getline(&line, &len, conffile) > 0) { 
			snprintf(name, sizeof(name), "%s/%s", path, line);
			name[strlen(name) - 1] = '\0'; /* Remove newline */
			if (load_module(name, 0) == NULL) {
				dsme_log(LOG_ERR, "error loading module %s", name);
				/* TODO: Change to MALF?? */
			}
		} 
		if (line)
			free(line);
		fclose(conffile);
	}

	free(modulename);
    dsme_log(LOG_INFO, "Module loading finished.");
}
