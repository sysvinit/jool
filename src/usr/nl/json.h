#ifndef SRC_USR_NL_JSON_H_
#define SRC_USR_NL_JSON_H_

#include "common/config.h"
#include "usr/nl/core.h"

struct jool_result joolnl_json_parse(
	struct joolnl_socket *sk,
	xlator_type xt,
	char const *iname,
	char const *file_name,
	bool force
);

struct jool_result joolnl_json_get_iname(
	char const *file_name,
	char **out
);

#endif /* SRC_USR_NL_JSON_H_ */
