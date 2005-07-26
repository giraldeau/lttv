/* libltt
 *
 * Linux Trace Toolkit Netlink Control Library
 *
 * Controls the ltt-control kernel module through a netlink socket.
 *
 * Heavily inspired from libipq.c (iptables) made by 
 * James Morris <jmorris@intercode.com.au>
 *
 * Copyright 2005 -
 * 	Mathieu Desnoyers <mathieu.desnoyers@polymtl.ca>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 	
 */

#include <libltt/libltt.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <errno.h>
#include <stdio.h>

/* Private interface */

enum {
	LTTCTL_ERR_NONE = 0,
	LTTCTL_ERR_IMPL,
	LTTCTL_ERR_HANDLE,
	LTTCTL_ERR_SOCKET,
	LTTCTL_ERR_BIND,
	LTTCTL_ERR_BUFFER,
	LTTCTL_ERR_RECV,
	LTTCTL_ERR_NLEOF,
	LTTCTL_ERR_ADDRLEN,
	LTTCTL_ERR_STRUNC,
	LTTCTL_ERR_RTRUNC,
	LTTCTL_ERR_NLRECV,
	LTTCTL_ERR_SEND,
	LTTCTL_ERR_SUPP,
	LTTCTL_ERR_RECVBUF,
	LTTCTL_ERR_TIMEOUT,
        LTTCTL_ERR_PROTOCOL
};
#define LTTCTL_MAXERR LTTCTL_ERR_PROTOCOL


struct lttctl_errmap_t {
	int errcode;
	char *message;
} lttctl_errmap[] = {
	{ LTTCTL_ERR_NONE, "Unknown error" },
	{ LTTCTL_ERR_IMPL, "Implementation error" },
	{ LTTCTL_ERR_HANDLE, "Unable to create netlink handle" },
	{ LTTCTL_ERR_SOCKET, "Unable to create netlink socket" },
	{ LTTCTL_ERR_BIND, "Unable to bind netlink socket" },
	{ LTTCTL_ERR_BUFFER, "Unable to allocate buffer" },
	{ LTTCTL_ERR_RECV, "Failed to receive netlink message" },
	{ LTTCTL_ERR_NLEOF, "Received EOF on netlink socket" },
	{ LTTCTL_ERR_ADDRLEN, "Invalid peer address length" },
	{ LTTCTL_ERR_STRUNC, "Sent message truncated" },
	{ LTTCTL_ERR_RTRUNC, "Received message truncated" },
	{ LTTCTL_ERR_NLRECV, "Received error from netlink" },
	{ LTTCTL_ERR_SEND, "Failed to send netlink message" },
	{ LTTCTL_ERR_SUPP, "Operation not supported" },
	{ LTTCTL_ERR_RECVBUF, "Receive buffer size invalid" },
	{ LTTCTL_ERR_TIMEOUT, "Timeout"},
	{ LTTCTL_ERR_PROTOCOL, "Invalid protocol specified" }
};

static int lttctl_errno = LTTCTL_ERR_NONE;


static ssize_t lttctl_netlink_sendto(const struct lttctl_handle *h,
                                  const void *msg, size_t len);

static ssize_t lttctl_netlink_recvfrom(const struct lttctl_handle *h,
                                    unsigned char *buf, size_t len,
                                    int timeout);

static ssize_t lttctl_netlink_sendmsg(const struct lttctl_handle *h,
                                   const struct msghdr *msg,
                                   unsigned int flags);

static char *lttctl_strerror(int errcode);

void lttctl_perror(const char *s);

static ssize_t lttctl_netlink_sendto(const struct lttctl_handle *h,
                                  const void *msg, size_t len)
{
	int status = sendto(h->fd, msg, len, 0,
	                    (struct sockaddr *)&h->peer, sizeof(h->peer));
	if (status < 0)
		lttctl_errno = LTTCTL_ERR_SEND;
	
	return status;
}

static ssize_t lttctl_netlink_sendmsg(const struct lttctl_handle *h,
                                   const struct msghdr *msg,
                                   unsigned int flags)
{
	int status = sendmsg(h->fd, msg, flags);
	if (status < 0)
		lttctl_errno = LTTCTL_ERR_SEND;
	return status;
}

