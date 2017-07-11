#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <linux/sctp.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <netdb.h>

#define RECVBUFSIZE     4096
#define PPID            1234

static int pass = 1;

static int get_asoc_id(int fd)
{
        char *buf;
        int len, asoc_id = 0;

        len = (1 * sizeof(sctp_assoc_t)) + sizeof(uint32_t);
        buf = (char *)malloc(len);

        if (getsockopt(fd, IPPROTO_SCTP, SCTP_GET_ASSOC_ID_LIST, (void *)buf, &len) < 0) {
                perror("getsockopt");
                exit(-1);
        }

        asoc_id = ((struct sctp_assoc_ids *)buf)->gaids_assoc_id[0];
        return asoc_id;
}

void *peeloff_sctp(void *args)
{
	int SctpScocket = (long long)args;
	sctp_peeloff_arg_t peeloff;
	int optlen, newsd, associd;
	int err;

	associd = get_asoc_id(SctpScocket);
	sleep(5);
	printf("sd is %d, %d\n", SctpScocket, associd);

	peeloff.associd = associd;
	optlen = sizeof(peeloff);
	newsd = getsockopt(SctpScocket, IPPROTO_SCTP, SCTP_SOCKOPT_PEELOFF, &peeloff, &optlen);
	if (errno != EBUSY)
		pass = 0;
	printf("new sd %d\n", newsd);
	system("iptables -F");

	return NULL;
}

int main(int argc, char **argv)
{
	int SctpScocket, in, flags;
	socklen_t opt_len;
	char * szAddress;
	int iPort;
	char * szMsg;
	int iMsgSize;
	char a[1024] = "1024";
	pthread_t peel;
	int i=0;

	struct sockaddr_in servaddr = {0};
	struct sctp_status status = {0};
	struct sctp_sndrcvinfo sndrcvinfo = {0};
	struct sctp_event_subscribe events = {0};
	struct sctp_initmsg initmsg = {0};
	char * szRecvBuffer[RECVBUFSIZE + 1] = {0};
	socklen_t from_len = (socklen_t) sizeof(struct sockaddr_in);
        struct addrinfo *remote_info;

	//get the arguments
	bzero( (void *)&servaddr, sizeof(servaddr) );
        if (getaddrinfo(argv[1], argv[2], NULL, &remote_info) != 0) {
                perror("getaddrinfo");
                exit(-1);
        }
        memcpy(&servaddr, remote_info->ai_addr, remote_info->ai_addrlen);

	printf("Starting SCTP client connection to %s:%s\n", argv[1], argv[2]);
	SctpScocket = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
	printf("socket created...\n");

	//set the association options
	initmsg.sinit_num_ostreams = 1;
	setsockopt( SctpScocket, IPPROTO_SCTP, SCTP_INITMSG, &initmsg,sizeof(initmsg));
	printf("setsockopt succeeded...\n");


	connect( SctpScocket, (struct sockaddr *)&servaddr, sizeof(servaddr));
	printf("connect succeeded...\n");

	//check status
	opt_len = (socklen_t) sizeof(struct sctp_status);
	getsockopt(SctpScocket, IPPROTO_SCTP, SCTP_STATUS, &status, &opt_len);

	pthread_create(&peel, NULL, peeloff_sctp, (void *)(long long)SctpScocket);
	system("iptables -A OUTPUT -p sctp --chunk-types any DATA -j DROP");

	for(i=0; i<100; i++)
	{
		int len = sctp_sendmsg(SctpScocket, (const void *)a, sizeof(a), &servaddr, sizeof(servaddr), htonl(PPID), 0, 0 , 0, 0);
	} 
	pthread_join(peel, NULL);

	close(SctpScocket);

	return !pass;
}
