/**
 * This file contains functions which are called by ptf gRPC service
 **/

#include "ptf_server_impl.hpp"

extern "C" {
    #include <ptf_grpc/switchd_app.h>
}

namespace ptf{
    namespace server{

    Status ptfServiceImpl::port_add(ServerContext * context,
		    		    const ptf_proto::switchd_app_int_t *request,
				    ptf_proto::response *response)
    {
        int status = p4_switchd_app_port_add(request->dev_id(),
					     request->port_name().c_str(),
					     request->mempool_name().c_str(),
					     request->pipe_name().c_str());
        response->set_responsem(status);
        return Status::OK;
    }

     Status ptfServiceImpl::enable_pipeline(ServerContext * context,
		     			    const ptf_proto::device_id *request,
					    ptf_proto::response *response)
     {
         int status = p4_switchd_enable_pipeline(request->dev_id());
         response->set_responsem(status);
         return Status::OK;
     }
     Status ptfServiceImpl::add_or_del_table_entry(ServerContext * context,
		     			    const ptf_proto::table *request,
					    ptf_proto::response *response)
     {
	 struct entry_request *e_request = (struct entry_request *)malloc(sizeof(struct entry_request));
	 if(e_request == NULL)
	 {
		 int status = BF_NO_SYS_RESOURCES;
		 response->set_responsem(status);
		 return Status::OK;
	 }

	 e_request->ipaddr = (uint32_t)request->ipaddr();
	 e_request->prefix = (uint32_t)request->prefix();
         e_request->port = (uint32_t)request->port();
         e_request->op = (int)request->op();
         e_request->p4_name = request->p4_name().c_str();
         e_request->table_name = request->table_name().c_str();
         e_request->field_id = request->field_id();
         e_request->action_id = request->action_id();
	 e_request->data_field = request->data_field();
	 e_request->prog_name = request->pn();

         int status = p4_switchd_rt_table_entry_add_or_del(e_request);
         response->set_responsem(status);
         return Status::OK;
     }
     Status ptfServiceImpl::wcm_add_or_del_table_entry(ServerContext *context,
						       const ptf_proto::wcm_table *request,
						       ptf_proto::response *response)
     {
	 struct entry_request *e_request = (struct entry_request *)malloc(sizeof(struct entry_request));
	 if(e_request == NULL)
	 {
		 int status = BF_NO_SYS_RESOURCES;
		 response->set_responsem(status);
		 return Status::OK;
	 }

	 e_request->ipaddr = (uint32_t)request->ipaddr();
	 e_request->mask = (uint32_t)request->mask();
         e_request->port = (uint32_t)request->port();
         e_request->op = (int)request->op();
         e_request->p4_name = request->p4_name().c_str();
         e_request->table_name = request->table_name().c_str();
         e_request->field_id = request->field_id();
         e_request->action_id = request->action_id();
	 e_request->data_field = request->data_field();
	 e_request->prog_name = request->pn();

	 int status = p4_switchd_rt_table_entry_add_or_del(e_request);
	 response->set_responsem(status);
	 return Status::OK;
     }
}
}
