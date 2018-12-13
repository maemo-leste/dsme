/**
   @file dsme-wd-wdd.c

   This file implements hardware watchdog kicker.
   <p>
   Copyright (C) 2004-2010 Nokia Corporation.

   @author Igor Stoppa <igor.stopaa@nokia.com>
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

#include "dsme-wdd-wd.h"
#include "dsme-wdd.h"

#include <cal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sched.h>
#include <linux/types.h>
#include <linux/watchdog.h>


#define DSME_STATIC_STRLEN(s) (sizeof(s) - 1)


typedef struct wd_t {
    int         period; /* watchdog timeout (s); 0 for keeping the default */
    const char* flag;   /* R&D flag in cal that disables the watchdog */
} wd_t;

/* the table of HW watchdogs; notice that their order matters! */
#define SHORTEST     14
#define MAX_WD_COUNT 10

static int shortest_timeout = SHORTEST;

static const wd_t wd[MAX_WD_COUNT] = {
    /* 10 entries: /dev/watchdog[0..9] */

    /* timeout (s), disabling R&D flag */
    {  SHORTEST,    "no-omap-wd" }, /* omap wd      */
    {  30,          "no-ext-wd"  }, /* twl (ext) wd */
    {  30,          "no-ext-wd"  },
    {  30,          "no-ext-wd"  },
    {  30,          "no-ext-wd"  },
    {  30,          "no-ext-wd"  },
    {  30,          "no-ext-wd"  },
    {  30,          "no-ext-wd"  },
    {  30,          "no-ext-wd"  },
    {  30,          "no-ext-wd"  },
};

#define WD_COUNT (sizeof(wd) / sizeof(wd[0]))

/* watchdog file descriptors */
static int wd_fd[WD_COUNT];

int dsme_wd_get_heartbeat_interval(void)
{
  // We take a 2 second window for kicking the watchdogs.
  return shortest_timeout - 2;
}

bool dsme_wd_is_wd_fd(int fd)
{
  int i;

  for (i = 0; i < WD_COUNT; ++i) {
      if (wd_fd[i] != -1 && fd == wd_fd[i])
          return true;
  }

  return false;
}

void dsme_wd_close(void) {
  int i;

  fprintf(stderr, "dsme_wd_close\n");

  for (i = 0; i < WD_COUNT; ++i) {
      if (wd_fd[i] != -1) {
          fprintf(stderr, "Closing watchdog\n");
          while ((write(wd_fd[i], "V", 1) == -1) && errno == EAGAIN) {
              fprintf(stderr, "Unable to write magic value to watchdog\n");
          }
          fprintf(stderr, "Wrote magic value; closing\n");
          close(wd_fd[i]);
      }
  }
}

void dsme_wd_kick(void)
{
  int i;
  int dummy;

  for (i = 0; i < WD_COUNT; ++i) {
      if (wd_fd[i] != -1) {
          int bytes_written;
          while ((bytes_written = write(wd_fd[i], "*", 1)) == -1 &&
                 errno == EAGAIN)
          {
              const char msg[] = "Got EAGAIN when kicking WD ";
              char c = '0' + i;

              dummy = write(STDERR_FILENO, msg, DSME_STATIC_STRLEN(msg));
              dummy = write(STDERR_FILENO, &c, 1);
              dummy = write(STDERR_FILENO, "\n", 1);
          }
          if (bytes_written != 1) {
              const char msg[] = "Error kicking WD ";
              char c = '0' + i;

              dummy = write(STDERR_FILENO, msg, DSME_STATIC_STRLEN(msg));
              dummy = write(STDERR_FILENO, &c, 1);
              dummy = write(STDERR_FILENO, "\n", 1);

              /* must not kick later wd's if an earlier one fails */
              break;
          }
      }
  }

#if 0 /* for debugging only */
  static struct timespec previous_timestamp = { 0, 0 };
  struct timespec timestamp;

  if (clock_gettime(CLOCK_MONOTONIC, &timestamp) != -1) {
      if (previous_timestamp.tv_sec != 0) {
          long ms;

          ms = (timestamp.tv_sec - previous_timestamp.tv_sec) * 1000;
          ms += (timestamp.tv_nsec - previous_timestamp.tv_nsec) / 1000000;

          if (ms > DSME_HEARTBEAT_INTERVAL * 1000 + 100) {
              fprintf(stderr, ME "took %ld ms between WD kicks\n", ms);
          }
      }
      previous_timestamp = timestamp;
  }
#endif
}

