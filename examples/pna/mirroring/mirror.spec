

struct ethernet_t {
	bit<48> dstAddr
	bit<48> srcAddr
	bit<16> etherType
}

struct ipv4_t {
	bit<8> version_ihl
	bit<8> diffserv
	bit<16> totalLen
	bit<16> identification
	bit<16> flags_fragOffset
	bit<8> ttl
	bit<8> protocol
	bit<16> hdrChecksum
	bit<32> srcAddr
	bit<32> dstAddr
}

struct mirror_and_send_arg_t {
	bit<32> port
	bit<16> mirror_session_id
}

struct send_arg_t {
	bit<32> port
}

header ethernet instanceof ethernet_t
header ipv4 instanceof ipv4_t

struct user_metadata_t {
	bit<32> pna_main_input_metadata_input_port
	bit<32> pna_main_output_metadata_output_port
	bit<8> mirrorSlot
	bit<16> mirrorSession
}
metadata instanceof user_metadata_t


regarray direction size 0x100 initval 0

action NoAction args none {
	return
}

action drop args none {
	drop
	return
}

action send args instanceof send_arg_t {
	mov m.pna_main_output_metadata_output_port t.port
	return
}

action mirror_and_send args instanceof mirror_and_send_arg_t {
	mov m.mirrorSlot 0x3
	mov m.mirrorSession t.mirror_session_id
	mirror m.mirrorSlot m.mirrorSession
	mov m.pna_main_output_metadata_output_port t.port
	return
}

table fwd {
	key {
		h.ethernet.dstAddr exact
		h.ethernet.srcAddr exact
	}
	actions {
		drop
		send
		mirror_and_send
		NoAction
	}
	default_action NoAction args none 
	size 0x10000
}


apply {
	rx m.pna_main_input_metadata_input_port
	extract h.ethernet
	jmpeq MAINPARSERIMPL_PARSE_IPV4 h.ethernet.etherType 0x800
	jmp MAINPARSERIMPL_ACCEPT
	MAINPARSERIMPL_PARSE_IPV4 :	extract h.ipv4
	MAINPARSERIMPL_ACCEPT :	jmpnv LABEL_END h.ethernet
	table fwd
	LABEL_END :	tx m.pna_main_output_metadata_output_port
}


