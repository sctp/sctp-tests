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
#include <netdb.h>
#include <pthread.h>

struct sn_context {
	int fd;
	int c;
	long cb[4];
	int asoc_id;
	struct sctp_sndrcvinfo *sri;
	struct sockaddr_storage *serv;
};

struct sn_case {
	int drop;
	int flags;
	int values[9];
	int length;
	int (*do_setup_pr)(struct sn_context *snc);
	int (*do_test_pr)(struct sn_context *snc);
	int (*do_check_pr)(struct sn_context *snc);
};

enum {
	SN_CMD_SET_STRRESET = 11,
	SN_CMD_CLOSE_ASOC,
	SN_CMD_ACK,
};

#define SN_CASE_MAX 19

static struct sn_case sn_cases[SN_CASE_MAX];

static int sn_send_asoc_cmd(struct sn_context *snc, int msg_len);
static int sn_send_asoc_msg(struct sn_context *snc, int msg_len, int sid,
			    int flags, int value);
static int sn_close_asoc_streams(struct sn_context *snc);
static int sn_reopen_asoc_streams(struct sn_context *snc);
static int sn_do_setup_so_sndbuf(struct sn_context *snc);

static int sn_do_setup_pr(struct sn_context *snc)
{
	return 0;
}

static int sn_do_test_pr(struct sn_context *snc)
{
	sn_close_asoc_streams(snc);
	sn_send_asoc_msg(snc, sn_cases[snc->c].length, 1,
			 sn_cases[snc->c].flags,
			 sn_cases[snc->c].values[0]);
	sleep(10);
	sn_send_asoc_msg(snc, 1000, 1, 0, 0);
	sn_reopen_asoc_streams(snc);
	return 0;
}

static int sn_do_test_pr_b(struct sn_context *snc)
{
	sn_close_asoc_streams(snc);
	sn_send_asoc_msg(snc, sn_cases[snc->c].length, 1,
			 sn_cases[snc->c].flags | 0x1,
			 sn_cases[snc->c].values[0]);
	sleep(10);
	sn_send_asoc_msg(snc, 1000, 1, 0x1, 0);
	sn_reopen_asoc_streams(snc);
	return 0;
}

static int sn_do_test_pr_c(struct sn_context *snc)
{
	sn_close_asoc_streams(snc);
	sn_send_asoc_msg(snc, 100, 1, sn_cases[snc->c].flags | 0x1,
			 sn_cases[snc->c].values[0]);
	sleep(10);
	sn_send_asoc_msg(snc, 100, 1, 0x1, 0);
	sn_send_asoc_msg(snc, 100, 1, 0, 0);
	sn_reopen_asoc_streams(snc);
	return 0;
}

static int sn_do_test_pr_x(struct sn_context *snc)
{
	sn_do_setup_so_sndbuf(snc);
	sn_close_asoc_streams(snc);
	sn_send_asoc_msg(snc, sn_cases[snc->c].length, 1,
			 sn_cases[snc->c].flags,
			 sn_cases[snc->c].values[0]);
	sn_send_asoc_msg(snc, 1000, 1, sn_cases[snc->c].flags,
			 sn_cases[snc->c].values[0] - 1);
	sn_reopen_asoc_streams(snc);
	return 0;
}

static int sn_do_test_pr_bx(struct sn_context *snc)
{
	sn_do_setup_so_sndbuf(snc);
	sn_close_asoc_streams(snc);
	sn_send_asoc_msg(snc, sn_cases[snc->c].length, 1,
			 sn_cases[snc->c].flags | 0x1,
			 sn_cases[snc->c].values[0]);
	sn_send_asoc_msg(snc, 1000, 1, sn_cases[snc->c].flags | 0x1,
			 sn_cases[snc->c].values[0] - 1);
	sn_reopen_asoc_streams(snc);
	return 0;
}

