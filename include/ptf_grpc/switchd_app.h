#include <port_mgr/bf_port_if.h>
#include <bf_rt/bf_rt_common.h>

struct entry_request{
	uint32_t ipaddr;
        uint16_t prefix;
        uint32_t mask;
        uint32_t port;
        int op;
        const char * p4_name;
        const char * table_name;
        int field_id;
        int action_id;
        int data_field;
	int prog_name;
};

enum p4Name{
	SIMPLE_L2L3_LPM = 0,
	SIMPLE_L2L3_WCM = 1,
};

enum operation{
	ADD_RULE = 0,
	DELETE_RULE = 1,
};

int p4_switchd_app_port_add(const int dev_id,
			    const char port[],
			    const char mempool[],
			    const char pipe[]);

int p4_switchd_enable_pipeline(const int dev_id);

int p4_switchd_rt_table_entry_add_or_del(const struct entry_request *request);
