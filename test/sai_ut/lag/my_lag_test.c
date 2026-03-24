#include <stdio.h>
#include <string.h>
#include "sai.h"

const char* test_profile_get_value(
    _In_ sai_switch_profile_id_t profile_id,
    _In_ const char* variable)
{
    return 0;
}

int test_profile_get_next_value(
    _In_ sai_switch_profile_id_t profile_id,
    _Out_ const char** variable,
    _Out_ const char** value)
{
    return -1;
}

const service_method_table_t test_services = {
    test_profile_get_value,
    test_profile_get_next_value
};

int generic_test() {
    sai_status_t              status;
    sai_switch_api_t         *switch_api;
    sai_attribute_t           attrs[2];
    sai_switch_notification_t notifications;
    sai_object_id_t           port_list[64];

    status = sai_api_initialize(0, &test_services);
    status = sai_api_query(SAI_API_SWITCH, (void**)&switch_api);
    status = switch_api->initialize_switch(0, "HW_ID", 0, &notifications);
    attrs[0].id = SAI_SWITCH_ATTR_PORT_LIST;
    attrs[0].value.objlist.list = port_list;
    attrs[0].value.objlist.count = 64;
    status = switch_api->get_switch_attribute(1, attrs);
    if (status != SAI_STATUS_SUCCESS || attrs[0].value.objlist.count < 4) {
        printf("Failed to retrieve enough ports\n");
        return 1;
    }

    sai_lag_api_t            *lag_api;
    sai_object_id_t           lag_oid[2], lag_member_oid[4], port_oid[4];
    sai_attribute_t           lag_member_attrs[2];

    status = sai_api_query(SAI_API_LAG, (void**)&lag_api);
    lag_member_attrs[0].id = SAI_LAG_MEMBER_ATTR_LAG_ID;
    lag_member_attrs[1].id = SAI_LAG_MEMBER_ATTR_PORT_ID;
    memcpy(port_oid, attrs[0].value.objlist.list, sizeof(port_oid));


    status = lag_api->create_lag(&lag_oid[0], 0, NULL);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to create a LAG 1, status=%d\n", status);
        return 1;
    }
    // LAG member 1
    lag_member_attrs[0].value.oid = lag_oid[0];
    lag_member_attrs[1].value.oid = port_oid[0];
    status = lag_api->create_lag_member(&lag_member_oid[0], 2, lag_member_attrs);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to create a LAG MEMBER 1, status=%d\n", status);
        return 1;
    }
    // LAG member 2
    lag_member_attrs[0].value.oid = lag_oid[0];
    lag_member_attrs[1].value.oid = port_oid[1];
    status = lag_api->create_lag_member(&lag_member_oid[1], 2, lag_member_attrs);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to create a LAG MEMBER 2, status=%d\n", status);
        return 1;
    }

    status = lag_api->create_lag(&lag_oid[1], 0, NULL);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to create a LAG 2, status=%d\n", status);
        return 1;
    }
    // LAG member 3
    lag_member_attrs[0].value.oid = lag_oid[1];
    lag_member_attrs[1].value.oid = port_oid[2];
    status = lag_api->create_lag_member(&lag_member_oid[2], 2, lag_member_attrs);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to create a LAG MEMBER 3, status=%d\n", status);
        return 1;
    }
    // LAG member 4
    lag_member_attrs[0].value.oid = lag_oid[1];
    lag_member_attrs[1].value.oid = port_oid[3];
    status = lag_api->create_lag_member(&lag_member_oid[3], 2, lag_member_attrs);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to create a LAG MEMBER 4, status=%d\n", status);
        return 1;
    }

    // removing LAG 1 and LAG 2
    status = lag_api->remove_lag(lag_oid[0]);
    if (status != SAI_STATUS_OBJECT_IN_USE) {
        printf("Removed a LAG 1 while it has LAG MEMBERS, status=%d\n", status);
        return 1;
    }
    status = lag_api->remove_lag(lag_oid[1]);
    if (status != SAI_STATUS_OBJECT_IN_USE) {
        printf("Removed a LAG 2 while it has LAG MEMBERS, status=%d\n", status);
        return 1;
    }

    sai_attribute_t           lag_attrs[2];
    lag_attrs[0].id = SAI_LAG_ATTR_PORT_LIST;
    lag_attrs[0].value.objlist.list = port_list;
    lag_attrs[0].value.objlist.count = 1;
    status = lag_api->get_lag_attribute(lag_oid[0], 1, lag_attrs);
    if (status != SAI_STATUS_BUFFER_OVERFLOW || lag_attrs[0].value.objlist.count != 2) {
        printf("Incorrect PORT LIST for a LAG 1 for too small input list, status=%d\n", status);
        return 1;
    }

    lag_attrs[0].value.objlist.count = 64;
    status = lag_api->get_lag_attribute(lag_oid[0], 1, lag_attrs);
    if (status != SAI_STATUS_SUCCESS || lag_attrs[0].value.objlist.count != 2
        || lag_attrs[0].value.objlist.list[0] != port_oid[0]
        || lag_attrs[0].value.objlist.list[1] != port_oid[1]) 
    {
        printf("Incorrect PORT LIST for a LAG 1: 0x%lX and 0x%lX, status=%d\n", 
            lag_attrs[0].value.objlist.list[0], lag_attrs[0].value.objlist.list[1], status);
        return 1;
    }

    status = lag_api->get_lag_attribute(lag_oid[1], 1, lag_attrs);
    if (status != SAI_STATUS_SUCCESS || lag_attrs[0].value.objlist.count != 2
        || lag_attrs[0].value.objlist.list[0] != port_oid[2]
        || lag_attrs[0].value.objlist.list[1] != port_oid[3]) 
    {
        printf("Incorrect PORT LIST for a LAG 2: 0x%lX and 0x%lX, status=%d\n", 
            lag_attrs[0].value.objlist.list[0], lag_attrs[0].value.objlist.list[1], status);
        return 1;
    }
    
    lag_member_attrs[0].id = SAI_LAG_MEMBER_ATTR_LAG_ID;
    status = lag_api->get_lag_member_attribute(lag_member_oid[0], 1, lag_member_attrs);
    if (status != SAI_STATUS_SUCCESS || lag_member_attrs[0].value.oid != lag_oid[0]) {
        printf("Incorrect LAG_ID: 0x%lX for a LAG MEMBER 1, status=%d\n", 
            lag_member_attrs[0].value.oid, status);
        return 1;
    }

    lag_member_attrs[0].id = SAI_LAG_MEMBER_ATTR_PORT_ID;
    status = lag_api->get_lag_member_attribute(lag_member_oid[2], 1, lag_member_attrs);
    if (status != SAI_STATUS_SUCCESS || lag_member_attrs[0].value.oid != port_oid[2]) {
        printf("Incorrect PORT_ID: 0x%lX for a LAG MEMBER 3, status=%d\n", 
            lag_member_attrs[0].value.oid, status);
        return 1;
    }

    status = lag_api->remove_lag_member(lag_member_oid[1]);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to remove a LAG MEMBER 2, status=%d\n", status);
        return 1;
    }

    status = lag_api->get_lag_attribute(lag_oid[0], 1, lag_attrs);
    if (status != SAI_STATUS_SUCCESS || lag_attrs[0].value.objlist.count != 1
        || lag_attrs[0].value.objlist.list[0] != port_oid[0]) 
    {
        printf("Incorrect PORT LIST for a LAG 1: 0x%lX, status=%d\n", 
            lag_attrs[0].value.objlist.list[0], status);
        return 1;
    }

    status = lag_api->remove_lag_member(lag_member_oid[2]);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to remove a LAG MEMBER 3, status=%d\n", status);
        return 1;
    }

    status = lag_api->get_lag_attribute(lag_oid[1], 1, lag_attrs);
    if (status != SAI_STATUS_SUCCESS || lag_attrs[0].value.objlist.count != 1
        || lag_attrs[0].value.objlist.list[0] != port_oid[3]) 
    {
        printf("Incorrect PORT LIST for a LAG 1: 0x%lX, status=%d\n", 
            lag_attrs[0].value.objlist.list[0], status);
        return 1;
    }

    status = lag_api->remove_lag_member(lag_member_oid[0]);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to remove a LAG MEMBER 1, status=%d\n", status);
        return 1;
    }
    status = lag_api->remove_lag_member(lag_member_oid[3]);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to remove a LAG MEMBER 4, status=%d\n", status);
        return 1;
    }
    status = lag_api->remove_lag(lag_oid[1]);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to remove a LAG 2, status=%d\n", status);
        return 1;
    }
    status = lag_api->remove_lag(lag_oid[0]);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed to remove a LAG 1, status=%d\n", status);
        return 1;
    }

    switch_api->shutdown_switch(0);
    status = sai_api_uninitialize();

    return 0;
}

