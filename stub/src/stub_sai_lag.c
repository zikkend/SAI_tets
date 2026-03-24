#include "sai.h"
#include "stub_sai.h"
#include "assert.h"

static const sai_attribute_entry_t lag_attribs[] = {
    { SAI_LAG_ATTR_PORT_LIST, false, false, false, true,
      "List of ports in LAG", SAI_ATTR_VAL_TYPE_OBJLIST },
    { END_FUNCTIONALITY_ATTRIBS_ID, false, false, false, false,
      "", SAI_ATTR_VAL_TYPE_UNDETERMINED }
};

static const sai_attribute_entry_t lag_member_attribs[] = {
    { SAI_LAG_MEMBER_ATTR_LAG_ID, true, true, false, true,
      "LAG ID", SAI_ATTR_VAL_TYPE_OID },
    { SAI_LAG_MEMBER_ATTR_PORT_ID, true, true, false, true,
      "PORT ID", SAI_ATTR_VAL_TYPE_OID },
    { END_FUNCTIONALITY_ATTRIBS_ID, false, false, false, false,
      "", SAI_ATTR_VAL_TYPE_UNDETERMINED }
};

#define MAX_NUMBER_OF_LAG_MEMBERS 16
#define MAX_NUMBER_OF_LAGS 5

typedef struct _lag_member_db_entry_t {
    bool            is_used;
    sai_object_id_t port_oid;
    sai_object_id_t lag_oid;
} lag_member_db_entry_t;

typedef struct _lag_member_entry_t {
    bool            is_used;
    uint16_t        lag_member_db_id;
} lag_member_entry_t;

typedef struct _lag_db_entry_t {
    bool               is_used;
    lag_member_entry_t member_db_idx[MAX_NUMBER_OF_LAG_MEMBERS];
    uint16_t           members_count;
} lag_db_entry_t;

struct lag_db_t {
    lag_db_entry_t        lags[MAX_NUMBER_OF_LAGS];
    lag_member_db_entry_t members[MAX_NUMBER_OF_LAG_MEMBERS];
} lag_db;


sai_status_t get_lag_member_attribute(_In_ const sai_object_key_t   *key,
                                      _Inout_ sai_attribute_value_t *value,
                                      _In_ uint32_t                  attr_index,
                                      _Inout_ vendor_cache_t        *cache,
                                      void                          *arg)
{
    sai_status_t status;
    uint32_t     lag_member_db_index;

    status = stub_object_to_type(key->object_id, SAI_OBJECT_TYPE_LAG_MEMBER, &lag_member_db_index);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Cannot get LAG MEMBER DB index.\n");
        return status;
    }
    if (lag_member_db_index >= MAX_NUMBER_OF_LAG_MEMBERS) {
        printf("Invalid LAG MEMBER ID 0x%lX\n", key->object_id);
        return SAI_STATUS_ITEM_NOT_FOUND;
    }
    if (!lag_db.members[lag_member_db_index].is_used) {
        printf("Attempt to get from an already freed LAG MEMBER ID 0x%lX\n", key->object_id);
        return SAI_STATUS_ITEM_NOT_FOUND;
    }

    switch ((int64_t)arg) {
    case SAI_LAG_MEMBER_ATTR_LAG_ID:
        value->oid = lag_db.members[lag_member_db_index].lag_oid;
        break;
    case SAI_LAG_MEMBER_ATTR_PORT_ID:
        value->oid = lag_db.members[lag_member_db_index].port_oid;
        break;
    default:
        printf("Got unexpected attribute ID\n");
        return SAI_STATUS_FAILURE;
    }

    return SAI_STATUS_SUCCESS;
}

static const sai_vendor_attribute_entry_t lag_member_vendor_attribs[] = {
    { SAI_LAG_MEMBER_ATTR_LAG_ID,
      { true, false, false, true },
      { true, false, false, true },
      get_lag_member_attribute, (void*) SAI_LAG_MEMBER_ATTR_LAG_ID,
      NULL, NULL },
    { SAI_LAG_MEMBER_ATTR_PORT_ID,
      { true, false, false, true },
      { true, false, false, true },
      get_lag_member_attribute, (void*) SAI_LAG_MEMBER_ATTR_PORT_ID,
      NULL, NULL }
};

