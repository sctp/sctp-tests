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

struct sn_context {
	int fd;
	int sched;
	long cb[4];
	int asoc_id;
	struct sctp_sndrcvinfo *sri;
	struct sockaddr_storage *serv;
};

struct sn_case {
	int sched;
	int sched_cork;
	int sched_sids[9];
	int (*do_setup_sched)(struct sn_context *snc);
	int (*do_setup_schedvalue)(struct sn_context *snc);
	int (*do_sched_tests)(struct sn_context *snc);
	int (*do_assert_tests)(struct sn_context *snc);
};

enum {
	SN_CMD_SET_STRRESET = 11,
	SN_CMD_CLOSE_ASOC,
	SN_CMD_ACK,
};

#define SN_CASE_MAX 21

static struct sn_case sn_cases[SN_CASE_MAX];

static int sn_send_asoc_cmd(struct sn_context *snc, int msg_len);
static int sn_send_asoc_msg(struct sn_context *snc, int msg_len, int sid,
			    int flags);

/* prio test 0*/
static int sn_do_setup_sched(struct sn_context *snc)
{
	int sched = sn_cases[snc->sched].sched;
	struct sctp_assoc_value schedparam;
	int param_len;

	schedparam.assoc_id = snc->asoc_id;
	schedparam.assoc_value = sched;
	param_len = sizeof(schedparam);
	if (setsockopt(snc->fd, IPPROTO_SCTP, SCTP_STREAM_SCHEDULER,
		       &schedparam, sizeof(schedparam))) {
		perror("setsockopt");
		exit(-1);
	}

	if (getsockopt(snc->fd, IPPROTO_SCTP, SCTP_STREAM_SCHEDULER,
		       (void *)&schedparam, &param_len)) {
		perror("getsockopt");
		exit(-1);
	}
	printf("ss sched: %d, expected: %d\n", schedparam.assoc_value,
	       sched);

	return 0;
}

static int sn_do_setup_schedvalue(struct sn_context *snc)
{
	struct sctp_stream_value sdvparam;
	int param_len, stream;

	for (stream = 1; stream < 4; stream++) {
		sdvparam.assoc_id = snc->asoc_id;
		sdvparam.stream_id = stream;
		sdvparam.stream_value = 10 - stream;
		param_len = sizeof(sdvparam);
		if (setsockopt(snc->fd, IPPROTO_SCTP,
			       SCTP_STREAM_SCHEDULER_VALUE, &sdvparam,
			       sizeof(sdvparam))) {
			perror("setsockopt");
			exit(-1);
		}
		if (getsockopt(snc->fd, IPPROTO_SCTP,
			       SCTP_STREAM_SCHEDULER_VALUE, &sdvparam,
			       &param_len)) {
			perror("getsockopt");
			exit(-1);
		}
		printf("ss schedvalue: %d, expected: %d\n",
		       sdvparam.stream_value, 10 - stream);
	}

	return 0;
}

static int sn_do_sched_tests(struct sn_context *snc)
{
	int stream, count;

	for (stream = 1; stream < 4; stream++)
		for (count = 0; count < 3; count++)
			sn_send_asoc_msg(snc, 1024, stream, 0);
}

static int sn_do_assert_tests(struct sn_context *snc)
{
	int *sids = sn_cases[snc->sched].sched_sids;

	if (snc->sri->sinfo_stream != sids[snc->cb[0]]) {
		snc->cb[1] = 1;
		printf("unexpected sid packet !!!\n");
	}

	printf("sid: %d, expected: %d\n", snc->sri->sinfo_stream,
	       sids[snc->cb[0]]);

	snc->cb[0]++;
}

/* prio test 1 */
static int sn_do_prio_setup_schedvalue(struct sn_context *snc)
{
	struct sctp_stream_value sdvparam;
	int param_len, stream;

	for (stream = 1; stream < 4; stream++) {
		int prio_value = (10 - stream)/8 + 8;

		sdvparam.assoc_id = snc->asoc_id;
		sdvparam.stream_id = stream;
		sdvparam.stream_value = prio_value;
		param_len = sizeof(sdvparam);
		if (setsockopt(snc->fd, IPPROTO_SCTP,
			       SCTP_STREAM_SCHEDULER_VALUE, &sdvparam,
			       sizeof(sdvparam))) {
			perror("setsockopt");
			exit(-1);
		}
		if (getsockopt(snc->fd, IPPROTO_SCTP,
			       SCTP_STREAM_SCHEDULER_VALUE, &sdvparam,
			       &param_len)) {
			perror("getsockopt");
			exit(-1);
		}
		printf("ss schedvalue: %d, expected: %d\n",
		       sdvparam.stream_value, prio_value);
	}

	return 0;
}

