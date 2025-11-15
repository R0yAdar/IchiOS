#include "vfs.h"
#include "vmm.h"
#include "str.h"

#define MAX_MOUNTPOINTS 10
#define VFS_SEPERATOR '/'

/*
    VFS
*/

struct mounted_fs
{
    BOOL is_mounted;
    char mount_path[256];
    file_system fs;
    uint64_t rc;
};


mounted_fs mountpoints[MAX_MOUNTPOINTS] = {0};

BOOL mount(file_system fs, const char* path) {
    uint8_t mount_index = 0;

    if (mountpoints[MAX_MOUNTPOINTS - 1].is_mounted) return FALSE;

    while (mountpoints[mount_index].is_mounted && strlen(path) > strlen(mountpoints[mount_index].mount_path))
    {
        ++mount_index;
    }

    for (size_t i = MAX_MOUNTPOINTS - 1; i > mount_index; --i)
    {
        mountpoints[i] = mountpoints[i - 1];
    }    

    mountpoints[mount_index] = (mounted_fs){.is_mounted = TRUE, .fs = fs};
    strcpy(mountpoints[mount_index].mount_path, path);
    
    return TRUE;
}

mounted_fs* find_mountpoint(const char* path) {
    for (size_t i = 0; i < MAX_MOUNTPOINTS; ++i)
    {
        if (mountpoints[i].is_mounted && strstr(path, mountpoints[i].mount_path) == path) {
            return &mountpoints[i];
        }
    }

    return NULL;
}

vnode* traverse_path(mounted_fs* mountpoint, const char* path) {
    vnode node;
    ventry entry;

    if (!mountpoint->fs.ops->open_root(mountpoint->fs.context, &node)) return NULL;

    const char* seperator = strchr(path, VFS_SEPERATOR);
    
    while (seperator)
    {
        while (!mountpoint->fs.ops->readdir(mountpoint->fs.context, &node, &entry))
        {
            if (strncmp(entry.name, path, seperator - path) == 0) {
                break;
            }
        }

        if (!strncmp(entry.name, path, seperator - path) == 0) {
            return NULL;
        }

        path = seperator + 1;
        seperator = strchr(path, VFS_SEPERATOR);
        mountpoint->fs.ops->close(&node);
        if (!mountpoint->fs.ops->open(mountpoint->fs.context, &entry, &node)) return NULL;
    }

    while (mountpoint->fs.ops->readdir(mountpoint->fs.context, &node, &entry))
    {
        if (strcmp(entry.name, path) == 0) {
            mountpoint->fs.ops->close(&node);

            vnode* out = (vnode*)kmalloc(sizeof(vnode));

            mountpoint->fs.ops->open(mountpoint->fs.context, &entry, out);

            return out;
        }
    }

    return NULL;
}

dir* opendir(const char* path) {
    mounted_fs* mountpoint = find_mountpoint(path);
    if (!mountpoint) return NULL;

    vnode* node = traverse_path(mountpoint, path + strlen(mountpoint->mount_path));
    dir* out = (dir*)kmalloc(sizeof(dir));
    out->vnode = node;

    ++mountpoint->rc;

    return out;
}

file* fopen(const char* path, file_access_mode mode) {
    mounted_fs* mountpoint = find_mountpoint(path);
    if (!mountpoint) return NULL;

    vnode* node = traverse_path(mountpoint, path + strlen(mountpoint->mount_path));

    file* out = (file*)kmalloc(sizeof(file));
    out->vnode = node;
    out->mode = mode;
    out->mountpoint = mountpoint;
    
    ++mountpoint->rc;

    return out;
}

size_t fread(void* buffer, uint64_t size, uint64_t count, file* f) {
    if (f->mode != READ) return 0;
    if (!f || !buffer) return 0;

    return f->mountpoint->fs.ops->readfile(f->mountpoint->fs.context, f->vnode, buffer, size * count);
}

void fclose(file* f) {
    if (!f) return;
    --f->mountpoint->rc;
    kfree(f->vnode);
    kfree(f);
}

dir_entry* readdir(dir* d);

void closedir(dir* d);


/*
    IO BUFFER
*/

struct io_buffer {
    void* vaddr;
    void* _phys;
    uint64_t _size;
};

io_buffer* io_alloc_buffer(uint64_t page_count) {
    void* phys;
    void* vaddr = kpage_alloc_dma(page_count, &phys);
    io_buffer* buffer = (io_buffer*)kmalloc(sizeof(io_buffer));

    if (vaddr == NULL) return buffer;

    buffer->vaddr = vaddr;
    buffer->_phys = phys;
    buffer->_size = page_count * PAGE_SIZE;

    return buffer;
}

uint64_t io_get_size(io_buffer* buffer) {
    return buffer->_size;
}

void* io_read(io_device* dev, io_buffer* buffer, uint64_t lba, uint64_t sector_count) {
    if (buffer->_size < sector_count * SECTOR_SIZE) return NULL;

    if (dev->read(lba + dev->start_lba, sector_count, buffer->_phys)) {
        return buffer->vaddr;
    }

    return NULL;
}

void* io_get(io_buffer* buffer) {
    return buffer->vaddr;
}

void io_release_buffer(io_buffer* buffer) {
    kpage_free_dma(buffer->_size / PAGE_SIZE, buffer->vaddr, buffer->_phys);
    kfree(buffer);
}
