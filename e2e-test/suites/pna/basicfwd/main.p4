#include <core.p4>
#include <pna.p4>

header eth_h {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}

struct header_t {
    eth_h eth;
}

struct metadata_t {}

parser MainParser(
    packet_in pkt,
    out header_t hdr,
    inout metadata_t meta,
    in pna_main_parser_input_metadata_t istd
) {
    state start {
        pkt.extract(hdr.eth);
        transition accept;
    }
}

control PreControl(
    in header_t hdr,
    inout metadata_t meta,
    in pna_pre_input_metadata_t istd,
    inout pna_pre_output_metadata_t ostd
) {
    apply {}
}

control MainControl(
    inout header_t hdr,
    inout metadata_t meta,
    in pna_main_input_metadata_t istd,
    inout pna_main_output_metadata_t ostd
) {
    apply {
        switch (istd.input_port) {
            (PortId_t) 0: { send_to_port((PortId_t) 1); }
            (PortId_t) 1: { send_to_port((PortId_t) 0); }
            (PortId_t) 2: { send_to_port((PortId_t) 3); }
            (PortId_t) 3: { send_to_port((PortId_t) 2); }
        }
    }
}

control MainDeparser(
    packet_out pkt,
    in header_t hdr,
    in metadata_t meta,
    in pna_main_output_metadata_t ostd
) {
    apply {
        pkt.emit(hdr);
    }
}

PNA_NIC(
    MainParser(),
    PreControl(),
    MainControl(),
    MainDeparser()
) main;