/* prio test 2 */
static int sn_do_prio_stress_setup_schedvalue(struct sn_context *snc)
{
	struct sctp_stream_value sdvparam;
	int param_len, stream;

	for (stream = 0; stream < 10; stream++) {
		int prio_value = (10 - stream);

		sdvparam.assoc_id = snc->asoc_id;
		sdvparam.stream_id = stream;
		sdvparam.stream_value = prio_value;
		param_len = sizeof(sdvparam);
		if (setsockopt(snc->fd, IPPROTO_SCTP,
			       SCTP_STREAM_SCHEDULER_VALUE, &sdvparam,
			       sizeof(sdvparam))) {
			perror("setsockopt");
			exit(-1);
		}
		if (getsockopt(snc->fd, IPPROTO_SCTP,
			       SCTP_STREAM_SCHEDULER_VALUE, &sdvparam,
			       &param_len)) {
			perror("getsockopt");
			exit(-1);
		}
		printf("ss schedvalue: %d, expected: %d\n",
		       sdvparam.stream_value, prio_value);
	}

	return 0;
}

static int sn_do_prio_sched_tests(struct sn_context *snc)
{
	int stream, count, repeat;

	for (repeat = 0; repeat < 10000; repeat++)
		for (stream = 0; stream < 10 ; stream++)
			for (count = 0; count < 10; count++)
				sn_send_asoc_msg(snc, 1024, stream, 0);
	sleep(10);
}

static int sn_do_prio_sched_tests_x(struct sn_context *snc)
{
	int stream, count, repeat;

	for (repeat = 0; repeat < 10000; repeat++)
		for (stream = 0; stream < 10 ; stream++)
			for (count = 0; count < 10; count++)
				sn_send_asoc_msg(snc, 1024, stream, 1);
	sleep(10);
}

static int sn_do_prio_sched_tests_y(struct sn_context *snc)
{
	int stream, count, repeat;

	for (repeat = 0; repeat < 10000; repeat++)
		for (stream = 0; stream < 10 ; stream++)
			for (count = 0; count < 5; count++) {
				sn_send_asoc_msg(snc, 1024, stream, 0);
				sn_send_asoc_msg(snc, 1024, stream, 1);
			}
	sleep(10);
}

static int sn_do_prio_assert_tests(struct sn_context *snc)
{
	snc->cb[0]++;

	if (snc->cb[0] == 1) {
		time(&snc->cb[3]);
	} else if (snc->cb[0] == 1000000) {
		long end;

		time(&end);
		printf("PERF: %lf mb/s\n", 8000.0 / (end - snc->cb[3]));
	}
}

/* prio test 3 */
static int sn_do_prio_stress_sched_tests(struct sn_context *snc)
{
	int stream, count, repeat;

	for (repeat = 0; repeat < 1000; repeat++)
		for (stream = 0; stream < 10 ; stream++)
			for (count = 0; count < 10; count++)
				sn_send_asoc_msg(snc, 10240, stream, 0);
	sleep(10);
}

static int sn_do_prio_stress_assert_tests(struct sn_context *snc)
{
	snc->cb[0] += snc->cb[2];

	if (!snc->cb[3]) {
		time(&snc->cb[3]);
	} else if (snc->cb[0] == 1024000000) {
		long end;

		time(&end);
		printf("PERF: %lf mb/s\n", 8000.0 / (end - snc->cb[3]));
	}
}

/* prio test 4 */
static int sn_do_prio_small_sched_tests(struct sn_context *snc)
{
	int stream, count, repeat;

	for (repeat = 0; repeat < 1000; repeat++)
		for (stream = 0; stream < 10 ; stream++)
			for (count = 0; count < 10; count++)
				sn_send_asoc_msg(snc, 64, stream, 0);
	sleep(10);
}

static int sn_do_prio_small_assert_tests(struct sn_context *snc)
{
	snc->cb[0] += snc->cb[2];

	if (!snc->cb[3]) {
		time(&snc->cb[3]);
	} else if (snc->cb[0] == 6400000) {
		long end;

		time(&end);
		printf("PERF: %lf mb/s\n", 50.0 / (end - snc->cb[3]));
	}
}

