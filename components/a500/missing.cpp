 /*
  * UAE - The Un*x Amiga Emulator
  *
  * Various stuff missing in some OSes.
  *
  * Copyright 1997 Bernd Schmidt
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "uae.h"

#ifndef HAVE_STRDUP

char *my_strdup (const char *s)
{
    /* The casts to char * are there to shut up the compiler on HPUX */
    char *x = (char*)xmalloc(strlen((char *)s) + 1);
    strcpy(x, (char *)s);
    return x;
}

#endif


#include "esp_heap_caps.h"

void *xmalloc (size_t n)
{
    void *a = heap_caps_malloc(n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!a) a = malloc(n);
    if (a == NULL) {
	write_log ("virtual memory exhausted (%u bytes)\n", (unsigned)n);
	return NULL;
    }
    return a;
}

void *xcalloc (size_t n, size_t size)
{
    void *a = heap_caps_calloc(n, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!a) a = calloc(n, size);
    if (a == NULL) {
	write_log ("virtual memory exhausted (%u bytes)\n", (unsigned)(n * size));
	return NULL;
    }
    return a;
}