sai_status_t get_lag_attribute(_In_ const sai_object_key_t   *key,
                                      _Inout_ sai_attribute_value_t *value,
                                      _In_ uint32_t                  attr_index,
                                      _Inout_ vendor_cache_t        *cache,
                                      void                          *arg)
{
    sai_status_t status;
    uint32_t     lag_db_index;

    status = stub_object_to_type(key->object_id, SAI_OBJECT_TYPE_LAG, &lag_db_index);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Cannot get LAG DB index.\n");
        return status;
    }
    if (lag_db_index >= MAX_NUMBER_OF_LAGS) {
        printf("Invalid LAG ID 0x%lX\n", key->object_id);
        return SAI_STATUS_ITEM_NOT_FOUND;
    }
    if (!lag_db.lags[lag_db_index].is_used) {
        printf("Attempt to get from an already freed LAG ID 0x%lX\n", key->object_id);
        return SAI_STATUS_ITEM_NOT_FOUND;
    }

    lag_db_entry_t* const lag_db_entry = &lag_db.lags[lag_db_index];

    switch ((int64_t)arg) {
    case SAI_LAG_ATTR_PORT_LIST:
        if (lag_db_entry->members_count > value->objlist.count) {
            value->objlist.count = lag_db_entry->members_count;
            status = SAI_STATUS_BUFFER_OVERFLOW;
            break;
        }

        uint32_t current_list_idx = 0;
        for (uint32_t ii = 0; ii < MAX_NUMBER_OF_LAG_MEMBERS; ii++)
        {
            if (lag_db_entry->member_db_idx[ii].is_used) {
                uint32_t lag_member_db_id = lag_db_entry->member_db_idx[ii].lag_member_db_id;
                value->objlist.list[current_list_idx++] = lag_db.members[lag_member_db_id].port_oid;
            }
        }

        assert(lag_db_entry->members_count == current_list_idx);
        value->objlist.count = lag_db_entry->members_count;
        status = SAI_STATUS_SUCCESS;
        break;
    default:
        printf("Got unexpected attribute ID\n");
        return SAI_STATUS_FAILURE;
    }
    return status;
}

static const sai_vendor_attribute_entry_t lag_vendor_attribs[] = {
    { SAI_LAG_ATTR_PORT_LIST,
      { false, false, false, true },
      { false, false, false, true },
      get_lag_attribute, (void*) SAI_LAG_ATTR_PORT_LIST,
      NULL, NULL }
};

sai_status_t stub_create_lag(
    _Out_ sai_object_id_t* lag_id,
    _In_ uint32_t attr_count,
    _In_ sai_attribute_t *attr_list)
{
    sai_status_t status;
    status = check_attribs_metadata(attr_count, attr_list, lag_attribs, lag_vendor_attribs, SAI_OPERATION_CREATE);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed LAG attributes check\n");
        return status;
    }

    uint32_t ii = 0;
    for (; ii < MAX_NUMBER_OF_LAGS; ii++) {
        if (!lag_db.lags[ii].is_used) {
            break;
        }
    }
    if (ii == MAX_NUMBER_OF_LAGS) {
        printf("Cannot create LAG: limit is reached\n");
        return SAI_STATUS_INSUFFICIENT_RESOURCES;
    }

    const uint32_t lag_db_id = ii;
    status = stub_create_object(SAI_OBJECT_TYPE_LAG, lag_db_id, lag_id);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Cannot create LAG OID\n");
        return status;
    }
    lag_db.lags[lag_db_id].is_used = true;
    lag_db.lags[lag_db_id].members_count = 0;

    char list_str[MAX_LIST_VALUE_STR_LEN];                                                       
    sai_attr_list_to_str(attr_count, attr_list, lag_attribs, MAX_LIST_VALUE_STR_LEN, list_str); 
    printf("CREATE LAG: 0x%lX (%s)\n", *lag_id, list_str);
    return status;
}

sai_status_t stub_remove_lag(
    _In_ sai_object_id_t  lag_id)
{
    sai_status_t status;
    uint32_t     lag_db_id;

    status = stub_object_to_type(lag_id, SAI_OBJECT_TYPE_LAG, &lag_db_id);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Cannot get LAG DB ID for LAG removal.\n");
        return status;
    }
    if (lag_db_id >= MAX_NUMBER_OF_LAGS) {
        printf("Invalid LAG ID 0x%lX\n", lag_id);
        return SAI_STATUS_ITEM_NOT_FOUND;
    }
    if (!lag_db.lags[lag_db_id].is_used) {
        printf("Attempt to free an already freed LAG ID 0x%lX\n", lag_id);
        return SAI_STATUS_ITEM_NOT_FOUND;
    }
    if (lag_db.lags[lag_db_id].members_count > 0) {
        printf("Cannot remove LAG 0x%lX, it has attached LAG MEMBER\n", lag_id);
        return SAI_STATUS_OBJECT_IN_USE;
    }

    assert(lag_db.lags[lag_db_id].members_count == 0);
    lag_db.lags[lag_db_id].is_used = false;
    memset(lag_db.lags[lag_db_id].member_db_idx, 0, sizeof(lag_db.lags[lag_db_id].member_db_idx));

    printf("REMOVE LAG: 0x%lX\n", lag_id);
    return SAI_STATUS_SUCCESS;
}