/* prio test 5 */
static int sn_do_prio_fuzz_sched_tests(struct sn_context *snc)
{
	int stream, count, repeat;

	srand((unsigned int)time(NULL));

	for (repeat = 0; repeat < 10000; repeat++) {
		for (stream = 0; stream < 10 ; stream++) {
			for (count = 0; count < 10; count++) {
				int msg_len = rand() % 1500 + 20;

				sn_send_asoc_msg(snc, msg_len, stream, 0);
			}
		}
	}
	sleep(10);
}

static int sn_do_prio_fuzz_assert_tests(struct sn_context *snc)
{
	snc->cb[0] += snc->cb[2];

	printf("sid: %d, len: %d, total_len: %d\n", snc->sri->sinfo_stream,
	       snc->cb[2], snc->cb[0]);
}

/* prio test 6 */
static int sn_do_prio_fuzzb_sched_tests(struct sn_context *snc)
{
	int stream, count, repeat;

	srand((unsigned int)time(NULL));

	for (repeat = 0; repeat < 10000; repeat++) {
		snc->sched = rand() % SN_CASE_MAX;
		sn_cases[snc->sched].do_setup_sched(snc);
		sn_cases[snc->sched].do_setup_schedvalue(snc);
		for (stream = 0; stream < 10 ; stream++) {
			for (count = 0; count < 10; count++) {
				int msg_len = rand() % 3000 + 20;

				sn_send_asoc_msg(snc, msg_len, stream, 0);
			}
		}
	}
	sleep(10);
}

/* rr test 0 */
static int sn_do_rr_setup_schedvalue(struct sn_context *snc)
{
	return 0;
}

