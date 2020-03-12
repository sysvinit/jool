#include "usr/nl/eamt.h"

#include <errno.h>
#include <netlink/genl/genl.h>
#include "usr/nl/attribute.h"
#include "usr/nl/common.h"

struct foreach_args {
	joolnl_eamt_foreach_cb cb;
	void *args;
	bool done;
	struct eamt_entry last;
};

static struct jool_result handle_foreach_response(struct nl_msg *response,
		void *arg)
{
	struct foreach_args *args = arg;
	struct nlattr *attr;
	int rem;
	struct eamt_entry entry;
	struct jool_result result;

	result = joolnl_init_foreach(response, "eam", &args->done);
	if (result.error)
		return result;

	foreach_entry(attr, genlmsg_hdr(nlmsg_hdr(response)), rem) {
		result = nla_get_eam(attr, &entry);
		if (result.error)
			return result;

		result = args->cb(&entry, args->args);
		if (result.error)
			return result;

		memcpy(&args->last, &entry, sizeof(entry));
	}

	return result_success();
}

struct jool_result joolnl_eamt_foreach(struct joolnl_socket *sk,
		char const *iname, joolnl_eamt_foreach_cb cb, void *_args)
{
	struct nl_msg *msg;
	struct foreach_args args;
	struct jool_result result;
	bool first_request;

	args.cb = cb;
	args.args = _args;
	args.done = false;
	memset(&args.last, 0, sizeof(args.last));
	first_request = true;

	do {
		result = joolnl_alloc_msg(sk, iname, JOP_EAMT_FOREACH, 0, &msg);
		if (result.error)
			return result;

		if (first_request) {
			first_request = false;

		} else if (nla_put_eam(msg, RA_OFFSET, &args.last) < 0) {
			nlmsg_free(msg);
			return joolnl_err_msgsize();
		}

		result = joolnl_request(sk, msg, handle_foreach_response, &args);
		if (result.error)
			return result;
	} while (!args.done);

	return result_success();
}

static struct jool_result __update(struct joolnl_socket *sk, char const *iname,
		enum jool_operation operation,
		struct ipv6_prefix const *p6, struct ipv4_prefix const *p4,
		__u8 flags)
{
	struct nl_msg *msg;
	struct nlattr *root;
	struct jool_result result;

	result = joolnl_alloc_msg(sk, iname, operation, flags, &msg);
	if (result.error)
		return result;

	if (p6 || p4) {
		root = nla_nest_start(msg, RA_OPERAND);
		if (!root)
			goto nla_put_failure;

		if (p6 && nla_put_prefix6(msg, EA_PREFIX6, p6) < 0)
			goto nla_put_failure;
		if (p4 && nla_put_prefix4(msg, EA_PREFIX4, p4) < 0)
			goto nla_put_failure;

		nla_nest_end(msg, root);
	}

	return joolnl_request(sk, msg, NULL, NULL);

nla_put_failure:
	nlmsg_free(msg);
	return joolnl_err_msgsize();
}

struct jool_result joolnl_eamt_add(struct joolnl_socket *sk, char const *iname,
		struct ipv6_prefix const *p6, struct ipv4_prefix const *p4,
		bool force)
{
	return __update(sk, iname, JOP_EAMT_ADD, p6, p4, force ? HDRFLAGS_FORCE : 0);
}

struct jool_result joolnl_eamt_rm(struct joolnl_socket *sk, char const *iname,
		struct ipv6_prefix const *p6, struct ipv4_prefix const *p4)
{
	return __update(sk, iname, JOP_EAMT_RM, p6, p4, 0);
}

struct jool_result joolnl_eamt_flush(struct joolnl_socket *sk, char const *iname)
{
	return __update(sk, iname, JOP_EAMT_FLUSH, NULL, NULL, 0);
}