sai_status_t stub_set_lag_attribute(
    _In_ sai_object_id_t  lag_id,
    _In_ const sai_attribute_t *attr)
{
    printf("SET LAG ATTRIBUTE IS CALLED FOR: 0x%lX\n", lag_id);
    return SAI_STATUS_SUCCESS;
}

sai_status_t stub_get_lag_attribute(
    _In_ sai_object_id_t lag_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list)
{
    printf("GET LAG ATTRIBUTE IS CALLED FOR: 0x%lX\n", lag_id);
    const sai_object_key_t key = { .object_id = lag_id };
    return sai_get_attributes(&key, NULL, lag_attribs, lag_vendor_attribs, attr_count, attr_list);
}

sai_status_t stub_create_lag_member(
    _Out_ sai_object_id_t* lag_member_id,
    _In_ uint32_t attr_count,
    _In_ sai_attribute_t *attr_list)
{
    sai_status_t status;
    status = check_attribs_metadata(attr_count, attr_list, lag_member_attribs, lag_member_vendor_attribs, SAI_OPERATION_CREATE);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Failed LAG MEMBER attributes check\n");
        return status;
    }

    const sai_attribute_value_t *lag_id, *port_id;
    uint32_t lag_id_idx, port_id_idx;
    status = find_attrib_in_list(attr_count, attr_list, SAI_LAG_MEMBER_ATTR_LAG_ID, &lag_id, &lag_id_idx);
    if (status != SAI_STATUS_SUCCESS) {
        printf("LAG_ID attribute not found.\n");
        return status;
    }
    status = find_attrib_in_list(attr_count, attr_list, SAI_LAG_MEMBER_ATTR_PORT_ID, &port_id, &port_id_idx);
    if (status != SAI_STATUS_SUCCESS) {
        printf("PORT_ID attribute not found.\n");
        return status;
    }
    // can also check if there are other LAG MEMBERS on the same port

    uint32_t ii = 0;
    for (; ii < MAX_NUMBER_OF_LAG_MEMBERS; ii++) {
        if (!lag_db.members[ii].is_used) {
            break;
        }
    }
    if (ii == MAX_NUMBER_OF_LAG_MEMBERS) {
        printf("Cannot create LAG MEMBER: limit is reached\n");
        return SAI_STATUS_INSUFFICIENT_RESOURCES;
    }

    uint32_t lag_db_id;
    status = stub_object_to_type(lag_id->oid, SAI_OBJECT_TYPE_LAG, &lag_db_id);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Cannot get LAG DB ID for LAG MEMBER creation.\n");
        return status;
    }
    if (lag_db_id >= MAX_NUMBER_OF_LAGS || !lag_db.lags[lag_db_id].is_used) {
        printf("Passed LAG ID for non-existent LAG");
    }
    lag_db_entry_t* const lag_db_entry = &lag_db.lags[lag_db_id];
    if (lag_db_entry->members_count >= MAX_NUMBER_OF_LAG_MEMBERS) {
        printf("Too many LAG MEMBERS for LAG %lX", lag_id->oid);
        return SAI_STATUS_FAILURE;
    }

    // check if we can add lag_member id to lag member list
    uint32_t jj = 0;
    for (; jj < MAX_NUMBER_OF_LAG_MEMBERS; jj++) {
        if (!lag_db_entry->member_db_idx[jj].is_used) {
            break;
        }
    }
    if (jj == MAX_NUMBER_OF_LAG_MEMBERS) {
        printf("Cannot add new LAG MEMBER %lX for LAG %lX: limit is reached\n", *lag_member_id, lag_id->oid);
        return SAI_STATUS_INSUFFICIENT_RESOURCES;
    }

     // fill lag_member entry
    const sai_uint32_t lag_member_db_id = ii;
    status = stub_create_object(SAI_OBJECT_TYPE_LAG_MEMBER, lag_member_db_id, lag_member_id);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Cannot create LAG MEMBER OID\n");
        return status;
    }
    lag_db.members[lag_member_db_id].is_used = true;
    lag_db.members[lag_member_db_id].lag_oid = lag_id->oid;
    lag_db.members[lag_member_db_id].port_oid = port_id->oid;

    // add lag_member id to lag member list
    sai_uint32_t lag_member_list_id = jj;
    lag_db_entry->member_db_idx[lag_member_list_id].lag_member_db_id = lag_member_db_id;
    lag_db_entry->member_db_idx[lag_member_list_id].is_used = true;
    lag_db_entry->members_count++;

    char list_str[MAX_LIST_VALUE_STR_LEN];                                                       
    sai_attr_list_to_str(attr_count, attr_list, lag_member_attribs, MAX_LIST_VALUE_STR_LEN, list_str); 
    printf("CREATE LAG MEMBER: 0x%lX (%s)\n", *lag_member_id, list_str);
    return status;
}

