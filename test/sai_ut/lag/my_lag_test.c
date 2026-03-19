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

int main()
{
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
    lag_member_attrs[1].id = SAI_LAG_MEMBER_ATTR_PORT_ID;
    lag_member_attrs[0].id = SAI_LAG_MEMBER_ATTR_LAG_ID;
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

    sai_attribute_t           lag_attrs[2];
    lag_attrs[0].id = SAI_LAG_ATTR_PORT_LIST;
    lag_attrs[0].value.objlist.list = port_list;
    lag_attrs[0].value.objlist.count = 64;
    status = lag_api->get_lag_attribute(lag_oid[0], 1, lag_attrs);
    status = lag_api->get_lag_attribute(lag_oid[1], 1, lag_attrs);
    
    lag_member_attrs[0].id = SAI_LAG_MEMBER_ATTR_LAG_ID;
    status = lag_api->get_lag_member_attribute(lag_member_oid[0], 1, lag_member_attrs);
    lag_member_attrs[0].id = SAI_LAG_MEMBER_ATTR_PORT_ID;
    status = lag_api->get_lag_member_attribute(lag_member_oid[2], 1, lag_member_attrs);

    status = lag_api->remove_lag_member(lag_member_oid[1]);

    status = lag_api->get_lag_attribute(lag_oid[0], 1, lag_attrs);

    status = lag_api->remove_lag_member(lag_member_oid[2]);

    status = lag_api->get_lag_attribute(lag_oid[1], 1, lag_attrs);

    status = lag_api->remove_lag_member(lag_member_oid[0]);
    status = lag_api->remove_lag_member(lag_member_oid[3]);
    status = lag_api->remove_lag(lag_oid[1]);
    status = lag_api->remove_lag(lag_oid[0]);

    switch_api->shutdown_switch(0);
    status = sai_api_uninitialize();

    return 0;
}