static struct sn_case sn_cases[SN_CASE_MAX] = {
	{ /* PRIO conformance A */
		.sched = SCTP_SS_PRIO,
		.sched_cork = 1,
		.sched_sids = {3, 3, 3, 2, 2, 2, 1, 1, 1},
		.do_setup_sched = sn_do_setup_sched,
		.do_setup_schedvalue = sn_do_setup_schedvalue,
		.do_sched_tests = sn_do_sched_tests,
		.do_assert_tests = sn_do_assert_tests,
	},
	{ /* PRIO conformance B */
		.sched = SCTP_SS_PRIO,
		.sched_cork = 1,
		.sched_sids = {3, 3, 3, 1, 2, 1, 2, 1, 2},
		.do_setup_sched = sn_do_setup_sched,
		.do_setup_schedvalue = sn_do_prio_setup_schedvalue,
		.do_sched_tests = sn_do_sched_tests,
		.do_assert_tests = sn_do_assert_tests,
	},
	{ /* PRIO performance A */
		.sched = SCTP_SS_PRIO,
		.sched_cork = 0,
		.sched_sids = {0},
		.do_setup_sched = sn_do_setup_sched,
		.do_setup_schedvalue = sn_do_prio_stress_setup_schedvalue,
		.do_sched_tests = sn_do_prio_sched_tests,
		.do_assert_tests = sn_do_prio_assert_tests,
	},
	{ /* PRIO performance B */
		.sched = SCTP_SS_PRIO,
		.sched_cork = 0,
		.sched_sids = {0},
		.do_setup_sched = sn_do_setup_sched,
		.do_setup_schedvalue = sn_do_prio_stress_setup_schedvalue,
		.do_sched_tests = sn_do_prio_stress_sched_tests,
		.do_assert_tests = sn_do_prio_stress_assert_tests,
	},
	{ /* PRIO performance C */
		.sched = SCTP_SS_PRIO,
		.sched_cork = 0,
		.sched_sids = {0},
		.do_setup_sched = sn_do_setup_sched,
		.do_setup_schedvalue = sn_do_prio_stress_setup_schedvalue,
		.do_sched_tests = sn_do_prio_small_sched_tests,
		.do_assert_tests = sn_do_prio_small_assert_tests,
	},
	{ /* PRIO fuzz A */
		.sched = SCTP_SS_PRIO,
		.sched_cork = 0,
		.sched_sids = {0},
		.do_setup_sched = sn_do_setup_sched,
		.do_setup_schedvalue = sn_do_prio_stress_setup_schedvalue,
		.do_sched_tests = sn_do_prio_fuzz_sched_tests,
		.do_assert_tests = sn_do_prio_fuzz_assert_tests,
	},
	{ /* PRIO fuzz B */
		.sched = SCTP_SS_PRIO,
		.sched_cork = 0,
		.sched_sids = {0},
		.do_setup_sched = sn_do_setup_sched,
		.do_setup_schedvalue = sn_do_prio_stress_setup_schedvalue,
		.do_sched_tests = sn_do_prio_fuzzb_sched_tests,
		.do_assert_tests = sn_do_prio_fuzz_assert_tests,
	},
	{  /* SS conformance A 7 */
		.sched = SCTP_SS_RR,
		.sched_cork = 1,
		.sched_sids = {1, 2, 3, 1, 2, 3, 1, 2, 3},
		.do_setup_sched = sn_do_setup_sched,
		.do_setup_schedvalue = sn_do_rr_setup_schedvalue,
		.do_sched_tests = sn_do_sched_tests,
		.do_assert_tests = sn_do_assert_tests,
	},
	{ /* SS performance A */
		.sched = SCTP_SS_RR,
		.sched_cork = 0,
		.sched_sids = {0},
		.do_setup_sched = sn_do_setup_sched,
		.do_setup_schedvalue = sn_do_rr_setup_schedvalue,
		.do_sched_tests = sn_do_prio_sched_tests,
		.do_assert_tests = sn_do_prio_assert_tests,
	},
	{ /* B */
		.sched = SCTP_SS_RR,
		.sched_cork = 0,
		.sched_sids = {0},
		.do_setup_sched = sn_do_setup_sched,
		.do_setup_schedvalue = sn_do_prio_stress_setup_schedvalue,
		.do_sched_tests = sn_do_prio_stress_sched_tests,
		.do_assert_tests = sn_do_prio_stress_assert_tests,
	},
	{ /* C */
		.sched = SCTP_SS_RR,
		.sched_cork = 0,
		.sched_sids = {0},
		.do_setup_sched = sn_do_setup_sched,
		.do_setup_schedvalue = sn_do_prio_stress_setup_schedvalue,
		.do_sched_tests = sn_do_prio_small_sched_tests,
		.do_assert_tests = sn_do_prio_small_assert_tests,
	},
	{ /* SS fuzz A */
		.sched = SCTP_SS_RR,
		.sched_cork = 0,
		.sched_sids = {0},
		.do_setup_sched = sn_do_setup_sched,
		.do_setup_schedvalue = sn_do_prio_stress_setup_schedvalue,
		.do_sched_tests = sn_do_prio_fuzz_sched_tests,
		.do_assert_tests = sn_do_prio_fuzz_assert_tests,
	},
	{ /* SS fuzz B */
		.sched = SCTP_SS_RR,
		.sched_cork = 0,
		.sched_sids = {0},
		.do_setup_sched = sn_do_setup_sched,
		.do_setup_schedvalue = sn_do_prio_stress_setup_schedvalue,
		.do_sched_tests = sn_do_prio_fuzzb_sched_tests,
		.do_assert_tests = sn_do_prio_fuzz_assert_tests,
	},
	{ /* FCFS conformance A 13 */
		.sched = SCTP_SS_FCFS,
		.sched_cork = 1,
		.sched_sids = {1, 1, 1, 2, 2, 2, 3, 3, 3},
		.do_setup_sched = sn_do_setup_sched,
		.do_setup_schedvalue = sn_do_rr_setup_schedvalue,
		.do_sched_tests = sn_do_sched_tests,
		.do_assert_tests = sn_do_assert_tests,
	},
	{ /* FCFS perforamnce A */
		.sched = SCTP_SS_FCFS,
		.sched_cork = 0,
		.sched_sids = {0},
		.do_setup_sched = sn_do_setup_sched,
		.do_setup_schedvalue = sn_do_rr_setup_schedvalue,
		.do_sched_tests = sn_do_prio_sched_tests,
		.do_assert_tests = sn_do_prio_assert_tests,
	},
	{ /* B */
		.sched = SCTP_SS_FCFS,
		.sched_cork = 0,
		.sched_sids = {0},
		.do_setup_sched = sn_do_setup_sched,
		.do_setup_schedvalue = sn_do_prio_stress_setup_schedvalue,
		.do_sched_tests = sn_do_prio_stress_sched_tests,
		.do_assert_tests = sn_do_prio_stress_assert_tests,
	},
	{ /* C */
		.sched = SCTP_SS_FCFS,
		.sched_cork = 0,
		.sched_sids = {0},
		.do_setup_sched = sn_do_setup_sched,
		.do_setup_schedvalue = sn_do_prio_stress_setup_schedvalue,
		.do_sched_tests = sn_do_prio_small_sched_tests,
		.do_assert_tests = sn_do_prio_small_assert_tests,
	},
	{ /* FCFS fuzz A */
		.sched = SCTP_SS_FCFS,
		.sched_cork = 0,
		.sched_sids = {0},
		.do_setup_sched = sn_do_setup_sched,
		.do_setup_schedvalue = sn_do_prio_stress_setup_schedvalue,
		.do_sched_tests = sn_do_prio_fuzz_sched_tests,
		.do_assert_tests = sn_do_prio_fuzz_assert_tests,
	},
	{ /* FCFS fuzz B */
		.sched = SCTP_SS_FCFS,
		.sched_cork = 0,
		.sched_sids = {0},
		.do_setup_sched = sn_do_setup_sched,
		.do_setup_schedvalue = sn_do_prio_stress_setup_schedvalue,
		.do_sched_tests = sn_do_prio_fuzz_sched_tests,
		.do_assert_tests = sn_do_prio_fuzz_assert_tests,
	},
	{ /* FCFS perforamnce X */
		.sched = SCTP_SS_FCFS,
		.sched_cork = 0,
		.sched_sids = {0},
		.do_setup_sched = sn_do_setup_sched,
		.do_setup_schedvalue = sn_do_rr_setup_schedvalue,
		.do_sched_tests = sn_do_prio_sched_tests_x,
		.do_assert_tests = sn_do_prio_assert_tests,
	},
	{ /* FCFS perforamnce Y */
		.sched = SCTP_SS_FCFS,
		.sched_cork = 0,
		.sched_sids = {0},
		.do_setup_sched = sn_do_setup_sched,
		.do_setup_schedvalue = sn_do_rr_setup_schedvalue,
		.do_sched_tests = sn_do_prio_sched_tests_y,
		.do_assert_tests = sn_do_prio_assert_tests,
	},
};