static ssize_t lttctl_netlink_recvfrom(const struct lttctl_handle *h,
                                    unsigned char *buf, size_t len,
                                    int timeout)
{
	int addrlen, status;
	struct nlmsghdr *nlh;

	if (len < sizeof(struct nlmsghdr)) {
		lttctl_errno = LTTCTL_ERR_RECVBUF;
		lttctl_perror("Netlink recvfrom");
		return -1;
	}
	addrlen = sizeof(h->peer);

	if (timeout != 0) {
		int ret;
		struct timeval tv;
		fd_set read_fds;
		
		if (timeout < 0) {
			/* non-block non-timeout */
			tv.tv_sec = 0;
			tv.tv_usec = 0;
		} else {
			tv.tv_sec = timeout / 1000000;
			tv.tv_usec = timeout % 1000000;
		}

		FD_ZERO(&read_fds);
		FD_SET(h->fd, &read_fds);
		ret = select(h->fd+1, &read_fds, NULL, NULL, &tv);
		if (ret < 0) {
			if (errno == EINTR) {
				printf("eintr\n");
				return 0;
			} else {
				lttctl_errno = LTTCTL_ERR_RECV;
				lttctl_perror("Netlink recvfrom");
				return -1;
			}
		}
		if (!FD_ISSET(h->fd, &read_fds)) {
			lttctl_errno = LTTCTL_ERR_TIMEOUT;
			printf("timeout\n");
			return 0;
		}
	}
	status = recvfrom(h->fd, buf, len, 0,
	                      (struct sockaddr *)&h->peer, &addrlen);
	
	if (status < 0) {
		lttctl_errno = LTTCTL_ERR_RECV;
		lttctl_perror("Netlink recvfrom");
		return status;
	}
	if (addrlen != sizeof(h->peer)) {
		lttctl_errno = LTTCTL_ERR_RECV;
		lttctl_perror("Netlink recvfrom");
		return -1;
	}
	if (h->peer.nl_pid != 0) {
		lttctl_errno = LTTCTL_ERR_RECV;
		lttctl_perror("Netlink recvfrom");
		return -1;
	}
	if (status == 0) {
		lttctl_errno = LTTCTL_ERR_NLEOF;
		lttctl_perror("Netlink recvfrom");
		return -1;
	}
	nlh = (struct nlmsghdr *)buf;
	if (nlh->nlmsg_flags & MSG_TRUNC || nlh->nlmsg_len > status) {
		lttctl_errno = LTTCTL_ERR_RTRUNC;
		lttctl_perror("Netlink recvfrom");
		return -1;
	}
	

	return status;
}


static char *lttctl_strerror(int errcode)
{
	if (errcode < 0 || errcode > LTTCTL_MAXERR)
		errcode = LTTCTL_ERR_IMPL;
	return lttctl_errmap[errcode].message;
}


char *lttctl_errstr(void)
{
	return lttctl_strerror(lttctl_errno);
}

void lttctl_perror(const char *s)
{
	if (s)
		fputs(s, stderr);
	else
		fputs("ERROR", stderr);
	if (lttctl_errno)
		fprintf(stderr, ": %s", lttctl_errstr());
	if (errno)
		fprintf(stderr, ": %s", strerror(errno));
	fputc('\n', stderr);
}

/* public interface */

/*
 * Create and initialise an lttctl handle.
 */
struct lttctl_handle *lttctl_create_handle(void)
{
	int status;
	struct lttctl_handle *h;

	h = (struct lttctl_handle *)malloc(sizeof(struct lttctl_handle));
	if (h == NULL) {
		lttctl_errno = LTTCTL_ERR_HANDLE;
		lttctl_perror("Create handle");
		goto alloc_error;
	}
	
	memset(h, 0, sizeof(struct lttctl_handle));
	
  h->fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_LTT);
        
	if (h->fd == -1) {
		lttctl_errno = LTTCTL_ERR_SOCKET;
		lttctl_perror("Create handle");
		goto socket_error;
	}
	memset(&h->local, 0, sizeof(struct sockaddr_nl));
	h->local.nl_family = AF_NETLINK;
	h->local.nl_pid = getpid();
	h->local.nl_groups = 0;
	status = bind(h->fd, (struct sockaddr *)&h->local, sizeof(h->local));
	if (status == -1) {
		lttctl_errno = LTTCTL_ERR_BIND;
		lttctl_perror("Create handle");
		goto bind_error;
	}
	memset(&h->peer, 0, sizeof(struct sockaddr_nl));
	h->peer.nl_family = AF_NETLINK;
	h->peer.nl_pid = 0;
	h->peer.nl_groups = 0;
	return h;
	
	/* Error condition */
bind_error:
socket_error:
		close(h->fd);
alloc_error:
		free(h);
	return NULL;
}

/*
 * No error condition is checked here at this stage, but it may happen
 * if/when reliable messaging is implemented.
 */
int lttctl_destroy_handle(struct lttctl_handle *h)
{
	if (h) {
		close(h->fd);
		free(h);
	}
	return 0;
}


