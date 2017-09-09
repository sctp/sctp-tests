/* Linux SCTP SENDER_DRY bug demonstration program */

/* Written by Harald Welte <laorge@gnumonks.org>, dedicated to the
 * public domain using CC-0 license, for more details see
 * https://creativecommons.org/publicdomain/zero/1.0/ */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/ioctl.h>

#include <netinet/in.h>
#include <netinet/sctp.h>

static int make_nonblock(int fd)
{
	int rc, on = 1;
	rc = ioctl(fd, FIONBIO, (unsigned char *)&on);
	if (rc < 0) {
		perror("ioctl(FIONBIO)");
		return rc;
	}
	return 0;
}

static int make_nodelay(int fd)
{
	int rc, on = 1;

	rc = setsockopt(fd, IPPROTO_SCTP, SCTP_NODELAY, &on, sizeof(on));
	if (rc < 0) {
		perror("setsockoopt(NODELAY)");
		return rc;
	}
	return 0;
}

static int enable_events(int fd)
{
	struct sctp_event_subscribe event;
	int rc;

	memset((uint8_t *)&event, 0, sizeof(event));

	/***********************************************************************
	 * This is the Key part of this demonstrator
	 ***********************************************************************/
	event.sctp_sender_dry_event = 1;
	event.sctp_data_io_event = 1;

	rc = setsockopt(fd, IPPROTO_SCTP, SCTP_EVENTS, &event, sizeof(event));
	if (rc < 0) {
		perror("setsockopt(EVENTS)");
		return rc;
	}
	return 0;
}

static int create_sctp_socket(void)
{
	int rc, fd;

	rc = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	if (rc < 0) {
		perror("socket");
		return rc;
	}
	fd = rc;

	rc = make_nodelay(fd);
	if (rc < 0)
		return rc;

	return fd;
}

static int create_sctp_socket_server(uint16_t port)
{
	int rc, fd;
	struct sockaddr_in sin = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = INADDR_ANY,
		.sin_port = htons(port),
	};

	rc = create_sctp_socket();
	if (rc < 0)
		return rc;
	fd = rc;

	rc = bind(fd, (struct sockaddr *)&sin, sizeof(sin));
	if (rc < 0) {
		perror("bind");
		return rc;
	}

	rc = listen(fd, 100);
	if (rc < 0) {
		perror("listen");
		return rc;
	}

	return fd;
}

static int create_sctp_socket_client(uint32_t srv_ip, uint16_t port)
{
	int rc, fd;
	struct sockaddr_in sin = {
		.sin_family = AF_INET,
	};

	rc = create_sctp_socket();
	if (rc < 0)
		return rc;
	fd = rc;

	sin.sin_addr.s_addr = htonl(0x7F000002);
	sin.sin_port = 3333;
	rc = bind(fd, (struct sockaddr *)&sin, sizeof(sin));
	if (rc < 0) {
		perror("bind");
		return rc;
	}

	sin.sin_addr.s_addr = htonl(srv_ip);
	sin.sin_port = htons(port);
	rc = connect(fd, (struct sockaddr *)&sin, sizeof(sin));
	if (rc < 0) {
		perror("connect");
		return rc;
	}

	return fd;
}

static int read_data(int fd)
{
	struct sctp_sndrcvinfo sinfo;
	static char buf[1024];
	int rc, flags;

	memset(&sinfo, 0, sizeof(sinfo));
	rc = sctp_recvmsg(fd, buf, sizeof(buf), NULL, NULL, &sinfo, &flags);
	if (rc < 0) {
		if (errno == EAGAIN)
			return 0;
		perror("sctp_recvmsg");
		return rc;
	} else if (rc == 0) {
		fprintf(stderr, "sctp_recvmsg: short read\n");
		return -1;
	}
	/* ignore notifications */
	if (flags & MSG_NOTIFICATION)
		return 0;
	buf[rc] = '\0';
	printf("Received %u bytes on PPID %u / Stream %u: %s\n",
		rc, ntohl(sinfo.sinfo_ppid), sinfo.sinfo_stream, buf);
	return 1;
}

static int send_data(int fd)
{
	struct sctp_sndrcvinfo sinfo = {
		.sinfo_ppid = htonl(23),
		.sinfo_stream = 0,
	};
	static char buf[1024];
	int rc;

	snprintf(buf, sizeof(buf), "012345678901234567890123");
	rc = sctp_send(fd, buf, strlen(buf), &sinfo, MSG_NOSIGNAL);
	if (rc <= 0) {
		perror("sctp_send");
		return rc;
	}
	return 0;
}

static int server_mainloop(int srv_fd, int enable_dry_events)
{
	while (1) {
		int rc, fd;

		rc = accept(srv_fd, NULL, 0);
		if (rc < 0)
			return rc;
		fd = rc;

		if (enable_dry_events) {
			rc = enable_events(fd);
			if (rc < 0)
				return rc;
		}

		rc = make_nodelay(fd);
		if (rc < 0)
			return rc;

		rc = make_nonblock(fd);
		if (rc < 0)
			return rc;

		while (1) {
			fd_set readset;

			FD_ZERO(&readset);
			FD_SET(fd, &readset);

			rc = select(fd+1, &readset, NULL, NULL, NULL);
			if (rc == 0)
				continue;
			if (rc < 0) {
				perror("select");
				break;
			}
			if (FD_ISSET(fd, &readset)) {
				rc = read_data(fd);
				if (rc < 0)
					break;
				if (rc == 1) {
					printf("Sending Response\n");
					rc = send_data(fd);
					if (rc < 0)
						break;
				}
			}
		}
		close(fd);
	}
}

static void print_usage_and_exit(int rc)
{
	printf("You can call this program the following ways:\n"
		"sctp-dry-event client\n"
		"\t* Run in client mode, connect to server\n"
		"\t* Send a single 24byte DATA message\n"
		"\t* Wait indefinitely for any response\n"
		"sctp-dry-event server\n"
		"\t* Run in server mode, wait for client connection\n"
		"\t* Do a non-blocking read\n"
		"\t* Send a response back to the client\n"
		"\t* Close the socket and re-start\n"
		"sctp-dry-event server dry\n"
		"\t* Run in server mode like above, but activate SENDER_DRY_EVENT\n"
		"\tIf you connect the client in this mode, there are high chances \n"
		"\tthat the server will not get out of the select() of the non-blocking\n"
		"\tread, which is the bug this tool is designed to point out\n");
	exit(rc);
}

int main(int argc, char **argv)
{
	int fd, rc;

	if (argc <= 1)
		print_usage_and_exit(0);

	if (!strcmp(argv[1], "server")) {
		int enable_dry = 0;
		if (argc > 2) {
			if (!strcmp(argv[2], "dry"))
				enable_dry = 1;
			else
				print_usage_and_exit(1);
		}

		printf("Running SCTP Server%s\n", enable_dry ? " WITH DRY EVENTS" : "");

		fd = create_sctp_socket_server(2905);
		if (fd < 0) {
			perror("Error creating server socket");
			exit(1);
		}

		server_mainloop(fd, enable_dry);
	} else if (!strcmp(argv[1], "client")) {
		printf("Connecting SCTP Client\n");
		fd = create_sctp_socket_client(0x7f000001, 2905);
		if (fd < 0) {
			perror("Error creating client socket");
			exit(1);
		}
		printf("Connected, sending data...\n");
		rc = send_data(fd);
		if (rc < 0)
			exit(1);
		printf("Waiting for response...\n");
		while (read_data(fd) == 0) { }
		close(fd);
	} else
		print_usage_and_exit(1);
}
