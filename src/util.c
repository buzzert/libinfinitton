/*
 * util.c
 *
 * Created 2019-02-13 by James Magahern <james@magahern.com>
 */

#include "../include/util.h"

#include <stdlib.h>
#include <string.h>

bool util_debugging_enabled ()
{
    static enum { UNK, TRUE, FALSE } enabled = UNK;
    if (enabled == UNK) {
        char *val = getenv ("DEBUG");
        if (strcmp (val, "1") == 0) {
            enabled = TRUE;
        } else {
            enabled = FALSE;
        }
    }

    return (enabled == TRUE);
}