static int sn_do_test_pr_cx(struct sn_context *snc)
{
	sn_do_setup_so_sndbuf(snc);
	sn_close_asoc_streams(snc);
	sn_send_asoc_msg(snc, sn_cases[snc->c].length, 1,
			 sn_cases[snc->c].flags | 0x1,
			 sn_cases[snc->c].values[0]);
	sn_send_asoc_msg(snc, 1000, 1, sn_cases[snc->c].flags | 0x1,
			 sn_cases[snc->c].values[0] - 1);
	sn_send_asoc_msg(snc, 1000, 1, sn_cases[snc->c].flags,
			 sn_cases[snc->c].values[0] - 1);
	sn_reopen_asoc_streams(snc);
	return 0;
}

static int sn_do_check_pr_cx(struct sn_context *snc)
{
	printf("expected %d, get: %d\n", 1000, snc->cb[2]);

	snc->cb[3]++;
	snc->cb[4] += snc->cb[2];
	if (snc->cb[3] == 1) {
		if (snc->cb[4] != 1000)
			snc->cb[1] = 1;
		else
			snc->cb[1] = 0;
	} else {
		if (snc->cb[4] != 2000)
			snc->cb[1] = 1;
		else
			snc->cb[1] = 0;
	}

	return 0;
}

static void *cleanup_sctp(void *args)
{
	struct sn_context *snc = args;

	sleep(3);
	sn_reopen_asoc_streams(snc);
}

static int sn_do_test_pr_cxx(struct sn_context *snc)
{
	pthread_t clean;

	sn_do_setup_so_sndbuf(snc);
	sn_close_asoc_streams(snc);
	sn_send_asoc_msg(snc, 1, 1, sn_cases[snc->c].flags | 0x1,
			 sn_cases[snc->c].values[0]);
	sn_send_asoc_msg(snc, 1, 1, sn_cases[snc->c].flags | 0x1,
			 sn_cases[snc->c].values[0] - 1);
	sn_send_asoc_msg(snc, 1, 1, sn_cases[snc->c].flags | 0x1,
			 sn_cases[snc->c].values[0]);
	sn_send_asoc_msg(snc, 1, 1, sn_cases[snc->c].flags | 0x1,
			 sn_cases[snc->c].values[0]);
	sn_send_asoc_msg(snc, 1, 1, sn_cases[snc->c].flags | 0x1,
			 sn_cases[snc->c].values[0] - 1);
	sleep(10);
	pthread_create(&clean, NULL, cleanup_sctp, (void *)snc);
	sn_send_asoc_msg(snc, 1000, 1, sn_cases[snc->c].flags | 0x1,
			 sn_cases[snc->c].values[0] - 1);
	sn_send_asoc_msg(snc, 1000, 1, sn_cases[snc->c].flags,
			 sn_cases[snc->c].values[0] - 1);
	pthread_join(clean, NULL);
	sleep(3);

	return 0;
}

static int sn_do_test_pr_cxxx(struct sn_context *snc)
{
	pthread_t clean;

	sn_do_setup_so_sndbuf(snc);
	sn_close_asoc_streams(snc);
	sn_send_asoc_msg(snc, 1, 0, sn_cases[snc->c].flags | 0x1,
			 sn_cases[snc->c].values[0]);
	sn_send_asoc_msg(snc, 1, 1, sn_cases[snc->c].flags | 0x1,
			 sn_cases[snc->c].values[0] - 1);
	sn_send_asoc_msg(snc, 1, 2, sn_cases[snc->c].flags | 0x1,
			 sn_cases[snc->c].values[0]);
	sn_send_asoc_msg(snc, 1, 3, sn_cases[snc->c].flags | 0x1,
			 sn_cases[snc->c].values[0]);
	sn_send_asoc_msg(snc, 1, 4, sn_cases[snc->c].flags | 0x1,
			 sn_cases[snc->c].values[0] - 1);
	sleep(10);
	pthread_create(&clean, NULL, cleanup_sctp, (void *)snc);
	sn_send_asoc_msg(snc, 1000, 2, sn_cases[snc->c].flags | 0x1,
			 sn_cases[snc->c].values[0] - 1);
	sn_send_asoc_msg(snc, 1000, 3, sn_cases[snc->c].flags,
			 sn_cases[snc->c].values[0] - 1);
	pthread_join(clean, NULL);
	sleep(3);

	return 0;
}

