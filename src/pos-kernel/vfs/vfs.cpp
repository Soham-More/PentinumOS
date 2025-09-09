#include "vfs.hpp"

#include <c/string.h>
#include <std/std.hpp>
#include <io/io.h>

namespace vfs
{
    void ignore_read (node_t*, size_t offset, void* buffer, size_t len){}
    i32  ignore_write(node_t*, size_t offset, const void* buffer, size_t len){return OP_SUCCESS;}

    struct event_link
    {
        event_t event;
        event_link* next;
    };

    struct vfs_t
    {
        size_t node_count;
        node_t root;

        // event queue
        event_link* queue_head;
        event_link* queue_tail;
    };

    static vfs_t g_vfs;

    vfs_t* init()
    {
        // setup root node
        g_vfs.root.name = "";
        g_vfs.root.flags = NODE_DIRECTORY;
        
        g_vfs.root.uid = 0;
        g_vfs.root.next = nullptr;
        g_vfs.root.prev = nullptr;

        g_vfs.root.child_count = 0;
        g_vfs.root.children_head = nullptr;
        g_vfs.root.children_tail = nullptr;

        // setup vfs data
        g_vfs.node_count = 1;
        
        // setup event chain
        g_vfs.queue_head = nullptr;
        g_vfs.queue_tail = nullptr;

        return &g_vfs;
    }

    void init_child(vfs_t* instance, node_t* child)
    {
        child->uid = instance->node_count;
        instance->node_count++;

        child->child_count = 0;
        child->children_tail = nullptr;
        child->children_head = nullptr;
    }
    void add_child(node_t* parent, node_t* child)
    {
        parent->child_count++;
        child->parent = parent;

        if(parent->children_tail == nullptr)
        {
            parent->children_head = child;
            parent->children_tail = child;
            return;
        }

        parent->children_tail->next = child;
        child->prev = parent->children_tail;
        
        parent->children_tail = child;
    }
    void add_event(vfs_t* instance, event_t event)
    {
        event_link* new_link = new event_link{};
        new_link->event = event;
        new_link->next = nullptr;

        if(instance->queue_tail == nullptr)
        {
            instance->queue_head = new_link;
            instance->queue_tail = instance->queue_head;
            return;
        }

        instance->queue_tail->next = new_link;
        instance->queue_tail = new_link;
    }
    void remove_child(node_t* child)
    {
        node_t* parent = child->parent;
        if(parent->children_tail == child)
        {
            if(parent->children_head == child)
            {
                parent->children_head = nullptr;
                parent->children_tail = nullptr;

                goto end;
            }
            parent->children_tail = child->prev;
            parent->children_tail->next = nullptr;

            child->next = nullptr;
            child->prev = nullptr;
            child->parent = nullptr;

            goto end;
        }

        child->next->prev = child->prev;
        child->prev->next = child->next;

        end:
        child->next = nullptr;
        child->prev = nullptr;
        child->parent = nullptr;

        // decrease the count
        child->parent->child_count--;
    }

    u32 node_len(const char* path)
    {
        const char* begin = path;
        while(*path != '/' && *path != '\0') path++;
        return (path - begin);
    }
    node_t* walk_path(vfs_t *instance, const char* path)
    {
        node_t* current_node = &instance->root;

        // skip delimenter
        if(*path == '/') path++;

        while(*path != '\0')
        {
            u32 len = node_len(path);

            node_t* new_node = ERR_PTR(ENONODE, node_t);
            node_t* child = current_node->children_head;
            while(child != nullptr)
            {
                if(strcmp(path, child->name, len) && child->name[len] == '\0')
                {
                    // found node
                    new_node = child;
                    break;
                }
                child = child->next;
                continue;
            }

            if(IS_ERR_PTR(new_node))
            {
                // forward the same error
                return new_node;
            }

            // otherwise
            current_node = new_node;

            path += len;
            // skip delimenter
            if(*path == '/') path++;
        }

        return current_node;
    }

    node_t* get_node(vfs_t *instance, const char* path)
    {
        return walk_path(instance, path);
    }

    node_t make_node(const char *name, bool is_directory)
    {
        return node_t{ .name = name, .flags = is_directory ? NODE_DIRECTORY : 0, .size=0, .data = nullptr };
    }

    i32 add_vnode(vfs_t *instance, const char *path, node_t dnode)
    {
        node_t* dir_node = walk_path(instance, path);
        // forward error
        if(IS_ERR_PTR(dir_node)) return ERR_CAST(dir_node);
        // the node is not a directory, return error
        if((dir_node->flags & NODE_DIRECTORY) == 0) return EINVPATH;

        node_t* child = new node_t(dnode);
        init_child(instance, child);
        add_child(dir_node, child);

        return OP_SUCCESS;
    }

