#ifndef MIRROR_VALUELOOKUP_DEMO_P4
#define MIRROR_VALUELOOKUP_DEMO_P4

#include <core.p4>
#include <pna.p4>

#define HOST_TO_NET 1w1
#define NET_TO_HOST 1w0

#define RxPkt(meta) (meta.common.direction == NET_TO_HOST)
#define TxPkt(meta) (meta.common.direction == HOST_TO_NET)

extern ExactMatchValueLookupTable<K, V, E> {
   ExactMatchValueLookupTable(int size, E const_entries, V default_value);
   V lookup(in K key);
}

typedef bit<48>  EthernetAddress;

header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t ipv4;
}

struct user_metadata_t {
}

parser MainParserImpl(
    packet_in pkt,
    out   headers_t hdr,
    inout user_metadata_t main_meta,
    in    pna_main_parser_input_metadata_t istd)
{
    state start {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            0x0800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition select (hdr.ipv4.protocol) {
            default: accept;
        }
    }
}


control PreControlImpl(
    in    headers_t hdr,
    inout user_metadata_t meta,
    in    pna_pre_input_metadata_t  istd,
    inout pna_pre_output_metadata_t ostd)
{
    apply { }
}

struct dpdk_mirror_profile_cfg_t {
	bit<32> trunc_size;
	bit<32> store_port;
}

control my_control(inout headers_t hdrs,
		inout user_metadata_t user_meta,
		in pna_main_input_metadata_t istd,
		inout pna_main_output_metadata_t ostd)
{
        @mvlt_type("mirror_profile")
	MatchValueLookupTable<bit<8>, dpdk_mirror_profile_cfg_t, _>(
		size = 256,
        const_entries = { {1, {2,3}},
                          {2, {3,4}}},
        default_value = {0,0}
	) mir_prof;

	action drop()
	{
		drop_packet();
	}

	action send(PortId_t port){
		send_to_port(port);
	}

	action mirror_and_send(PortId_t port, MirrorSessionId_t mirror_session_id) {
		mirror_packet((MirrorSlotId_t)3, (MirrorSessionId_t)mirror_session_id);
		send_to_port(port);
	}

	/*
	 * L2 forwarding table based on destination MAC.
	 */
	table fwd {
		key = {
			hdrs.ethernet.dstAddr : exact;
			hdrs.ethernet.srcAddr : exact;
		}

		actions = {
			drop;send;mirror_and_send;

		}
	}
	apply {
                if (hdrs.ethernet.isValid()) {
			/*
			 * Forward based on the sole MAC.
			 * Forward either to local vport or externally.
			 */
			fwd.apply();
		}
	}
}

control MainDeparserImpl(
    packet_out pkt,
    in    headers_t  main_hdr,
    in    user_metadata_t main_user_meta,
    in    pna_main_output_metadata_t ostd) {
  apply{}
}

PNA_NIC(MainParserImpl(), PreControlImpl(), my_control(), MainDeparserImpl()) main;

#endif
