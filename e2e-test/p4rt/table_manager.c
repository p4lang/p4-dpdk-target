#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <rte_ethdev.h>
#include <rte_eal.h>
#include <rte_lcore.h>
#include <rte_malloc.h>

// P4 Runtime API headers
#include "p4rt/table_entry.h"
#include "p4rt/runtime.h"

// Convert MAC string to byte array
static void str_to_mac(const char *str, uint8_t *mac) {
    sscanf(str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
           &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
}

// Convert IP string to uint32_t
static uint32_t str_to_ip(const char *str) {
    struct in_addr addr;
    inet_pton(AF_INET, str, &addr);
    return ntohl(addr.s_addr);
}

// Print table entries
static void print_table_entries(p4rt_ctx_t *ctx, const char *table_name) {
    printf("\nEntries in table '%s':\n", table_name);
    p4rt_table_entry_t *entries = NULL;
    uint32_t num_entries = 0;
    
    if (p4rt_table_entries_get(ctx, table_name, &entries, &num_entries) != 0) {
        printf("Failed to get entries for table %s\n", table_name);
        return;
    }
    
    printf("Found %d entries\n", num_entries);
    
    for (uint32_t i = 0; i < num_entries; i++) {
        p4rt_table_entry_t *entry = &entries[i];
        printf("Entry %d:\n", i);
        
        // Print match keys
        for (uint32_t j = 0; j < entry->num_match_fields; j++) {
            p4rt_match_field_t *field = &entry->match_fields[j];
            printf("  Match field '%s': ", field->name);
            
            if (field->match_type == P4RT_MATCH_EXACT) {
                printf("Exact match, value: ");
                for (uint32_t k = 0; k < field->data.exact.length; k++) {
                    printf("%02x", field->data.exact.value[k]);
                }
            } else if (field->match_type == P4RT_MATCH_TERNARY) {
                printf("Ternary match, value: ");
                for (uint32_t k = 0; k < field->data.ternary.length; k++) {
                    printf("%02x", field->data.ternary.value[k]);
                }
                printf(", mask: ");
                for (uint32_t k = 0; k < field->data.ternary.length; k++) {
                    printf("%02x", field->data.ternary.mask[k]);
                }
            }
            printf("\n");
        }
        
        // Print action info
        printf("  Action: %s\n", entry->action_name);
        for (uint32_t j = 0; j < entry->num_action_params; j++) {
            p4rt_action_param_t *param = &entry->action_params[j];
            printf("    Param '%s': ", param->name);
            for (uint32_t k = 0; k < param->length; k++) {
                printf("%02x", param->value[k]);
            }
            printf("\n");
        }
        
        printf("  Priority: %d\n", entry->priority);
    }
    
    // Free allocated resources
    if (entries) {
        p4rt_table_entries_free(entries, num_entries);
    }
}

int main(int argc, char *argv[]) {
    // Initialize EAL
    int ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "Error initializing EAL\n");
    }
    
    // Initialize P4 Runtime context
    p4rt_ctx_t *ctx = p4rt_context_init("mac_modifier");
    if (!ctx) {
        rte_exit(EXIT_FAILURE, "Failed to initialize P4 Runtime context\n");
    }
    
    printf("P4 Runtime context initialized successfully\n");
    
    // Add entry to exact match table
    p4rt_table_entry_t exact_entry = {0};
    exact_entry.table_name = "exact_match_table";
    
    // Allocate memory for match fields
    exact_entry.num_match_fields = 3;
    exact_entry.match_fields = (p4rt_match_field_t *)malloc(
        exact_entry.num_match_fields * sizeof(p4rt_match_field_t));
    
    // Set match fields
    uint8_t dst_mac[6] = {0};
    str_to_mac("00:11:22:33:44:55", dst_mac);
    exact_entry.match_fields[0].name = "ethernet.dstAddr";
    exact_entry.match_fields[0].match_type = P4RT_MATCH_EXACT;
    exact_entry.match_fields[0].data.exact.value = dst_mac;
    exact_entry.match_fields[0].data.exact.length = 6;
    
    uint8_t src_mac[6] = {0};
    str_to_mac("AA:BB:CC:DD:EE:FF", src_mac);
    exact_entry.match_fields[1].name = "ethernet.srcAddr";
    exact_entry.match_fields[1].match_type = P4RT_MATCH_EXACT;
    exact_entry.match_fields[1].data.exact.value = src_mac;
    exact_entry.match_fields[1].data.exact.length = 6;
    
    uint8_t protocol = 6; // TCP
    exact_entry.match_fields[2].name = "ipv4.protocol";
    exact_entry.match_fields[2].match_type = P4RT_MATCH_EXACT;
    exact_entry.match_fields[2].data.exact.value = &protocol;
    exact_entry.match_fields[2].data.exact.length = 1;
    
    // Set action
    exact_entry.action_name = "modify_src_mac";
    exact_entry.num_action_params = 1;
    exact_entry.action_params = (p4rt_action_param_t *)malloc(
        exact_entry.num_action_params * sizeof(p4rt_action_param_t));
    
    uint8_t new_src_mac[6] = {0};
    str_to_mac("11:22:33:44:55:66", new_src_mac);
    exact_entry.action_params[0].name = "new_src_mac";
    exact_entry.action_params[0].value = new_src_mac;
    exact_entry.action_params[0].length = 6;
    
    // Add entry to table
    ret = p4rt_table_entry_add(ctx, &exact_entry);
    if (ret != 0) {
        printf("Failed to add entry to exact match table: %d\n", ret);
    } else {
        printf("Successfully added entry to exact match table\n");
    }
    
    // Add entry to ternary match table
    p4rt_table_entry_t ternary_entry = {0};
    ternary_entry.table_name = "ternary_match_table";
    
    // Allocate memory for match fields
    ternary_entry.num_match_fields = 3;
    ternary_entry.match_fields = (p4rt_match_field_t *)malloc(
        ternary_entry.num_match_fields * sizeof(p4rt_match_field_t));
    
    // Set match fields
    uint32_t src_ip = str_to_ip("192.168.1.0");
    uint32_t src_ip_mask = str_to_ip("255.255.255.0");
    ternary_entry.match_fields[0].name = "ipv4.srcAddr";
    ternary_entry.match_fields[0].match_type = P4RT_MATCH_TERNARY;
    ternary_entry.match_fields[0].data.ternary.value = (uint8_t *)&src_ip;
    ternary_entry.match_fields[0].data.ternary.mask = (uint8_t *)&src_ip_mask;
    ternary_entry.match_fields[0].data.ternary.length = 4;
    
    uint32_t dst_ip = str_to_ip("10.0.0.0");
    uint32_t dst_ip_mask = str_to_ip("255.0.0.0");
    ternary_entry.match_fields[1].name = "ipv4.dstAddr";
    ternary_entry.match_fields[1].match_type = P4RT_MATCH_TERNARY;
    ternary_entry.match_fields[1].data.ternary.value = (uint8_t *)&dst_ip;
    ternary_entry.match_fields[1].data.ternary.mask = (uint8_t *)&dst_ip_mask;
    ternary_entry.match_fields[1].data.ternary.length = 4;
    
    uint8_t proto_val = 17; // UDP
    uint8_t proto_mask = 0xFF;
    ternary_entry.match_fields[2].name = "ipv4.protocol";
    ternary_entry.match_fields[2].match_type = P4RT_MATCH_TERNARY;
    ternary_entry.match_fields[2].data.ternary.value = &proto_val;
    ternary_entry.match_fields[2].data.ternary.mask = &proto_mask;
    ternary_entry.match_fields[2].data.ternary.length = 1;
    
    // Set action
    ternary_entry.action_name = "modify_dst_mac";
    ternary_entry.num_action_params = 1;
    ternary_entry.action_params = (p4rt_action_param_t *)malloc(
        ternary_entry.num_action_params * sizeof(p4rt_action_param_t));
    
    uint8_t new_dst_mac[6] = {0};
    str_to_mac("66:55:44:33:22:11", new_dst_mac);
    ternary_entry.action_params[0].name = "new_dst_mac";
    ternary_entry.action_params[0].value = new_dst_mac;
    ternary_entry.action_params[0].length = 6;
    
    // Set priority for ternary match
    ternary_entry.priority = 10;
    
    // Add entry to table
    ret = p4rt_table_entry_add(ctx, &ternary_entry);
    if (ret != 0) {
        printf("Failed to add entry to ternary match table: %d\n", ret);
    } else {
        printf("Successfully added entry to ternary match table\n");
    }
    
    // Print all table entries
    print_table_entries(ctx, "exact_match_table");
    print_table_entries(ctx, "ternary_match_table");
    
    // Clean up resources
    free(exact_entry.match_fields);
    free(exact_entry.action_params);
    free(ternary_entry.match_fields);
    free(ternary_entry.action_params);
    
    // Clean up P4 Runtime context
    p4rt_context_cleanup(ctx);
    
    // Clean up EAL
    rte_eal_cleanup();
    
    return 0;
}