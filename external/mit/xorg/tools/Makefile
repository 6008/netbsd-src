#	$NetBSD: Makefile,v 1.5 2015/01/07 20:42:01 joerg Exp $

.include <bsd.own.mk>

# These are used by the 'includes' phase,
# and can't assume that ${DESTDIR}${X11INCDIR} has been populated.

SUBDIR+= makestrs .WAIT

# The rest of the tools can rely upon 'includes'

# XXX pswrap/gen_matypes -- missing

SUBDIR+=bdftopcf fc-cache ucs2any
SUBDIR+=makekeys mkfontscale .WAIT xkbcomp

.include <bsd.buildinstall.mk>
.include <bsd.subdir.mk>
