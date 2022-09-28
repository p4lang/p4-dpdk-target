
struct ethernet_h {
	bit<48> dst_addr
	bit<48> src_addr
	bit<16> ether_type
}

struct vlan_tag_h {
	bit<16> pcp_cfi_vid
	bit<16> ether_type
}

struct ipv4_h {
	bit<8> version_ihl
	bit<8> diffserv
	bit<16> total_len
	bit<16> identification
	bit<16> flags_frag_offset
	bit<8> ttl
	bit<8> protocol
	bit<16> hdr_checksum
	bit<32> src_addr
	bit<32> dst_addr
}

struct send_arg_t {
	bit<32> port
}

struct set_port_and_src_mac_arg_t {
	bit<32> port
	bit<48> src_mac
	bit<48> dst_mac
}

header ethernet instanceof ethernet_h
header vlan_tag instanceof vlan_tag_h
header ipv4 instanceof ipv4_h

struct my_ingress_metadata_t {
	bit<32> psa_ingress_parser_input_metadata_ingress_port
	bit<32> psa_ingress_parser_input_metadata_packet_path
	bit<32> psa_egress_parser_input_metadata_egress_port
	bit<32> psa_egress_parser_input_metadata_packet_path
	bit<32> psa_ingress_input_metadata_ingress_port
	bit<32> psa_ingress_input_metadata_packet_path
	bit<64> psa_ingress_input_metadata_ingress_timestamp
	bit<8> psa_ingress_input_metadata_parser_error
	bit<8> psa_ingress_output_metadata_class_of_service
	bit<8> psa_ingress_output_metadata_clone
	bit<16> psa_ingress_output_metadata_clone_session_id
	bit<8> psa_ingress_output_metadata_drop
	bit<8> psa_ingress_output_metadata_resubmit
	bit<32> psa_ingress_output_metadata_multicast_group
	bit<32> psa_ingress_output_metadata_egress_port
	bit<8> psa_egress_input_metadata_class_of_service
	bit<32> psa_egress_input_metadata_egress_port
	bit<32> psa_egress_input_metadata_packet_path
	bit<16> psa_egress_input_metadata_instance
	bit<64> psa_egress_input_metadata_egress_timestamp
	bit<8> psa_egress_input_metadata_parser_error
	bit<32> psa_egress_deparser_input_metadata_egress_port
	bit<8> psa_egress_output_metadata_clone
	bit<16> psa_egress_output_metadata_clone_session_id
	bit<8> psa_egress_output_metadata_drop
}
metadata instanceof my_ingress_metadata_t

struct psa_ingress_output_metadata_t {
	bit<8> class_of_service
	bit<8> clone
	bit<16> clone_session_id
	bit<8> drop
	bit<8> resubmit
	bit<32> multicast_group
	bit<32> egress_port
}

struct psa_egress_output_metadata_t {
	bit<8> clone
	bit<16> clone_session_id
	bit<8> drop
}

struct psa_egress_deparser_input_metadata_t {
	bit<32> egress_port
}

action NoAction args none {
	return
}

action send args instanceof send_arg_t {
	mov m.psa_ingress_output_metadata_egress_port t.port
	return
}

action drop args none {
	mov m.psa_ingress_output_metadata_drop 1
	return
}

action set_port_and_src_mac args instanceof set_port_and_src_mac_arg_t {
	mov m.psa_ingress_output_metadata_egress_port t.port
	mov h.ethernet.src_addr t.src_mac
	mov h.ethernet.dst_addr t.dst_mac
	return
}

table mymac {
	key {
		h.ethernet.dst_addr exact
	}
	actions {
		NoAction
	}
	default_action NoAction args none 
	size 0x10000
}


table l2_fwd {
	key {
		h.ethernet.dst_addr exact
	}
	actions {
		send
		drop
		NoAction
	}
	default_action NoAction args none 
	size 0x10000
}


table ipv4_host {
	key {
		h.ipv4.dst_addr exact
	}
	actions {
		send
		drop
		NoAction
	}
	default_action NoAction args none 
	size 0x10000
}


table ipv4_lpm {
	key {
		h.ipv4.dst_addr lpm
	}
	actions {
		set_port_and_src_mac
		send
		drop
	}
	default_action drop args none 
	size 0x10000
}


apply {
	rx m.psa_ingress_input_metadata_ingress_port
	mov m.psa_ingress_output_metadata_drop 0x0
	extract h.ethernet
	jmpeq INGRESS_PARSER_PARSE_VLAN_TAG h.ethernet.ether_type 0x8100
	jmpeq INGRESS_PARSER_PARSE_IPV4 h.ethernet.ether_type 0x800
	jmp INGRESS_PARSER_ACCEPT
	INGRESS_PARSER_PARSE_VLAN_TAG :	extract h.vlan_tag
	jmpeq INGRESS_PARSER_PARSE_IPV4 h.vlan_tag.ether_type 0x800
	jmp INGRESS_PARSER_ACCEPT
	INGRESS_PARSER_PARSE_IPV4 :	extract h.ipv4
	INGRESS_PARSER_ACCEPT :	jmpnv LABEL_0FALSE h.ethernet
	table mymac
	jmpnv LABEL_0END h.ipv4
	table ipv4_host
	table ipv4_lpm
	jmp LABEL_0END
	jmp LABEL_0END
	LABEL_0FALSE :	table l2_fwd
	LABEL_0END :	jmpneq LABEL_DROP m.psa_ingress_output_metadata_drop 0x0
	emit h.ethernet
	emit h.vlan_tag
	emit h.ipv4
	tx m.psa_ingress_output_metadata_egress_port
	LABEL_DROP : drop
}


