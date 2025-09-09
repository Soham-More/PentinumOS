#pragma once

#include <includes.h>

namespace vfs
{
    enum NODE_TYPE : u32
    {
        NODE_FILE      = 0x0,
        NODE_DEVICE    = 0x1,
        NODE_BLOCK     = 0x2,

        NODE_TYPE_MASK = 0x7,
        NODE_DIRECTORY = 0x8,
    };

    enum EVENT_TYPE : u32
    {
        EVENT_INVALID    = 0x0,
        EVENT_DEVICE_ADD = 0x1,
        EVENT_DEVICE_DEL = 0x2,
        EVENT_BLOCK_ADD  = 0x3,
        EVENT_BLOCK_DEL  = 0x4,

        EVENT_DEVICE_READY = 0x5,
        EVENT_BLOCK_READY  = 0x6,
    };

    struct node_t
    {
        const char* name;
        u32 flags;
        u32 driver_uid;
        u32 size;

        void* data;
        void (*read) (node_t*, size_t offset, void* buffer, size_t len) = 0;
        i32  (*write)(node_t*, size_t offset, const void* buffer, size_t len) = 0;

        uid_t uid;
        // location in filesystem
        node_t* parent;
        node_t* next;
        node_t* prev;
        // all the children of this node
        u32 child_count;
        node_t* children_head;
        node_t* children_tail;
    };

    struct event_t
    {
        u32 flags;
        uid_t driver_uid;
        node_t* trigger_node;
    };

    struct vfs_t;

    void ignore_read (node_t*, size_t offset, void* buffer, size_t len);
    i32  ignore_write(node_t*, size_t offset, const void* buffer, size_t len);

    // initialize vfs
    vfs_t* init();

    // get node for the path
    node_t* get_node(vfs_t* instance, const char* path);

    node_t make_node(const char* name, bool is_directory);

    // add a "virtual"(i.e does not exist on a filesystem) node
    i32 add_vnode(vfs_t* instance, const char* path, node_t dnode);

    // add a device node at path
    i32 add_dnode(vfs_t* instance, const char* path, node_t dnode);
    // remove a device node at path
    i32 remove_dnode(vfs_t* instance, const char* path);
    // add a device node at path
    i32 add_bnode(vfs_t* instance, const char* path, node_t blk_node);
    // remove a device node at path
    i32 remove_bnode(vfs_t* instance, const char* path);

    // set a device/block device ready
    u32 set_node_ready(vfs_t* instance, node_t* node, uid_t driver_uid);

    // ls path
    void log_children(vfs_t* instance, const char* path);

    // free a node
    void free_node(node_t* node);

    // poll events
    bool poll(vfs_t* instance);
    // pop an event
    event_t pop_event(vfs_t* instance);
}

