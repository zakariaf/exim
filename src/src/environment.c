/*************************************************
*     Exim - an Internet mail transport agent    *
*************************************************/

/* Copyright (c) Heiko Schlittermann 2016
 * hs@schlittermann.de
 * See the file NOTICE for conditions of use and distribution.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "exim.h"

extern char **environ;

/* The cleanup_environment() function is used during the startup phase
of the Exim process, right after reading the configurations main
part, before any expansions take place. It retains the environment
variables we trust (via the keep_environment option) and allows to
set additional variables (via add_environment).

Returns:    TRUE if successful
            FALSE otherwise
*/

BOOL
cleanup_environment()
{
int old_pool = store_pool;
store_pool = POOL_PERM;		/* Need perm memory for any created env vars */

if (!keep_environment || *keep_environment == '\0')
  {
  /* From: https://github.com/dovecot/core/blob/master/src/lib/env-util.c#L55
  Try to clear the environment.
  a) environ = NULL crashes on OS X.
  b) *environ = NULL doesn't work on FreeBSD 7.0.
  c) environ = emptyenv doesn't work on Haiku OS
  d) environ = calloc() should work everywhere */

  if (environ) *environ = NULL;

  }
else if (Ustrcmp(keep_environment, "*") != 0)
  {
  rmark reset_point = store_mark();
  if (environ) for (uschar ** p = USS environ; *p; /* see below */)
    {
    /* It's considered broken if we do not find the '=', according to
    Florian Weimer. For now we ignore such strings. unsetenv() would complain,
    getenv() would complain. */
    uschar * eqp = Ustrchr(*p, '=');

    if (eqp)
      {
      uschar * name = string_copyn(*p, eqp - *p);

      if (OK != match_isinlist(name, CUSS &keep_environment,
          0, NULL, NULL, MCL_NOEXPAND, FALSE, NULL))
        if (os_unsetenv(name) < 0) return FALSE;
        else p = USS environ; /* RESTART from the beginning */
      else p++;
      }
    }
  store_reset(reset_point);
  }
if (add_environment)
  {
  uschar * p;
  int sep = 0;
  const uschar * envlist = add_environment;

  while ((p = string_nextinlist(&envlist, &sep, NULL, 0)))
    {
    DEBUG(D_expand) debug_printf("adding %s\n", p);
    putenv(CS p);
    }
  }
#ifndef DISABLE_TLS
tls_clean_env();
#endif

store_pool = old_pool;
return TRUE;
}
