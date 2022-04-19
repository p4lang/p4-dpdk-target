#include <port_mgr/bf_port_if.h>
#include <bf_rt/bf_rt_common.h>

enum operation{
	ADD_RULE = 0,
	DELETE_RULE = 1,
};
int p4_switchd_app_port_add(const int dev_id,
			    const char port[],
			    const char mempool[],
			    const char pipe[]);

int p4_switchd_enable_pipeline(const int dev_id);

int p4_switchd_rt_table_entry_add_or_del(const uint32_t ipaddr,
				  const uint16_t prefix,
				  const uint32_t port,
				  const int operation);
