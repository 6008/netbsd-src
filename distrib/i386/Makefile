#	$NetBSD: Makefile,v 1.10 2013/07/16 02:10:43 khorben Exp $

SUBDIR=
SUBDIR+=	ramdisks
SUBDIR+=	.WAIT
SUBDIR+=	instkernel
SUBDIR+=	kmod
SUBDIR+=	kmod-cgdroot
SUBDIR+=	.WAIT
SUBDIR+=	cdroms
SUBDIR+=	floppies
SUBDIR+=	liveimage
SUBDIR+=	installimage
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

live_image:
	${MAKEDIRTARGET} liveimage live_image

install_image:
	${MAKEDIRTARGET} installimage install_image

.include <bsd.subdir.mk>
