/*	$NetBSD: pcap-bt-monitor-linux.c,v 1.2 2014/11/19 19:33:30 christos Exp $	*/

/*
 * Copyright (c) 2014 Michal Labedzki for Tieto Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 * products derived from this software without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: pcap-bt-monitor-linux.c,v 1.2 2014/11/19 19:33:30 christos Exp $");

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/mgmt.h>

#include "pcap/bluetooth.h"
#include "pcap-int.h"

#define BT_CONTROL_SIZE 32
#define INTERFACE_NAME "bluetooth-monitor"

int
bt_monitor_findalldevs(pcap_if_t **alldevsp, char *err_str)
{
    int         ret = 0;

    if (pcap_add_if(alldevsp, INTERFACE_NAME, 0,
               "Bluetooth Linux Monitor", err_str) < 0)
    {
        ret = -1;
    }

    return ret;
}

static int
bt_monitor_read(pcap_t *handle, int max_packets _U_, pcap_handler callback, u_char *user)
{
    struct cmsghdr *cmsg;
    struct msghdr msg;
    struct iovec  iv[2];
    ssize_t ret;
    struct pcap_pkthdr pkth;
    pcap_bluetooth_linux_monitor_header *bthdr;
    struct mgmt_hdr hdr;
    int in = 0;

    bthdr = (pcap_bluetooth_linux_monitor_header*) &handle->buffer[handle->offset];

    iv[0].iov_base = &hdr;
    iv[0].iov_len = MGMT_HDR_SIZE;
    iv[1].iov_base = &handle->buffer[handle->offset + sizeof(pcap_bluetooth_linux_monitor_header)];
    iv[1].iov_len = handle->snapshot;

    memset(&pkth.ts, 0, sizeof(pkth.ts));
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = iv;
    msg.msg_iovlen = 2;
    msg.msg_control = handle->buffer;
    msg.msg_controllen = handle->offset;

    do {
        ret = recvmsg(handle->fd, &msg, 0);
        if (handle->break_loop)
        {
            handle->break_loop = 0;
            return -2;
        }
    } while ((ret == -1) && (errno == EINTR));

    if (ret < 0) {
        snprintf(handle->errbuf, PCAP_ERRBUF_SIZE,
            "Can't receive packet: %s", strerror(errno));
        return -1;
    }

    pkth.caplen = ret - MGMT_HDR_SIZE + sizeof(pcap_bluetooth_linux_monitor_header);
    pkth.len = pkth.caplen;

    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
        if (cmsg->cmsg_level != SOL_SOCKET) continue;

        if (cmsg->cmsg_type == SCM_TIMESTAMP) {
            memcpy(&pkth.ts, CMSG_DATA(cmsg), sizeof(pkth.ts));
        }
    }

    bthdr->adapter_id = htons(hdr.index);
    bthdr->opcode = htons(hdr.opcode);

    if (handle->fcode.bf_insns == NULL ||
        bpf_filter(handle->fcode.bf_insns, &handle->buffer[handle->offset],
          pkth.len, pkth.caplen)) {
        callback(user, &pkth, &handle->buffer[handle->offset]);
        return 1;
    }
    return 0;   /* didn't pass filter */
}

static int
bt_monitor_inject(pcap_t *handle, const void *buf _U_, size_t size _U_)
{
    snprintf(handle->errbuf, PCAP_ERRBUF_SIZE, "inject not supported yet");
    return -1;
}

static int
bt_monitor_setdirection(pcap_t *p, pcap_direction_t d)
{
    p->direction = d;
    return 0;
}

static int
bt_monitor_stats(pcap_t *handle _U_, struct pcap_stat *stats)
{
    stats->ps_recv = 0;
    stats->ps_drop = 0;
    stats->ps_ifdrop = 0;

    return 0;
}

static int
bt_monitor_activate(pcap_t* handle)
{
    struct sockaddr_hci addr;
    int err = PCAP_ERROR;
    int opt;

    if (handle->opt.rfmon) {
        /* monitor mode doesn't apply here */
        return PCAP_ERROR_RFMON_NOTSUP;
    }

    handle->bufsize = handle->snapshot + BT_CONTROL_SIZE + sizeof(pcap_bluetooth_linux_monitor_header);
    handle->offset = BT_CONTROL_SIZE;
    handle->linktype = DLT_BLUETOOTH_LINUX_MONITOR;

    handle->read_op = bt_monitor_read;
    handle->inject_op = bt_monitor_inject;
    handle->setfilter_op = install_bpf_program; /* no kernel filtering */
    handle->setdirection_op = bt_monitor_setdirection;
    handle->set_datalink_op = NULL; /* can't change data link type */
    handle->getnonblock_op = pcap_getnonblock_fd;
    handle->setnonblock_op = pcap_setnonblock_fd;
    handle->stats_op = bt_monitor_stats;

    handle->fd = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
    if (handle->fd < 0) {
        snprintf(handle->errbuf, PCAP_ERRBUF_SIZE,
            "Can't create raw socket: %s", strerror(errno));
        return PCAP_ERROR;
    }

    handle->buffer = malloc(handle->bufsize);
    if (!handle->buffer) {
        snprintf(handle->errbuf, PCAP_ERRBUF_SIZE, "Can't allocate dump buffer: %s",
            pcap_strerror(errno));
        goto close_fail;
    }

    /* Bind socket to the HCI device */
    addr.hci_family = AF_BLUETOOTH;
    addr.hci_dev = HCI_DEV_NONE;
    addr.hci_channel = HCI_CHANNEL_MONITOR;

    if (bind(handle->fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        snprintf(handle->errbuf, PCAP_ERRBUF_SIZE,
            "Can't attach to interface: %s", strerror(errno));
        goto close_fail;
    }

    opt = 1;
    if (setsockopt(handle->fd, SOL_SOCKET, SO_TIMESTAMP, &opt, sizeof(opt)) < 0) {
        snprintf(handle->errbuf, PCAP_ERRBUF_SIZE,
            "Can't enable time stamp: %s", strerror(errno));
        goto close_fail;
    }

    handle->selectable_fd = handle->fd;

    return 0;

close_fail:
    pcap_cleanup_live_common(handle);
    return err;
}

pcap_t *
bt_monitor_create(const char *device, char *ebuf, int *is_ours)
{
    pcap_t      *p;
    const char  *cp;

    cp = strrchr(device, '/');
    if (cp == NULL)
        cp = device;

    if (strcmp(cp, INTERFACE_NAME) != 0) {
        *is_ours = 0;
        return NULL;
    }

    *is_ours = 1;
    p = pcap_create_common(device, ebuf, 0);
    if (p == NULL)
        return NULL;

    p->activate_op = bt_monitor_activate;

    return p;
}