static int sn_do_check_pr_cxx(struct sn_context *snc)
{
	printf("expected %d, get: %d\n", 1000, snc->cb[2]);

	snc->cb[3]++;
	snc->cb[4] += snc->cb[2];

	switch (snc->cb[3]) {
	case 1:
		if (snc->cb[4] != 1)
			snc->cb[1] = 1;
		else
			snc->cb[1] = 0;
		break;
	case 2:
		if (snc->cb[4] != 2)
			snc->cb[1] = 1;
		else
			snc->cb[1] = 0;
		break;
	case 3:
		if (snc->cb[4] != 1002)
			snc->cb[1] = 1;
		else
			snc->cb[1] = 0;
		break;
	case 4:
		if (snc->cb[4] != 2002)
			snc->cb[1] = 1;
		else
			snc->cb[1] = 0;
		break;
	}

	return 0;
}


static int sn_do_check_pr(struct sn_context *snc)
{
	printf("expected %d, get: %d\n", 1000, snc->cb[2]);

	if (snc->cb[2] != 1000)
		snc->cb[1] = 1;
	else
		snc->cb[1] = 0;

	return 0;
}

static int sn_do_check_pr_c(struct sn_context *snc)
{
	printf("expected %d, get: %d\n", 100, snc->cb[2]);

	snc->cb[3]++;
	snc->cb[4] += snc->cb[2];
	if (snc->cb[3] == 1) {
		if (snc->cb[4] != 100)
			snc->cb[1] = 1;
		else
			snc->cb[1] = 0;
	} else {
		if (snc->cb[4] != 200)
			snc->cb[1] = 1;
		else
			snc->cb[1] = 0;
	}

	return 0;
}

/* ttl test 0*/
static struct sn_case sn_cases[SN_CASE_MAX] = {
	{ /* ttl conformance A */
		.drop = 1,
		.flags = SCTP_PR_SCTP_TTL,
		.values = {5000},
		.length = 1024,
		.do_setup_pr = sn_do_setup_pr,
		.do_test_pr = sn_do_test_pr,
		.do_check_pr = sn_do_check_pr,
	},
	{ /* ttl conformance B */
		.drop = 1,
		.flags = SCTP_PR_SCTP_TTL,
		.values = {5000},
		.length = 1024,
		.do_setup_pr = sn_do_setup_pr,
		.do_test_pr = sn_do_test_pr_b,
		.do_check_pr = sn_do_check_pr,
	},
	{ /* ttl conformance C */
		.drop = 1,
		.flags = SCTP_PR_SCTP_TTL,
		.values = {5000},
		.length = 1024,
		.do_setup_pr = sn_do_setup_pr,
		.do_test_pr = sn_do_test_pr_c,
		.do_check_pr = sn_do_check_pr_c,
	},
	{ /* rtx conformance A */
		.drop = 1,
		.flags = SCTP_PR_SCTP_RTX,
		.values = {2},
		.length = 1024,
		.do_setup_pr = sn_do_setup_pr,
		.do_test_pr = sn_do_test_pr,
		.do_check_pr = sn_do_check_pr,
	},
	{ /* rtx conformance B */
		.drop = 1,
		.flags = SCTP_PR_SCTP_RTX,
		.values = {2},
		.length = 1024,
		.do_setup_pr = sn_do_setup_pr,
		.do_test_pr = sn_do_test_pr_b,
		.do_check_pr = sn_do_check_pr,
	},
	{ /* rtx conformance C */
		.drop = 1,
		.flags = SCTP_PR_SCTP_RTX,
		.values = {2},
		.length = 1024,
		.do_setup_pr = sn_do_setup_pr,
		.do_test_pr = sn_do_test_pr_c,
		.do_check_pr = sn_do_check_pr_c,
	},

