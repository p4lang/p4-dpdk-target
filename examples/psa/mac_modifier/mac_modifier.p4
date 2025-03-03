/* -*- P4_16 -*- */
#include <core.p4>
#include <dpdk/psa.p4>

typedef bit<48> MacAddr;
typedef bit<32> IPv4Addr;
typedef bit<16> PortId_t;

// Headers
header ethernet_t {
    MacAddr dstAddr;
    MacAddr srcAddr;
    bit<16> etherType;
}

header ipv4_t {
    bit<4>    version;
    bit<4>    ihl;
    bit<8>    diffserv;
    bit<16>   totalLen;
    bit<16>   identification;
    bit<3>    flags;
    bit<13>   fragOffset;
    bit<8>    ttl;
    bit<8>    protocol;
    bit<16>   hdrChecksum;
    IPv4Addr  srcAddr;
    IPv4Addr  dstAddr;
}

// Parser and struct definitions
struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

struct metadata_t {
    // Empty for now
}

// Parser
parser IngressParserImpl(
    packet_in buffer,
    out headers_t headers,
    inout metadata_t meta,
    in psa_ingress_parser_input_t istd,
    in empty_t resubmit_meta,
    in empty_t recirculate_meta) {

    state start {
        buffer.extract(headers.ethernet);
        transition select(headers.ethernet.etherType) {
            0x0800: parse_ipv4;
            default: accept;
        }
    }

    state parse_ipv4 {
        buffer.extract(headers.ipv4);
        transition accept;
    }
}

parser EgressParserImpl(
    packet_in buffer,
    out headers_t headers,
    inout metadata_t meta,
    in psa_egress_parser_input_t istd,
    in empty_t normal_meta,
    in empty_t clone_i2e_meta,
    in empty_t clone_e2e_meta) {

    state start {
        buffer.extract(headers.ethernet);
        transition select(headers.ethernet.etherType) {
            0x0800: parse_ipv4;
            default: accept;
        }
    }

    state parse_ipv4 {
        buffer.extract(headers.ipv4);
        transition accept;
    }
}

// Main control block
control IngressImpl(
    inout headers_t headers,
    inout metadata_t meta,
    in    psa_ingress_input_t  istd,
    inout psa_ingress_output_t ostd) {

    // Action to modify source MAC
    action modify_src_mac(MacAddr new_src_mac) {
        headers.ethernet.srcAddr = new_src_mac;
    }

    // Action to modify destination MAC
    action modify_dst_mac(MacAddr new_dst_mac) {
        headers.ethernet.dstAddr = new_dst_mac;
    }

    // Exact match table for source MAC modification
    table exact_match_table {
        key = {
            headers.ethernet.dstAddr: exact;
            headers.ethernet.srcAddr: exact;
            headers.ipv4.protocol: exact;
        }
        actions = {
            modify_src_mac;
            NoAction;
        }
        default_action = NoAction();
        size = 1024;
    }

    // Ternary match table for destination MAC modification
    table ternary_match_table {
        key = {
            headers.ipv4.srcAddr: ternary;
            headers.ipv4.dstAddr: ternary;
            headers.ipv4.protocol: ternary;
        }
        actions = {
            modify_dst_mac;
            NoAction;
        }
        default_action = NoAction();
        size = 1024;
    }

    apply {
        if (headers.ipv4.isValid()) {
            exact_match_table.apply();
            ternary_match_table.apply();
        }
        
        // Send out packet on the same port it came in
        send_to_port(ostd, (PortId_t)istd.ingress_port);
    }
}

control EgressImpl(
    inout headers_t headers,
    inout metadata_t meta,
    in    psa_egress_input_t  istd,
    inout psa_egress_output_t ostd) {
    apply { }
}

// Deparser
control IngressDeparserImpl(
    packet_out packet,
    out empty_t clone_i2e_meta,
    out empty_t resubmit_meta,
    out empty_t normal_meta,
    inout headers_t headers,
    in metadata_t meta,
    in psa_ingress_output_t istd) {
    apply {
        packet.emit(headers.ethernet);
        packet.emit(headers.ipv4);
    }
}

control EgressDeparserImpl(
    packet_out packet,
    out empty_t clone_e2e_meta,
    out empty_t recirculate_meta,
    inout headers_t headers,
    in metadata_t meta,
    in psa_egress_output_t istd,
    in psa_egress_deparser_input_t edstd) {
    apply {
        packet.emit(headers.ethernet);
        packet.emit(headers.ipv4);
    }
}

// Instantiate the PSA switch
IngressParserImpl() ip;
IngressImpl() ingress;
IngressDeparserImpl() id;
EgressParserImpl() ep;
EgressImpl() egress;
EgressDeparserImpl() ed;

PSA_Switch(ip, ingress, id, ep, egress, ed) main;