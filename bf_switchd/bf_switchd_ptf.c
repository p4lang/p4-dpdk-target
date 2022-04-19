/*
 * Copyright(c) 2021 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * bf_switchd_ptf.c
 */

/* Standard includes */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <bf_pal/bf_pal_port_intf.h>
#include <ptf_grpc/switchd_app.h>
#include <bf_rt/bf_rt_init.h>
#include <bf_rt/bf_rt_info.h>
#include <bf_rt/bf_rt_table.h>
#include <bf_rt/bf_rt_session.h>
#include <bf_rt/bf_rt_table_key.h>
#include <bf_rt/bf_rt_table_data.h>

#define MTU 1500

/**********************************************************
 * ptf test to add new ports.
 **********************************************************/
int p4_switchd_app_port_add(const int dev_id,
			    const char port[],
			    const char mempool[],
			    const char pipe[])
{
	int status = 0;
	struct port_attributes_t *port_attrib = (struct port_attributes_t *)malloc(sizeof(struct port_attributes_t));

	if(port_attrib == NULL)
	{
		return BF_NO_SYS_RESOURCES;
	}
	sprintf(port_attrib->port_name, "%s%d", port, dev_id);
	sprintf(port_attrib->mempool_name, "%s", mempool);
	sprintf(port_attrib->pipe_in, "%s", pipe);
	sprintf(port_attrib->pipe_out, "%s", pipe);
	port_attrib->port_type = BF_DPDK_TAP;
	port_attrib->port_dir = 0; //PM_PORT_DIR_DEFAULT
	port_attrib->port_in_id = dev_id;
	port_attrib->port_out_id = dev_id;
	port_attrib->tap.mtu = MTU;

	status = bf_pal_port_add(dev_id, dev_id, port_attrib);
	free(port_attrib);

	return status;
}

/**********************************************************
 * ptf test to enable pipeline.
 **********************************************************/
int p4_switchd_enable_pipeline(const int dev_id)
{
	int status = 0;

	status = bf_rt_enable_pipeline(dev_id);

	return status;
}

/**********************************************************
 * ptf test add/delete entry in table.
 **********************************************************/
int p4_switchd_rt_table_entry_add_or_del(const struct entry_request *request)
{
	bf_status_t status = 0;
	bf_rt_info_hdl *bfrt_info_hdl = (bf_rt_info_hdl *)malloc(sizeof(bf_rt_info_hdl));

	const bf_rt_table_hdl *table_hdl = (bf_rt_table_hdl *)malloc(sizeof(table_hdl));

	if (bfrt_info_hdl == NULL || table_hdl == NULL)
		return BF_NO_SYS_RESOURCES;

	bf_rt_table_key_hdl *key_hdl;
	bf_rt_table_data_hdl *data_hdl;
	bf_rt_session_hdl *session;
	bf_rt_target_t dev_tgt;

	dev_tgt.dev_id = 0;
	dev_tgt.pipe_id = 0;
	dev_tgt.direction = 0xFF;
	dev_tgt.prsr_id = 0xFF;

	const bf_rt_id_t field_id = request->field_id;
	const bf_rt_id_t action_id = request->action_id;
	const bf_rt_id_t data_field_id = request->data_field;

	uint32_t port_nw = (uint32_t)htonl((uint32_t)request->port);

	// create a session
	status = bf_rt_session_create(&session);

	// get bfrt info hdl
	status = bf_rt_info_get(dev_tgt.dev_id,
				request->p4_name,
				(const bf_rt_info_hdl **)&bfrt_info_hdl);

	// get table hdl
	status = bf_rt_table_from_name_get(bfrt_info_hdl,
					   request->table_name,
					   &table_hdl);

	// allocate key hdl
	status = bf_rt_table_key_allocate(table_hdl, &key_hdl);

	switch (request->prog_name) {
	//this case will run for simple_l2l3_lpm example to fill key hdl
	case SIMPLE_L2L3_LPM:
		status = bf_rt_key_field_set_value_lpm(key_hdl,
						       field_id,
						       request->ipaddr,
						       request->prefix);
		break;

	//this case will run for simple_l2l3_wcm example to fill key hdl
	case SIMPLE_L2L3_WCM:
		status = bf_rt_key_field_set_value_and_mask(key_hdl,
						    field_id,
						    request->ipaddr,
						    request->mask);
		break;
	
	default:
		printf("Error: Unsupported p4_program.");
		return BF_NOT_SUPPORTED;
	}
	
	switch (request->op) {
	//this case will run when cleanup config executed
	case DELETE_RULE:
		status = bf_rt_table_entry_del(table_hdl,
					       session,
					       &dev_tgt,
					       key_hdl);

		break;

	//this case will run when entry addition config executed
	case ADD_RULE:
	// allocate data hdl
		status = bf_rt_table_action_data_allocate(table_hdl,
							  action_id,
							  &data_hdl);

		//fill data hdl
		status = bf_rt_data_field_set_value_ptr(data_hdl,
							data_field_id,
							(const uint8_t *)&port_nw,
							sizeof(uint32_t));

		// create table entry
		status = bf_rt_table_entry_add(table_hdl,
					       session,
					       &dev_tgt,
					       key_hdl,
					       data_hdl);

		// deallocate data hdl
		status = bf_rt_table_data_deallocate(data_hdl);

		break;

	default:
		printf("Error: Unsupported operation only add/del are valid");
		return BF_NOT_SUPPORTED;
	}

	//Deallocate key hdl
	status = bf_rt_table_key_deallocate(key_hdl);

	//Destroy the session
	status = bf_rt_session_destroy(session);

	return status;
}
/**********************************************************
 * end ptf test code.
 **********************************************************/
