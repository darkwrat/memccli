#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include <libmemcached/memcached.h>

struct cli_server {
	char *host;
	in_port_t port;
	char *user;
	char *pass;
	uint64_t snd_timeout_us;
	uint64_t rcv_timeout_us;
};

#define CLI_REQ_FLAG_ADD 0x1
#define CLI_REQ_FLAG_DEL 0x2
#define CLI_REQ_FLAG_MAX 0x4

struct cli_request {
	uint32_t cli_flags;
	char *key;
	char *value;
	uint32_t flags;
	time_t expire;
};

// clang-format off
static const struct option longopts[] = {
    {"host",           required_argument, 0, 'h'},
    {"port",           required_argument, 0, 'P'},
    {"snd-timeout-us", required_argument, 0, 'r'},
    {"rcv-timeout-us", required_argument, 0, 'w'},
    {"key",            required_argument, 0, 'k'},
    {"value",          required_argument, 0, 'v'},
    {"add",            no_argument,       0, 'a'},
    {"del",            no_argument,       0, 'd'},
    {"user",           required_argument, 0, 'u'},
    {"pass",           required_argument, 0, 'p'},
    {"flags",          required_argument, 0, 'f'},
    {"expire",         required_argument, 0, 'e'},
    { 0 },
};
// clang-format on

__attribute__((noreturn)) void cli_usage(void);

int cli_execute(memcached_st *m, struct cli_request req);

int cli_get(memcached_st *m, struct cli_request req);
int cli_set(memcached_st *m, struct cli_request req);
int cli_add(memcached_st *m, struct cli_request req);
int cli_del(memcached_st *m, struct cli_request req);

void cli_server_dtor(struct cli_server *p);
void cli_request_dtor(struct cli_request *p);
void cli_memcached_server_list_dtor(memcached_server_list_st *p);
void cli_memcached_dtor(memcached_st **p);
void cli_free_dtor(void **p);

int main(int argc, char *argv[])
{
	struct cli_server serv __attribute__((cleanup(cli_server_dtor))) = {0};
	struct cli_request req __attribute__((cleanup(cli_request_dtor))) = {0};

	int option_index;

	while (1) {
		int c =
		    getopt_long(argc, argv, "h:P:w:r:k:v:adu:p:f:e:", longopts,
				&option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'h':
			serv.host = strdup(optarg);
			break;
		case 'P':
			serv.port = atoi(optarg);
			break;
		case 'w':
			serv.snd_timeout_us = atol(optarg);
			break;
		case 'r':
			serv.rcv_timeout_us = atol(optarg);
			break;
		case 'k':
			req.key = strdup(optarg);
			break;
		case 'v':
			req.value = strdup(optarg);
			break;
		case 'a':
			req.cli_flags |= CLI_REQ_FLAG_ADD;
			break;
		case 'd':
			req.cli_flags |= CLI_REQ_FLAG_DEL;
			break;
		case 'u':
			serv.user = strdup(optarg);
			break;
		case 'p':
			serv.pass = strdup(optarg);
			break;
		case 'f':
			req.flags = atol(optarg);
			break;
		case 'e':
			req.expire = atol(optarg);
			break;
		case '?':
			cli_usage();
			/* unreachable */
		default:
			fprintf(stderr, "getopt returned character code `%d'\n",
				(int)c);
			cli_usage();
			/* unreachable */
		}
	}

	if (optind < argc) {
		printf("non-option ARGV-elements: ");
		while (optind < argc)
			printf("%s ", argv[optind++]);

		printf("%s", "\n");
		cli_usage();
		/* unreachable */
	}

	if (!req.key) {
		fprintf(stderr, "missing `--key' option\n");
		cli_usage();
		/* unreachable */
	}

	memcached_st *m __attribute__((cleanup(cli_memcached_dtor))) =
	    memcached_create(NULL);

	memcached_return r;
	memcached_server_list_st
	    __attribute__((cleanup(cli_memcached_server_list_dtor))) sl =
		memcached_server_list_append(NULL, serv.host, serv.port, &r);

	if (r != MEMCACHED_SUCCESS) {
		fprintf(stderr, "memcached_server_list_append failed: %s\n",
			memcached_strerror(m, r));
		return EX_SOFTWARE;
	}

	if (serv.user) {
		r = memcached_set_sasl_auth_data(m, serv.user, serv.pass);
		if (r != MEMCACHED_SUCCESS) {
			fprintf(stderr,
				"memcached_set_sasl_auth_data failed: %s\n",
				memcached_strerror(m, r));
			return EX_SOFTWARE;
		}
	}

	if (serv.snd_timeout_us <= 0)
		serv.snd_timeout_us = 1000000; /*1s*/
	if (serv.rcv_timeout_us <= 0)
		serv.rcv_timeout_us = 1000000; /*1s*/

	uint64_t flags[MEMCACHED_BEHAVIOR_MAX] = {
	    [MEMCACHED_BEHAVIOR_SND_TIMEOUT] = serv.snd_timeout_us,
	    [MEMCACHED_BEHAVIOR_RCV_TIMEOUT] = serv.rcv_timeout_us,
	    [MEMCACHED_BEHAVIOR_TCP_NODELAY] = 1,
	    [MEMCACHED_BEHAVIOR_BINARY_PROTOCOL] = 1,
	};

	for (memcached_behavior_t f = 0; f < MEMCACHED_BEHAVIOR_MAX; f++) {
		if (!flags[f])
			continue;

		r = memcached_behavior_set(m, f, flags[f]);
		if (r != MEMCACHED_SUCCESS) {
			fprintf(stderr,
				"memcached_behavior_set flag %d data %zu: %s\n",
				f, flags[f], memcached_strerror(m, r));
			return EX_SOFTWARE;
		}
	}

	r = memcached_server_push(m, sl);
	if (r != MEMCACHED_SUCCESS) {
		fprintf(stderr, "memcached_server_push failed: %s\n",
			memcached_strerror(m, r));
		return EX_NOHOST;
	}

	return cli_execute(m, req);
}

