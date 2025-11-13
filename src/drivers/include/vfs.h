#ifndef VFS_H
#define VFS_H

#include "types.h"

typedef BOOL(*reader_t)(uint64_t lba, uint32_t sector_count, void* pbuffer);
typedef BOOL(*writer_t)(uint64_t lba, uint32_t sector_count, void* pbuffer);

typedef struct io_buffer io_buffer;

typedef struct {
    uint64_t start_lba;
    uint64_t end_lba;
    reader_t read;
    writer_t write;
} io_device;

uint64_t io_get_size(io_buffer* buffer);

io_buffer* io_alloc_buffer(uint64_t page_count);

void* io_read(io_device* dev, io_buffer* buffer, uint64_t lba, uint64_t sector_count);

void* io_get(io_buffer* buffer);

void io_release_buffer(io_buffer* buffer);

typedef enum  {
    READ,
} file_access_mode;

typedef struct {
    char* name;
    void* data;
    io_buffer* buffer;
    uint64_t size;
} vnode;

typedef struct {
    char name[257];
    uint64_t id;
} ventry;

typedef struct {
    void** vnode;
    uint64_t size;
    uint64_t position;
    file_access_mode mode;
} file;

typedef struct {
    void** vnode;
} dir;

typedef struct {
    void** vnode;
} dir_entry;

dir* opendir(const char* path);

dir_entry* readdir(dir* d);

void closedir(dir* d);

file* fopen(const char* path, file_access_mode mode);

size_t fread(void* buffer, uint64_t size, uint64_t count, file* f);

size_t fwrite(void* buffer, uint64_t size, uint64_t count, file* f);

void fclose(file* f);

typedef dir*(*opendir_t)(const char* path);
typedef dir_entry*(*readdir_t)(const char* path);
typedef void(closedir_t)(const char* path);
typedef file*(*fopen_t)(const char* path, file_access_mode mode);
typedef size_t(*fread_t)(void* buffer, uint64_t size, uint64_t count, file* f);
typedef size_t(*fwrite_t)(void* buffer, uint64_t size, uint64_t count, file* f);
typedef void(*fclose_t)(file* f);

typedef struct
{
    opendir_t*  opendir;
    readdir_t*  readdir;
    closedir_t* closedir;
    fopen_t*    fopen;
    fread_t*    fread;
    fwrite_t*   fwrite;
    fclose_t*   fclose;
} vnodeops;

typedef struct vnode
{
    char path[256];
};


typedef struct {
    void* device;
    void* context;
    vnodeops* ops;
} file_system;

// resort to vnode!

char mount(file_system fs);

#endif