int lttctl_create_trace(const struct lttctl_handle *h,
		char *name, enum trace_mode mode)
{
	int err;
	
	struct {
		struct nlmsghdr	nlh;
		lttctl_peer_msg_t	msg;
	} req;
	struct {
		struct nlmsghdr	nlh;
		struct nlmsgerr	nlerr;
		lttctl_peer_msg_t	msg;
	} ack;

	memset(&req, 0, sizeof(req));
	req.nlh.nlmsg_len = NLMSG_LENGTH(sizeof(lttctl_peer_msg_t));
	req.nlh.nlmsg_flags = NLM_F_REQUEST|NLM_F_ACK;
	req.nlh.nlmsg_type = LTTCTLM_CONTROL;
	req.nlh.nlmsg_pid = h->local.nl_pid;
	req.nlh.nlmsg_seq = 0;

	strncpy(req.msg.trace_name, name, NAME_MAX);
	req.msg.op = OP_CREATE;
	req.msg.args.mode = mode;

	err = lttctl_netlink_sendto(h, (void *)&req, req.nlh.nlmsg_len);
	if(err < 0) goto senderr;

	err = lttctl_netlink_recvfrom(h, (void*)&ack, sizeof(ack), 0);
	if(err < 0) goto senderr;

	err = ack.nlerr.error;
	if(err != 0) {
		errno = err;
		lttctl_perror("Create Trace Error");
		return -1;
	}

	return 0;

senderr:
	lttctl_perror("Create Trace Error");
	return err;
}

int lttctl_destroy_trace(const struct lttctl_handle *h,
		char *name)
{
	struct {
		struct nlmsghdr	nlh;
		lttctl_peer_msg_t	msg;
	} req;
	struct {
		struct nlmsghdr	nlh;
		struct nlmsgerr	nlerr;
		lttctl_peer_msg_t	msg;
	} ack;
	int err;

	memset(&req, 0, sizeof(req));
	req.nlh.nlmsg_len = NLMSG_LENGTH(sizeof(lttctl_peer_msg_t));
	req.nlh.nlmsg_flags = NLM_F_REQUEST;
	req.nlh.nlmsg_type = LTTCTLM_CONTROL;
	req.nlh.nlmsg_pid = h->local.nl_pid;

	strncpy(req.msg.trace_name, name, NAME_MAX);
	req.msg.op = OP_DESTROY;

	err = lttctl_netlink_sendto(h, (void *)&req, req.nlh.nlmsg_len);
	if(err < 0) goto senderr;

	err = lttctl_netlink_recvfrom(h, (void*)&ack, sizeof(ack), 0);
	if(err < 0) goto senderr;

	err = ack.nlerr.error;
	if(err != 0) {
		errno = err;
		lttctl_perror("Destroy Trace Channels Error");
		return -1;
	}

	return 0;

senderr:
	lttctl_perror("Destroy Trace Channels Error");
	return err;

}

int lttctl_start(const struct lttctl_handle *h,
		char *name)
{
	struct {
		struct nlmsghdr	nlh;
		lttctl_peer_msg_t	msg;
	} req;
	struct {
		struct nlmsghdr	nlh;
		struct nlmsgerr	nlerr;
		lttctl_peer_msg_t	msg;
	} ack;

	int err;

	memset(&req, 0, sizeof(req));
	req.nlh.nlmsg_len = NLMSG_LENGTH(sizeof(lttctl_peer_msg_t));
	req.nlh.nlmsg_flags = NLM_F_REQUEST;
	req.nlh.nlmsg_type = LTTCTLM_CONTROL;
	req.nlh.nlmsg_pid = h->local.nl_pid;

	strncpy(req.msg.trace_name, name, NAME_MAX);
	req.msg.op = OP_START;

	err = lttctl_netlink_sendto(h, (void *)&req, req.nlh.nlmsg_len);
	if(err < 0) goto senderr;

	err = lttctl_netlink_recvfrom(h, (void*)&ack, sizeof(ack), 0);
	if(err < 0) goto senderr;

	err = ack.nlerr.error;
	if(err != 0) {
		errno = err;
		lttctl_perror("Start Trace Error");
		return -1;
	}

	return 0;

senderr:
	lttctl_perror("Start Trace Error");
	return err;

}

int lttctl_stop(const struct lttctl_handle *h,
		char *name)
{
	struct {
		struct nlmsghdr	nlh;
		lttctl_peer_msg_t	msg;
	} req;
	struct {
		struct nlmsghdr	nlh;
		struct nlmsgerr	nlerr;
		lttctl_peer_msg_t	msg;
	} ack;
	int err;

	memset(&req, 0, sizeof(req));
	req.nlh.nlmsg_len = NLMSG_LENGTH(sizeof(lttctl_peer_msg_t));
	req.nlh.nlmsg_flags = NLM_F_REQUEST;
	req.nlh.nlmsg_type = LTTCTLM_CONTROL;
	req.nlh.nlmsg_pid = h->local.nl_pid;

	strncpy(req.msg.trace_name, name, NAME_MAX);
	req.msg.op = OP_STOP;

	err = lttctl_netlink_sendto(h, (void *)&req, req.nlh.nlmsg_len);
	if(err < 0) goto senderr;

	err = lttctl_netlink_recvfrom(h, (void*)&ack, sizeof(ack), 0);
	if(err < 0) goto senderr;

	err = ack.nlerr.error;
	if(err != 0) {
		errno = err;
		lttctl_perror("Stop Trace Error");
		return -1;
	}

	return 0;

senderr:
	lttctl_perror("Stop Trace Error");
	return err;
}
