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
         int status = p4_switchd_rt_table_entry_add_or_del((uint32_t)request->ipaddr(),
			 			    (uint16_t)request->prefix(),
						    (uint32_t)request->port(),
						    (int)request->op());
         response->set_responsem(status);
         return Status::OK;
     }
}
}
