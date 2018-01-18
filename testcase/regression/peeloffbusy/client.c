#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/sctp.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>

static int get_asoc_id(int fd)
{
	int len = (1 * sizeof(sctp_assoc_t)) + sizeof(uint32_t);
	char *buf = (char *)malloc(len);

	if (getsockopt(fd, IPPROTO_SCTP, SCTP_GET_ASSOC_ID_LIST,
	    (void *)buf, &len) == -1) {
		perror("getsockopt");
		return 0;
	}

	return ((struct sctp_assoc_ids *)buf)->gaids_assoc_id[0];
}

static void *peeloff_sctp(void *args)
{
	int sctpsd = (long long)args;
	sctp_peeloff_arg_t peeloff;
	int optlen, newsd, associd;

	associd = get_asoc_id(sctpsd);
	printf("sd is %d, %d\n", sctpsd, associd);
	if (!associd)
		goto out;

	sleep(5);
	peeloff.associd = associd;
	optlen = sizeof(peeloff);

	newsd = getsockopt(sctpsd, IPPROTO_SCTP, SCTP_SOCKOPT_PEELOFF,
			   &peeloff, &optlen);
	printf("new sd %d\n", newsd);
	if (newsd == -1)
		perror("getsockopt");

out:
	system("iptables -F");
	return NULL;
}

int main(int argc, char **argv)
{
	struct addrinfo *remote_info;
	struct sockaddr_in servaddr;
	char a[1024] = "hello";
	int sctpsd, i, err;
	pthread_t peel;
	int pass = 0;

	signal(SIGPIPE, SIG_IGN);
	bzero((void *)&servaddr, sizeof(servaddr));

	err = getaddrinfo(argv[1], argv[2], NULL, &remote_info);
	if (err) {
		perror("getaddrinfo");
		return !pass;
	}
	memcpy(&servaddr, remote_info->ai_addr, remote_info->ai_addrlen);
	printf("Starting SCTP client connection to %s:%s\n", argv[1], argv[2]);

	sctpsd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	err = connect(sctpsd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	if (err == -1) {
		perror("connect");
		goto out;
	}

	pthread_create(&peel, NULL, peeloff_sctp, (void *)(long long)sctpsd);
	system("iptables -A OUTPUT -p sctp --chunk-types any DATA -j DROP");
	for (i = 0; i < 100; i++) {
		err = sctp_sendmsg(sctpsd, (const void *)a, sizeof(a),
				   &servaddr, sizeof(servaddr), 0, 0, 0, 0, 0);
		if (errno == EPIPE) {
			pass = 1;
			break;
		}
	}
	pthread_join(peel, NULL);

out:
	close(sctpsd);
	return !pass;
}
