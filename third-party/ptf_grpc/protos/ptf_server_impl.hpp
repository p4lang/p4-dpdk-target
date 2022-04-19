#include <ptfRpc.grpc.pb.h>

namespace ptf{
    namespace server{
        using grpc::Server;
        using grpc::ServerContext;
        using grpc::Status;
        using grpc::StatusCode;

        //This class overrides the function generated by protoc cpp out
        class ptfServiceImpl : public ptf_proto::switchd_application::Service{
            public:
                Status port_add(ServerContext *context,
				const ptf_proto::switchd_app_int_t *request,
				ptf_proto::response* response) override;

                Status enable_pipeline(ServerContext *context,
				       const ptf_proto::device_id *request,
				       ptf_proto::response *response) override;

                Status add_or_del_table_entry(ServerContext *context,
				       const ptf_proto::table *request,
				       ptf_proto::response *response) override;
        };
    }
}