sai_status_t stub_remove_lag_member(
    _In_ sai_object_id_t  lag_member_id)
{
    sai_status_t status;
    uint32_t     lag_member_db_id;
    
    status = stub_object_to_type(lag_member_id, SAI_OBJECT_TYPE_LAG_MEMBER, &lag_member_db_id);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Cannot get LAG MEMBER DB ID.\n");
        return status;
    }
    if (lag_member_db_id >= MAX_NUMBER_OF_LAG_MEMBERS) {
        printf("Invalid LAG MEMBER ID 0x%lX\n", lag_member_id);
        return SAI_STATUS_ITEM_NOT_FOUND;
    }
    if (!lag_db.members[lag_member_db_id].is_used) {
        printf("Attempt to free an already freed LAG MEMBER ID 0x%lX\n", lag_member_id);
        return SAI_STATUS_ITEM_NOT_FOUND;
    }

    uint32_t lag_db_id;
    status = stub_object_to_type(lag_db.members[lag_member_db_id].lag_oid, SAI_OBJECT_TYPE_LAG, &lag_db_id);
    if (status != SAI_STATUS_SUCCESS) {
        printf("Cannot get LAG DB ID for LAG MEMBER removal.\n");
        return status;
    }
    assert(lag_db_id < MAX_NUMBER_OF_LAGS);
    lag_db_entry_t* const lag_db_entry = &lag_db.lags[lag_db_id];
    assert(lag_db_entry->is_used);

    // remove lag_member from lag member list
    uint32_t jj = 0;
    for (; jj < MAX_NUMBER_OF_LAG_MEMBERS; jj++) {
        if (lag_db_entry->member_db_idx[jj].is_used && 
            lag_db_entry->member_db_idx[jj].lag_member_db_id == lag_member_db_id) {
            break;
        }
    }
    assert(jj < MAX_NUMBER_OF_LAG_MEMBERS);
    lag_db_entry->member_db_idx[jj].is_used = false;
    assert(lag_db_entry->members_count > 0);
    lag_db_entry->members_count--;

    // clear lag member data
    lag_db.members[lag_member_db_id].is_used = false;
    lag_db.members[lag_member_db_id].lag_oid = SAI_NULL_OBJECT_ID;
    lag_db.members[lag_member_db_id].port_oid = SAI_NULL_OBJECT_ID;

    printf("REMOVE LAG MEMBER: 0x%lX\n", lag_member_id);
    return SAI_STATUS_SUCCESS;
}

sai_status_t stub_set_lag_member_attribute(
    _In_ sai_object_id_t  lag_member_id,
    _In_ const sai_attribute_t *attr)
{
    printf("SET LAG MEMBER ATTRIBUTE IS CALLED FOR: 0x%lX\n", lag_member_id);
    return SAI_STATUS_SUCCESS;
}

sai_status_t stub_get_lag_member_attribute(
    _In_ sai_object_id_t lag_member_id,
    _In_ uint32_t attr_count,
    _Inout_ sai_attribute_t *attr_list)
{
    printf("GET LAG MEMBER ATTRIBUTE IS CALLED FOR: 0x%lX\n", lag_member_id);
    const sai_object_key_t key = { .object_id = lag_member_id };
    return sai_get_attributes(&key, NULL, lag_member_attribs, lag_member_vendor_attribs, attr_count, attr_list);
}

const sai_lag_api_t lag_api = {
    stub_create_lag,
    stub_remove_lag,
    stub_set_lag_attribute,
    stub_get_lag_attribute,
    stub_create_lag_member,
    stub_remove_lag_member,
    stub_set_lag_member_attribute,
    stub_get_lag_member_attribute
};