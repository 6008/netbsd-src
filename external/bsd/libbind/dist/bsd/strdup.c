/*	$NetBSD: strdup.c,v 1.1.1.1 2009/04/12 15:33:26 christos Exp $	*/

#include "port_before.h"

#include <stdlib.h>

#include "port_after.h"

#ifndef NEED_STRDUP
int __bind_strdup_unneeded;
#else
char *
strdup(const char *src) {
	char *dst = malloc(strlen(src) + 1);

	if (dst)
		strcpy(dst, src);
	return (dst);
}
#endif

/*! \file */