	{ /* prio conformance A */
		.drop = 1,
		.flags = SCTP_PR_SCTP_PRIO,
		.values = {2},
		.length = 4500,
		.do_setup_pr = sn_do_setup_pr,
		.do_test_pr = sn_do_test_pr_x,
		.do_check_pr = sn_do_check_pr,
	},
	{ /* prio conformance B */
		.drop = 1,
		.flags = SCTP_PR_SCTP_PRIO,
		.values = {2},
		.length = 4500,
		.do_setup_pr = sn_do_setup_pr,
		.do_test_pr = sn_do_test_pr_bx,
		.do_check_pr = sn_do_check_pr,
	},
	{ /* prio conformance C */
		.drop = 1,
		.flags = SCTP_PR_SCTP_PRIO,
		.values = {2},
		.length = 4500,
		.do_setup_pr = sn_do_setup_pr,
		.do_test_pr = sn_do_test_pr_cx,
		.do_check_pr = sn_do_check_pr_cx,
	},
	{ /* prio conformance D */
		.drop = 1,
		.flags = SCTP_PR_SCTP_PRIO,
		.values = {2},
		.length = 500,
		.do_setup_pr = sn_do_setup_pr,
		.do_test_pr = sn_do_test_pr_cxx,
		.do_check_pr = sn_do_check_pr_cxx,
	},
	{ /* prio conformance E */
		.drop = 1,
		.flags = SCTP_PR_SCTP_PRIO,
		.values = {2},
		.length = 500,
		.do_setup_pr = sn_do_setup_pr,
		.do_test_pr = sn_do_test_pr_cxxx,
		.do_check_pr = sn_do_check_pr_cxx,
	},
};

/* socket */
static int sn_get_asoc_id(int fd)
{
	int len = (1 * sizeof(sctp_assoc_t)) + sizeof(uint32_t);
	char *buf = (char *)malloc(len);

	if (getsockopt(fd, IPPROTO_SCTP, SCTP_GET_ASSOC_ID_LIST,
		       (void *)buf, &len)) {
		perror("getsockopt");
		exit(-1);
	}

	return ((struct sctp_assoc_ids *)buf)->gaids_assoc_id[0];
}

static int sn_do_setup_frag_interleave(struct sn_context *snc)
{
	int param, param_len;

	param = 1;
	param_len = sizeof(param);
	if (setsockopt(snc->fd, IPPROTO_SCTP, SCTP_FRAGMENT_INTERLEAVE,
		       &param, sizeof(param))) {
		perror("setsockopt");
		exit(-1);
	}

	return 0;
}

static int sn_do_setup_strm_interleave(struct sn_context *snc)
{
	struct sctp_assoc_value param;

	param.assoc_id = snc->asoc_id;
	param.assoc_value = sizeof(param);

	if (setsockopt(snc->fd, IPPROTO_SCTP, SCTP_INTERLEAVING_SUPPORTED,
		       &param, sizeof(param))) {
		perror("setsockopt");
	}

	return 0;
}

static int sn_create_asoc_sk(struct sn_context *snc, char *ip, char *port)
{
	struct addrinfo *remote_info;

	if (getaddrinfo(ip, port, NULL, &remote_info)) {
		perror("getaddrinfo");
		exit(-1);
	}
	memcpy(snc->serv, remote_info->ai_addr, remote_info->ai_addrlen);

	snc->fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);

	sn_do_setup_frag_interleave(snc);
	sn_do_setup_strm_interleave(snc);

	if (connect(snc->fd, (struct sockaddr *)snc->serv,
		    sizeof(*snc->serv))) {
		perror("connect");
		exit(-1);
	}

	snc->asoc_id = sn_get_asoc_id(snc->fd);

	return 0;
}

