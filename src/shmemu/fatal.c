/* For license: see LICENSE file at top-level */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void
shmemu_fatal(const char *fmt, ...)
{
        va_list ap;

        va_start(ap, fmt);
        {
            fprintf(stderr, "FATAL: ");
            vfprintf(stderr, fmt, ap);
            fprintf(stderr, "\n");
            fflush(stderr);
        }
        va_end(ap);

        exit(EXIT_FAILURE);
}