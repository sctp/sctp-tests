#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#include <assert.h>

#define BUFFERSIZE 1
#define LOOPS 100000

static int fds[50][50];

static int create_assocs(int family, char *local_port,
		struct sockaddr_storage addr_ser, int cnt)
{
	int fd, i, j;
	char addr_str[64];
	struct addrinfo *res = NULL;
	struct sockaddr_storage addr_cli;

	for(i = 1; i <= cnt; i++){
		for(j = 1; j <= cnt; j++) {
			memset(&addr_cli, 0, sizeof(addr_cli));

			if (family == AF_INET) {
				sprintf(addr_str, "172.16.%d.%d", i, j);
			} else {
				sprintf(addr_str, "2020::%d:%d", i, j);
			}

			if (getaddrinfo(addr_str, local_port, NULL, &res) != 0) {
				perror("getaddrinfo");
				exit(-1);
			}
			assert(res->ai_family == family);
			memcpy(&addr_cli, res->ai_addr, res->ai_addrlen);

			if ((fd = socket(res->ai_family, SOCK_SEQPACKET, IPPROTO_SCTP)) == -1){
				perror("socket");
				exit(-1);
			}
			freeaddrinfo(res);

			if (bind(fd, (struct sockaddr *)&addr_cli, sizeof(addr_cli)) == -1){
				perror("bind");
				exit(-1);
			}

			if (connect(fd, (struct sockaddr *)&addr_ser, sizeof(addr_ser)) == -1){
				perror("connect");
				exit(-1);
			}
			fds[i-1][j-1] = fd;
		}
	}
	return 0;
}

int main(int argc, char **argv)
{
	int fd, msg_flags, send_len, recv_len, i = 0, j, cnt;
	unsigned long total_len = 0;
	time_t start, end;
	char addr_str[64];
	char buf[BUFFERSIZE];

	struct sctp_sndrcvinfo sri;
	struct addrinfo *local_info = NULL, *remote_info = NULL;
	struct sockaddr_storage peeraddr;
	socklen_t len = sizeof(peeraddr);

	struct sockaddr_storage addr_cli, addr_ser;
	double time_spent = 0.0;

	if (argc != 5) {
		printf("Usage: %s <remote_addr> <remote_port> <local_port> <addr_cnt>\n", argv[0]);
		exit(-1);
	}

	memset(&addr_ser, 0, sizeof(addr_ser));
	if (getaddrinfo(argv[1], argv[2], NULL, &remote_info) != 0) {
		perror("getaddrinfo");
		exit(-1);
	}
	memcpy(&addr_ser, remote_info->ai_addr, remote_info->ai_addrlen);

	memset(&addr_cli, 0, sizeof(addr_cli));
	if (remote_info->ai_family == AF_INET) {
		sprintf(addr_str, "172.16.%d.%d", 253, 253);
	} else {
		sprintf(addr_str, "2020::%d:%d", 253, 253);
	}
	freeaddrinfo(remote_info);

	if (getaddrinfo(addr_str, argv[3], NULL, &local_info) != 0) {
		perror("getaddrinfo");
		exit(-1);
	}
	memcpy(&addr_cli, local_info->ai_addr, local_info->ai_addrlen);

	if ((fd = socket(local_info->ai_family, SOCK_SEQPACKET, IPPROTO_SCTP)) == -1){
		perror("socket");
		exit(-1);
	}

	if (bind(fd, (struct sockaddr *)&addr_cli, sizeof(addr_cli)) == -1){
		perror("bind");
		exit(-1);
	}

	if (connect(fd, (struct sockaddr *)&addr_ser, sizeof(addr_ser)) == -1){
		perror("connect");
		exit(-1);
	}

	cnt = atoi(argv[4]);
	create_assocs(local_info->ai_family, argv[3], addr_ser, cnt);
	freeaddrinfo(local_info);

	start = time(NULL);
	while(i < LOOPS){
		if ((send_len = sctp_sendmsg(fd, buf, BUFFERSIZE,
			(struct sockaddr *)&addr_ser, sizeof(addr_ser), 0, 0, 0, 0, 0)) == -1) {
			perror("sctp_sendmsg");
			exit(-1);
		}
		total_len += send_len;
		i++;
	}

	end = time(NULL);
	time_spent = difftime(end, start);
	printf("time is %lf, send pkt is %lu\n", time_spent, total_len);

	if (time_spent > 1000) {
		printf("Expect spending less than 1000 seconds\n");
		return 1;
	}

	for(i = 1; i <= cnt; i++)
		for(j = 1; j <= cnt; j++)
			close(fds[i-1][j-1]);

	close(fd);
	sleep(30);

	return 0;
}