static int sn_create_listen_sk(struct sn_context *snc, char *ip, char *port)
{
	struct addrinfo *remote_info;

	if (getaddrinfo(ip, port, NULL, &remote_info)) {
		perror("getaddrinfo");
		exit(-1);
	}

	snc->fd = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);

	sn_do_setup_frag_interleave(snc);
	sn_do_setup_strm_interleave(snc);

	if (bind(snc->fd, (struct sockaddr *)remote_info->ai_addr,
		 remote_info->ai_addrlen)) {
		perror("bind");
		exit(-1);
	}

	if (listen(snc->fd, 10)) {
		perror("listen");
		exit(-1);
	}

	return 0;
}

static int sn_close_asoc_sk(struct sn_context *snc)
{
	sn_send_asoc_cmd(snc, SN_CMD_CLOSE_ASOC);

	close(snc->fd);
	printf("closed asoc done\n");
	sleep(5);
}

/* setsockopt */
int sn_enable_asoc_prsctp_support(struct sn_context *snc)
{
	struct sctp_assoc_value params;

	params.assoc_id = snc->asoc_id;
	params.assoc_value = 1;
	if (setsockopt(snc->fd, IPPROTO_SCTP, SCTP_PR_SUPPORTED, &params,
		       sizeof(params))) {
		perror("setsockopt");
		exit(-1);
	}
}

static int sn_enable_asoc_strreset(struct sn_context *snc)
{
	struct sctp_assoc_value strresetparam;

	snc->asoc_id = sn_get_asoc_id(snc->fd);
	strresetparam.assoc_id = snc->asoc_id;
	strresetparam.assoc_value = SCTP_ENABLE_RESET_STREAM_REQ;
	if (setsockopt(snc->fd, IPPROTO_SCTP, SCTP_ENABLE_STREAM_RESET,
		       &strresetparam, sizeof(strresetparam))) {
		perror("setsockopt");
		exit(-1);
	}

	return 0;
}

static int sn_enable_sk_recvinfo(struct sn_context *snc)
{
	struct sctp_event_subscribe events;

	memset(&events, 0, sizeof(events));
	events.sctp_data_io_event = 1;
	if (setsockopt(snc->fd, IPPROTO_SCTP, SCTP_EVENTS, &events,
		       sizeof(events))) {
		perror("setsockopt");
		exit(-1);
	}

	return 0;
}

/* stream */
static int sn_close_asoc_streams(struct sn_context *snc)
{
	struct sctp_reset_streams sconfparam;

	if (!sn_cases[snc->c].drop)
		return;

	system("iptables -A OUTPUT -p sctp --chunk-types any DATA -j DROP");
	printf("all stream data will be drop in INPUT\n");

	return 0;
}

static int sn_reopen_asoc_streams(struct sn_context *snc)
{
	if (!sn_cases[snc->c].drop)
		return;

	system("iptables -F");
	printf("all stream data can be transmitted now\n");
	sleep(10);

	return 0;
}

/* sn cmd process */
static int sn_send_asoc_cmd(struct sn_context *snc, int msg_len)
{
	struct sockaddr_storage peeraddr;
	int recv_len, len, msg_flags = 0;
	struct sctp_sndrcvinfo sri;
	char buf[1024];

	memset(&peeraddr, 0, sizeof(peeraddr));
	len = sizeof(peeraddr);
	if (sctp_sendmsg(snc->fd, (const void *)buf, msg_len, snc->serv,
			 sizeof(*snc->serv), 0, 0, 0, 0, 0) == -1) {
		perror("sendmsg");
		exit(-1);
	}

	recv_len = sctp_recvmsg(snc->fd, buf, sizeof(buf),
				(struct sockaddr *)&peeraddr,
				&len, &sri, &msg_flags);
	if (recv_len == -1) {
		perror("recvmsg");
		exit(-1);
	}
	printf("recv_len: %d, expected: %d\n", recv_len, msg_len);

	return 0;
}

