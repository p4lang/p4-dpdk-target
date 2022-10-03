
struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct ipv4_t {
	bit<8> version_ihl
	bit<8> diffserv
	bit<16> totalLength
	bit<16> identification
	bit<16> flags_fragOffset
	bit<8> ttl
	bit<8> protocol
	bit<16> hdrChecksum
	bit<32> srcAddr
	bit<32> dstAddr
}

struct tcp_t {
	bit<16> srcPort
	bit<16> dstPort
	bit<32> seqNo
	bit<32> ackNo
	bit<8> dataOffset_res
	bit<8> flags
	bit<16> window
	bit<16> checksum
	bit<16> urgentPtr
}

struct send_arg_t {
	bit<32> port
}

struct metadata_t {
	bit<32> pna_main_input_metadata_direction
	bit<32> pna_main_input_metadata_input_port
	bit<32> pna_main_output_metadata_output_port
	bit<8> MainControlT_do_add_on_miss
	bit<8> MainControlT_update_aging_info
	bit<8> MainControlT_update_expire_time
	bit<8> MainControlT_new_expire_time_profile_id
	bit<32> MainControlT_key
	bit<32> MainControlT_key_0
	bit<8> MainControlImpl_ct_tcp_table_ipv4_protocol
	bit<16> MainControlT_key_1
	bit<16> MainControlT_key_2
}
metadata instanceof metadata_t

header eth instanceof ethernet_t
header ipv4 instanceof ipv4_t
header tcp instanceof tcp_t

regarray direction size 0x100 initval 0

action NoAction args none {
	return
}

action drop args none {
	drop
	return
}

action tcp_syn_packet args none {
	mov m.MainControlT_do_add_on_miss 1
	mov m.MainControlT_update_aging_info 1
	mov m.MainControlT_update_expire_time 1
	mov m.MainControlT_new_expire_time_profile_id 0x1
	return
}

action tcp_fin_or_rst_packet args none {
	mov m.MainControlT_update_aging_info 1
	mov m.MainControlT_update_expire_time 1
	mov m.MainControlT_new_expire_time_profile_id 0x0
	return
}

action tcp_other_packets args none {
	mov m.MainControlT_update_aging_info 1
	mov m.MainControlT_update_expire_time 1
	mov m.MainControlT_new_expire_time_profile_id 0x2
	return
}

action ct_tcp_table_hit args none {
	jmpneq LABEL_END_4 m.MainControlT_update_aging_info 0x1
	jmpneq LABEL_FALSE_1 m.MainControlT_update_expire_time 0x1
	rearm m.MainControlT_new_expire_time_profile_id
	jmp LABEL_END_4
	LABEL_FALSE_1 :	rearm
	LABEL_END_4 :	return
}

action ct_tcp_table_miss args none {
	jmpneq LABEL_FALSE_2 m.MainControlT_do_add_on_miss 0x1
	learn ct_tcp_table_hit m.MainControlT_new_expire_time_profile_id
	jmp LABEL_END_6
	LABEL_FALSE_2 :	drop
	LABEL_END_6 :	return
}

action send args instanceof send_arg_t {
	mov m.pna_main_output_metadata_output_port t.port
	return
}

table set_ct_options {
	key {
		h.tcp.flags wildcard
	}
	actions {
		tcp_syn_packet
		tcp_fin_or_rst_packet
		tcp_other_packets
	}
	default_action tcp_other_packets args none const
	size 0x10000
}


table ip_fwd_table {
	key {
		h.ipv4.srcAddr exact
		h.ipv4.dstAddr exact
	}
	actions {
		send
		drop
		NoAction
	}
	default_action NoAction args none const
	size 0x10000
}


learner ct_tcp_table {
	key {
		m.MainControlT_key
		m.MainControlT_key_0
		m.MainControlImpl_ct_tcp_table_ipv4_protocol
		m.MainControlT_key_1
		m.MainControlT_key_2
	}
	actions {
		ct_tcp_table_hit @tableonly
		ct_tcp_table_miss @defaultonly
	}
	default_action ct_tcp_table_miss args none 
	size 65536
	timeout {
		60
		120
		180
		}
}

apply {
	rx m.pna_main_input_metadata_input_port
	extract h.eth
	jmpeq MAINPARSERIMPL_PARSE_IPV4 h.eth.etherType 0x800
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	jmpeq MAINPARSERIMPL_PARSE_TCP h.ipv4.protocol 0x6
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_TCP :	extract h.tcp
	MAINPARSERIMPL_ACCEPT :	mov m.MainControlT_do_add_on_miss 0
	jmpnv LABEL_END h.ipv4
	jmpnv LABEL_END h.tcp
	table set_ct_options
	table ip_fwd_table
	regrd m.pna_main_input_metadata_direction direction m.pna_main_input_metadata_input_port
	jmpeq LABEL_TRUE_0 m.pna_main_input_metadata_direction 0x0
	mov m.MainControlT_key h.ipv4.dstAddr
	jmp LABEL_END_0
	LABEL_TRUE_0 :	mov m.MainControlT_key h.ipv4.srcAddr
	LABEL_END_0 :	jmpeq LABEL_TRUE_1 m.pna_main_input_metadata_direction 0x0
	mov m.MainControlT_key_0 h.ipv4.srcAddr
	jmp LABEL_END_1
	LABEL_TRUE_1 :	mov m.MainControlT_key_0 h.ipv4.dstAddr
	LABEL_END_1 :	jmpeq LABEL_TRUE_2 m.pna_main_input_metadata_direction 0x0
	mov m.MainControlT_key_1 h.tcp.dstPort
	jmp LABEL_END_2
	LABEL_TRUE_2 :	mov m.MainControlT_key_1 h.tcp.srcPort
	LABEL_END_2 :	jmpeq LABEL_TRUE_3 m.pna_main_input_metadata_direction 0x0
	mov m.MainControlT_key_2 h.tcp.srcPort
	jmp LABEL_END_3
	LABEL_TRUE_3 :	mov m.MainControlT_key_2 h.tcp.dstPort
	LABEL_END_3 :	mov m.MainControlImpl_ct_tcp_table_ipv4_protocol h.ipv4.protocol
	table ct_tcp_table
	LABEL_END :	emit h.eth
	emit h.ipv4
	emit h.tcp
	tx m.pna_main_output_metadata_output_port
}