/* setsockopt */
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

/* socket */
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

/* stream */
static int sn_close_asoc_streams(struct sn_context *snc)
{
	struct sctp_reset_streams sconfparam;

	if (!sn_cases[snc->sched].sched_cork)
		return;

	system("iptables -A INPUT -p sctp -j DROP");

	sconfparam.srs_assoc_id = snc->asoc_id;
	sconfparam.srs_flags = SCTP_STREAM_RESET_OUTGOING;
	sconfparam.srs_number_streams = 0;
	if (setsockopt(snc->fd, IPPROTO_SCTP, SCTP_RESET_STREAMS,
		       &sconfparam, sizeof(sconfparam))) {
		system("iptables -F");
		perror("setsockopt");
		exit(-1);
	}
	printf("all stream are closed\n");

	return 0;
}

static int sn_reopen_asoc_streams(struct sn_context *snc)
{
	if (!sn_cases[snc->sched].sched_cork)
		return;

	system("iptables -F");
	printf("all stream are reopened\n");
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
			    int flags)
{
	char buf[10240];

	if (sctp_sendmsg(snc->fd, (const void *)buf, msg_len, snc->serv,
			 sizeof(*snc->serv), 0, flags, sid, 0, 0) == -1) {
		perror("sendmsg");
		exit(-1);
	}

	return 0;
}

static int sn_send_peer_ack(struct sn_context *snc)
{
	sn_send_asoc_msg(snc, SN_CMD_ACK, 0, 0);
}

/* entrance */
int do_client(char **argv)
{
	struct sockaddr_storage servaddr;
	struct sn_context snc;

	sleep(3); /* wait for server */
	memset(&snc, 0, sizeof(snc));
	snc.serv = &servaddr;
	snc.sched = atoi(argv[3]);
	sn_create_asoc_sk(&snc, argv[1], argv[2]);

	sn_cases[snc.sched].do_setup_sched(&snc);
	sn_cases[snc.sched].do_setup_schedvalue(&snc);

	sn_enable_asoc_strreset(&snc);
	sn_enable_peer_strreset(&snc);
	sn_close_asoc_streams(&snc);

	sn_cases[snc.sched].do_sched_tests(&snc);

	sn_reopen_asoc_streams(&snc);
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
	snc.sched = atoi(argv[3]);
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
			sn_cases[snc.sched].do_assert_tests(&snc);
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