int cli_execute(memcached_st *m, struct cli_request req)
{
	bool del = req.cli_flags & CLI_REQ_FLAG_DEL;
	bool add = req.cli_flags & CLI_REQ_FLAG_ADD;

	if (add && req.value)
		return cli_add(m, req);
	if (del)
		return cli_del(m, req);
	if (req.value)
		return cli_set(m, req);

	return cli_get(m, req);
}

int cli_get(memcached_st *m, struct cli_request req)
{

	size_t vlen;
	uint32_t flags;
	memcached_return r;

	void *v __attribute__((cleanup(cli_free_dtor))) =
	    memcached_get(m, req.key, strlen(req.key), &vlen, &flags, &r);
	if (r != MEMCACHED_SUCCESS) {
		fprintf(stderr, "memcached_get failed: %s\n",
			memcached_strerror(m, r));
		return -r + EX__BASE;
	}

	fprintf(stdout, "%.*s", (int)vlen, (char *)v);

	return EX_OK;
}

int cli_set(memcached_st *m, struct cli_request req)
{
	memcached_return r =
	    memcached_set(m, req.key, strlen(req.key), req.value,
			  strlen(req.value), req.expire, req.flags);
	if (r != MEMCACHED_SUCCESS) {
		fprintf(stderr, "memcached_set failed: %s\n",
			memcached_strerror(m, r));
		return -r + EX__BASE;
	}

	return EX_OK;
}

int cli_add(memcached_st *m, struct cli_request req)
{
	memcached_return r =
	    memcached_add(m, req.key, strlen(req.key), req.value,
			  strlen(req.value), req.expire, req.flags);
	if (r != MEMCACHED_SUCCESS) {
		fprintf(stderr, "memcached_add failed: %s\n",
			memcached_strerror(m, r));
		return -r + EX__BASE;
	}

	return EX_OK;
}

int cli_del(memcached_st *m, struct cli_request req)
{
	memcached_return r =
	    memcached_delete(m, req.key, strlen(req.key), req.expire);
	if (r != MEMCACHED_SUCCESS) {
		fprintf(stderr, "memcached_delete failed: %s\n",
			memcached_strerror(m, r));
		return -r + EX__BASE;
	}

	return EX_OK;
}

__attribute__((noreturn)) void cli_usage(void)
{
	fprintf(stderr, "Supported options:\n");

	const struct option *opt;
	for (opt = &longopts[0]; opt->name; opt++) {
		char *arg = "";
		if (opt->has_arg == required_argument) {
			arg = " <val>";
		} else if (opt->has_arg == optional_argument) {
			arg = " [val]";
		}

		fprintf(stderr, "	--%s%s, -%c%s\n", opt->name, arg,
			(char)opt->val, arg);
	}

	exit(EX_USAGE);
}

void cli_server_dtor(struct cli_server *p)
{
	free(p->host);
	free(p->user);
	free(p->pass);
}

void cli_request_dtor(struct cli_request *p)
{
	free(p->key);
	free(p->value);
}

void cli_memcached_server_list_dtor(memcached_server_list_st *p)
{
	memcached_server_list_free(*p);
}

void cli_memcached_dtor(memcached_st **p)
{
	(void)memcached_destroy_sasl_auth_data(*p);
	memcached_free(*p);
}

void cli_free_dtor(void **p) { free(*p); }