static void check_for_cal_wd_flags(bool wd_enabled[])
{
    void*         vptr = NULL;
    unsigned long len  = 0;
    int           ret  = 0;
    char*         p;
    int           i;

    /* see if there are any R&D flags to disable any watchdogs */
    ret = cal_read_block(0, "r&d_mode", &vptr, &len, CAL_FLAG_USER);
    if (ret < 0) {
        fprintf(stderr,
                ME "Error reading R&D mode flags, WD kicking enabled\n");
        return;
    }
    p = vptr;
    if (len >= 1 && *p) {
        fprintf(stderr, ME "R&D mode enabled\n");

        if (len > 1) {
            for (i = 0; i < WD_COUNT; ++i) {
                if (strstr(p, wd[i].flag)) {
                    wd_enabled[i] = false;
                    fprintf(stderr, ME "WD kicking disabled for watchdog: %d\n", i);
                }
            }
        } else {
            fprintf(stderr, ME "No WD flags found, WD kicking enabled\n");
        }
    }

    free(vptr);
    return;
}

bool dsme_wd_init(void)
{
    int  opened_wd_count = 0;
    bool wd_enabled[WD_COUNT];
    int  i;

    /* we don't support more, all temp buffers are accounted for that value */
    if (WD_COUNT > MAX_WD_COUNT)
    {
        fprintf(stderr,
                ME "WD count %zu > maximum supported %d\n",
                WD_COUNT,
                MAX_WD_COUNT);
        return false;
    }

    for (i = 0; i < WD_COUNT; ++i) {
        wd_enabled[i] = true; /* enable all watchdogs by default */
        wd_fd[i]      = -1;
    }

    /* disable the watchdogs that have a disabling R&D flag */
    check_for_cal_wd_flags(wd_enabled);
    char watchdog_path[15];

    /* open enabled watchdog devices */
    for (i = 0; i < WD_COUNT; ++i) {
        if (wd_enabled[i]) {
            snprintf(watchdog_path, sizeof(watchdog_path),
                     "/dev/watchdog%d", i);
            wd_fd[i] = open(watchdog_path, O_RDWR);

            if (wd_fd[i] == -1) {
                fprintf(stderr,
                        ME "Error opening WD %s: %s\n",
                        watchdog_path,
                        strerror(errno));
            } else {
                ++opened_wd_count;

                if (wd[i].period != 0) {
                    fprintf(stderr,
                             ME "Setting WD period to %d s for %s\n",
                             wd[i].period,
                             watchdog_path);

                    /* set the wd period */
                    /* ioctl() will overwrite tmp with the actual timeout set */
                    int tmp = wd[i].period;
                    int ret;
                    ret = ioctl(wd_fd[i], WDIOC_SETTIMEOUT, &tmp);

                    if (ret == 0) {
                        if (tmp < shortest_timeout) {
                            fprintf(stderr,
                                    ME "Warning: returned timeout is shorter than %d, setting shortest_timeout to %d",
                                    shortest_timeout, tmp);
                            shortest_timeout = tmp;
                          }
                    } else {
                        fprintf(stderr,
                                 ME "Error setting WD period for %s\n",
                                 watchdog_path);
                    }
                } else {
                    fprintf(stderr,
                             ME "Keeping default WD period for %s\n",
                             watchdog_path);
                }
            }
        }
    }

    return (opened_wd_count != 0);
}