static int sn_get_peer_cmd(struct sn_context *snc)
{
	int recv_len, len, msg_flags = 0;
	char buf[20480];

	len = sizeof(*snc->serv);
	memset(snc->serv, 0, len);
	recv_len = sctp_recvmsg(snc->fd, buf, sizeof(buf),
				(struct sockaddr *)snc->serv,
				&len, snc->sri, &msg_flags);
	if (recv_len == -1) {
		perror("recvmsg");
		exit(-1);
	}

	return recv_len;
}

static int sn_enable_peer_strreset(struct sn_context *snc)
{
	sn_send_asoc_cmd(snc, SN_CMD_SET_STRRESET);
}

static int sn_send_asoc_msg(struct sn_context *snc, int msg_len, int sid,
			    int flags, int value)
{
	char buf[10240];

	if (sctp_sendmsg(snc->fd, (const void *)buf, msg_len, snc->serv,
			 sizeof(*snc->serv), 0, flags, sid, value, 0) == -1) {
		perror("sendmsg");
		exit(-1);
	}

	return 0;
}

static int sn_send_peer_ack(struct sn_context *snc)
{
	sn_send_asoc_msg(snc, SN_CMD_ACK, 0, 0, 0);
}

static int sn_do_setup_so_sndbuf(struct sn_context *snc)
{
	int param = 2500;

	if (setsockopt(snc->fd, SOL_SOCKET, SO_SNDBUF, &param, sizeof(int))) {
		perror("setsockopt");
		exit(-1);
	}

	return 0;
}

/* entrance */
int do_client(char **argv)
{
	struct sockaddr_storage servaddr;
	struct sn_context snc;

	sleep(3); /* wait for server */
	memset(&snc, 0, sizeof(snc));
	snc.serv = &servaddr;
	snc.c = atoi(argv[3]);
	sn_create_asoc_sk(&snc, argv[1], argv[2]);

	sn_cases[snc.c].do_setup_pr(&snc);

	sn_enable_asoc_prsctp_support(&snc);

	sn_cases[snc.c].do_test_pr(&snc);

	sleep(5); /* wait for server */
	sn_close_asoc_sk(&snc);

	return 0;
}

int do_server(char **argv)
{
	struct sockaddr_storage peeraddr;
	struct sctp_sndrcvinfo sri;
	struct sn_context snc;
	int cmd = 0;

	memset(&snc, 0, sizeof(snc));
	snc.sri = &sri;
	snc.serv = &peeraddr;
	snc.c = atoi(argv[3]);
	snc.cb[1] = 1;
	sn_create_listen_sk(&snc, argv[1], argv[2]);
	sn_enable_sk_recvinfo(&snc);

	while (cmd >= 0) {
		cmd = sn_get_peer_cmd(&snc);
		switch (cmd) {
		case SN_CMD_SET_STRRESET:
			sn_enable_asoc_strreset(&snc);
			sn_send_peer_ack(&snc);
			break;
		case SN_CMD_CLOSE_ASOC:
			cmd = -1;
			sn_send_peer_ack(&snc);
			break;
		default:
			snc.cb[2] = cmd;
			sn_cases[snc.c].do_check_pr(&snc);
			break;
		}
	}

	return snc.cb[1];
}

int do_help(char **argv)
{
	printf("\n   usage: %s  ip  port  [0-4]  [-l | -s]\n\n", argv[0]);
}

int main(int argc, char **argv)
{
	char *mode = argv[4];

	if (argc != 5)
		return do_help(argv);

	if (!strcmp(mode, "-l"))
		return do_server(argv);
	else if (!strcmp(mode, "-s"))
		return do_client(argv);
	else
		return do_help(argv);
}
