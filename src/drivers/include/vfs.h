#ifndef VFS_H
#define VFS_H

#include "types.h"

/*
    IO DEVICE
*/

typedef BOOL(*reader_t)(uint64_t lba, uint32_t sector_count, void* pbuffer);
typedef BOOL(*writer_t)(uint64_t lba, uint32_t sector_count, void* pbuffer);

typedef struct {
    uint64_t start_lba;
    uint64_t end_lba;
    reader_t read;
    writer_t write;
} io_device;

typedef struct io_buffer io_buffer;

uint64_t io_get_size(io_buffer* buffer);

io_buffer* io_alloc_buffer(uint64_t page_count);

void* io_read(io_device* dev, io_buffer* buffer, uint64_t lba, uint64_t sector_count);

void* io_get(io_buffer* buffer);

void io_release_buffer(io_buffer* buffer);

/*
    VFS
*/

typedef enum  {
    READ,
} file_access_mode;

typedef struct {
    char* name;
    void* data;
    io_buffer* buffer;
    uint64_t size;
    uint32_t flags;
} vnode;

typedef struct {
    char name[256];
    uint64_t id;
} ventry;

typedef struct mounted_fs mounted_fs;

typedef struct {
    vnode* vnode;
    uint64_t size;
    file_access_mode mode;
    mounted_fs* mountpoint;
} file;

typedef struct {
    void* vnode;
} dir;

typedef struct {
    void* vnode;
} dir_entry;

typedef enum {
    SEEK_SET = 0,
    SEEK_CUR = 1,
    SEEK_END = 2
} seek_mode;

dir* opendir(const char* path);

dir_entry* readdir(dir* d);

void closedir(dir* d);

file* fopen(const char* path, file_access_mode mode);

size_t fread(void* buffer, uint64_t size, uint64_t count, file* f);

size_t fwrite(void* buffer, uint64_t size, uint64_t count, file* f);

uint64_t fseek(file* f, uint64_t offset, seek_mode whence);

uint64_t ftell(file* f);

void fclose(file* f);

typedef BOOL(*open_root_callback)(void* ctx, vnode* out);
typedef BOOL(*open_callback)(void* ctx, ventry* entry, vnode* out);
typedef BOOL(*readdir_callback)(void* ctx, vnode* node, ventry* out);
typedef uint64_t (*readfile_callback)(void* ctx, vnode* node, void* buffer, uint64_t len);
typedef uint64_t (*seek_callback)(vnode* node, uint64_t offset, seek_mode whence);
typedef uint64_t (*tell_callback)(vnode* node);
typedef void(*close_callback)(vnode* node);


typedef struct {
    open_root_callback open_root;
    open_callback open;
    readdir_callback readdir;
    readfile_callback readfile;
    seek_callback seek;
    tell_callback tell;
    close_callback close;
} vfs_ops;

typedef struct {
    void* context;
    vfs_ops* ops;
} file_system;

BOOL mount(file_system fs, const char* path);

#endif