int db_overflow_test() {
    sai_status_t              status;
    sai_lag_api_t            *lag_api;
    sai_object_id_t           lag_oid[6], lag_member_oid[17];
    sai_switch_api_t         *switch_api;
    sai_attribute_t           attrs[2], lag_member_attrs[2];
    sai_switch_notification_t notifications;
    sai_object_id_t           port_list[64];

    int test_result = 0;

    status = sai_api_initialize(0, &test_services);
    status = sai_api_query(SAI_API_SWITCH, (void**)&switch_api);
    status = switch_api->initialize_switch(0, "HW_ID", 0, &notifications);
    attrs[0].id = SAI_SWITCH_ATTR_PORT_LIST;
    attrs[0].value.objlist.list = port_list;
    attrs[0].value.objlist.count = 64;
    status = switch_api->get_switch_attribute(1, attrs);

    uint32_t lags_created = 5;
    status = sai_api_query(SAI_API_LAG, (void**)&lag_api);
    for (uint32_t i = 0; i < 5; i++) {
        status = lag_api->create_lag(&lag_oid[i], 0, NULL);
        if (status != SAI_STATUS_SUCCESS) {
            printf("Failed to create a LAG %d, status=%d\n", i, status);
            test_result = 1;
            lags_created = i;
            goto tear_down;         
        }
    }
    lag_member_attrs[0].id = SAI_LAG_MEMBER_ATTR_LAG_ID;
    lag_member_attrs[1].id = SAI_LAG_MEMBER_ATTR_PORT_ID;

    uint32_t lag_members_created = 16;
    for (uint32_t i = 0; i < 16; i++)
    {
        lag_member_attrs[0].value.oid = lag_oid[0];
        lag_member_attrs[1].value.oid = attrs[0].value.objlist.list[0];;
        status = lag_api->create_lag_member(&lag_member_oid[i], 2, lag_member_attrs);
        if (status != SAI_STATUS_SUCCESS) {
            printf("Failed to create a LAG MEMBER %d, status=%d\n", i, status);
            test_result = 1;
            lag_members_created = i;
            goto tear_down;
        }
    }
    
    for (uint32_t i = 0; i < 5; i++)
    {
        lag_member_attrs[0].value.oid = lag_oid[0];
        lag_member_attrs[1].value.oid = attrs[0].value.objlist.list[0];;
        status = lag_api->create_lag_member(&lag_member_oid[16], 2, lag_member_attrs);
        if (status != SAI_STATUS_INSUFFICIENT_RESOURCES) {
            printf("Created too many LAG MEMBERs, status=%d\n", status);
            test_result = 1;
            lag_members_created = i;
            goto tear_down;
        }
    }

    for (uint32_t i = 0; i < 5; i++) {
        status = lag_api->create_lag(&lag_oid[6], 0, NULL);
        if (status != SAI_STATUS_INSUFFICIENT_RESOURCES) {
            printf("Created too many LAGs, status=%d\n", status);
            test_result = 1;
            goto tear_down;         
           
        }
    }


    tear_down:

    for (uint32_t i = 0; i < lag_members_created; i++) {
        status = lag_api->remove_lag_member(lag_member_oid[i]);
        if (status != SAI_STATUS_SUCCESS) {
            printf("Failed to remove a LAG MEMBER %d, status=%d\n", i, status);
            test_result = 1;         
        }
    }

    for (uint32_t i = 0; i < lags_created; i++) {
        status = lag_api->remove_lag(lag_oid[i]);
        if (status != SAI_STATUS_SUCCESS) {
            printf("Failed to remove a LAG %d, status=%d\n", i, status);
            test_result = 1;         
        }
    }

    return test_result;
}

int main()
{

    if (db_overflow_test())
        printf("DB overflow test failed\n");
    else
        printf("DB overflow test passed\n");  
    printf("-----------------------------------------------\n");

    if (generic_test())
        printf("Generic test failed\n");
    else
        printf("Generic test passed\n");
}
