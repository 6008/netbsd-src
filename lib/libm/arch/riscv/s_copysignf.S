/* $NetBSD: s_copysignf.S,v 1.1 2014/09/19 17:36:25 matt Exp $ */

/* The contents of this file are in the public domain. */

#include <machine/asm.h>

ENTRY(copysignf)
	fsgnj.s	fv0, fa0, fa1
	ret
END(copysignf)
