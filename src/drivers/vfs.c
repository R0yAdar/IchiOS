#include "vfs.h"
#include "vmm.h"

#define MAX_MOUNTPOINTS 27

/*
    VFS
*/

/*
    out->name = (char*)kmalloc(2);
    out->name[0] = '/';
    out->name[1] = '\0';
*/

file_system mountpoints[MAX_MOUNTPOINTS] = {0};


char mount(file_system fs) {
    uint8_t i = 0;

    while (mountpoints[i].device != NULL)
    {
        if (i == MAX_MOUNTPOINTS) return 0;

        ++i;
    }

    mountpoints[i] = fs;
    return 'A' + i;
}

dir* opendir(const char* path) {
    mountpoints[path[0] - 'A'];

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
    kpage_free(buffer->vaddr, buffer->_size / PAGE_SIZE);
    kfree(buffer);
}