    // add a device node at path
    i32 add_dnode(vfs_t* instance, const char* path, node_t dnode)
    {
        node_t* dir_node = walk_path(instance, path);
        // forward error
        if(IS_ERR_PTR(dir_node)) return ERR_CAST(dir_node);
        // the node is not a directory, return error
        if((dir_node->flags & NODE_DIRECTORY) == 0) return EINVPATH;

        node_t* child = new node_t(dnode);
        init_child(instance, child);
        add_child(dir_node, child);

        event_t event;
        event.driver_uid = dnode.driver_uid;
        event.flags = EVENT_DEVICE_ADD;
        event.trigger_node = child;

        add_event(instance, event);

        return OP_SUCCESS;
    }
    // remove a device node at path
    i32 remove_dnode(vfs_t* instance, const char* path)
    {
        node_t* dir_node = walk_path(instance, path);
        // forward error
        if(IS_ERR_PTR(dir_node)) return ERR_CAST(dir_node);
        // the node is not a directory or a device, return error
        if((dir_node->flags & NODE_TYPE_MASK) == NODE_DEVICE) return EINVPATH;

        remove_child(dir_node);

        event_t event;
        event.driver_uid = dir_node->driver_uid;
        event.flags = EVENT_DEVICE_DEL;
        event.trigger_node = dir_node;

        add_event(instance, event);

        return OP_SUCCESS;
    }
    // add a device node at path
    i32 add_bnode(vfs_t* instance, const char* path, node_t blk_node)
    {
        node_t* dir_node = walk_path(instance, path);
        // forward error
        if(IS_ERR_PTR(dir_node)) return ERR_CAST(dir_node);
        // the node is not a directory or a device, return error
        if((dir_node->flags & NODE_DIRECTORY) == 0) return EINVPATH;

        node_t* child = new node_t(blk_node);
        init_child(instance, child);
        add_child(dir_node, child);

        event_t event;
        event.driver_uid = blk_node.driver_uid;
        event.flags = EVENT_BLOCK_ADD;
        event.trigger_node = child;

        add_event(instance, event);

        return OP_SUCCESS;
    }
    // remove a device node at path
    i32 remove_bnode(vfs_t* instance, const char* path)
    {
        node_t* dir_node = walk_path(instance, path);
        // forward error
        if(IS_ERR_PTR(dir_node)) return ERR_CAST(dir_node);
        // the node is not a directory or a device, return error
        if((dir_node->flags & NODE_TYPE_MASK) == NODE_BLOCK) return EINVPATH;

        remove_child(dir_node);

        event_t event;
        event.driver_uid = dir_node->driver_uid;
        event.flags = EVENT_DEVICE_DEL;
        event.trigger_node = dir_node;

        add_event(instance, event);

        return OP_SUCCESS;
    }

    u32 set_node_ready(vfs_t* instance, node_t* node, uid_t driver_uid)
    {
        // validate the node
        if(!(node->flags == NODE_DEVICE || node->flags == NODE_BLOCK)) return EINVARG;

        // generate the event
        event_t event;
        event.driver_uid = driver_uid;
        event.trigger_node = node;
        event.flags = EVENT_INVALID;

        if(node->flags == NODE_DEVICE) event.flags = EVENT_DEVICE_READY;
        else if(node->flags == NODE_BLOCK) event.flags = EVENT_BLOCK_READY;

        // add the event
        add_event(instance, event);

        return u32();
    }

    void log_children(vfs_t *instance, const char *path)
    {
        node_t* node = get_node(instance, path);

        if(IS_ERR_PTR(node)) return;
        if((node->flags & NODE_DIRECTORY) == 0) return;

        node = node->children_head;
        while(node != nullptr)
        {
            log_info("%10s size:%u B\n", node->name, node->size);
            node = node->next;
        }
    }

    void free_node(node_t *node)
    {
        // TODO: maybe add a check to see if the node has freed all it's 
        // children
        delete node;
    }
    
    bool poll(vfs_t *instance)
    {
        return instance->queue_head != nullptr;
    }
    event_t pop_event(vfs_t *instance)
    {
        // remove a event from the queue
        if(instance->queue_head == nullptr) return event_t{.flags=EVENT_INVALID};

        event_t poped_event = instance->queue_head->event;

        delete instance->queue_head;

        // pop the event from queue
        instance->queue_head = instance->queue_head->next;
        if(instance->queue_head == nullptr)
        {
            instance->queue_tail = instance->queue_head;
        }

        return poped_event;
    }
}
