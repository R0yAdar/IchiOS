/*
    EXT2 FILESYSTEM Driver
    Only supports LE systems, because who cares.
*/

#include "vmm.h"
#include "serial.h"
#include "math.h"
#include "array.h"
#include "cstring.h"
#include "ext2.h"

/// EXT2 FILESYSTEM INTERNAL STRUCTURES

struct ext2_context {
    io_device* _device;
    io_buffer* _buffer;
    superblock _sb;
    extended_superblock _esb;
    uint32_t block_size;
    uint32_t sectors_per_block;
};

typedef struct {
    ext2_inode* inode;
    uint32_t position;
} ext2_vnode_data;

/// EXT2 FILESYSTEM IMPLEMENTATION

BOOL ext2_parse_superblock(ext2_context* ctx) {
    // read superblock (sectors 2-3)
    void* data = io_read(ctx->_device, ctx->_buffer, 2, 2);
    
    if (!data) return FALSE;

    superblock* sb = (superblock*)data;
    extended_superblock* esb = (extended_superblock*)(sb + 1);

    if (sb->magic != EXT2_MAGIC) return FALSE;

    ctx->_sb = *sb;

    if (sb->major_ver_level < 1) {
        qemu_log("EXT2 major version is 0");
        esb->inode_size = 128;
    }

    ctx->_esb = *esb;

    return TRUE;
}

uint32_t ext2_locate_inode_group(ext2_context* ctx, uint32_t inode) {
    return (inode - 1) / ctx->_sb.inodes_per_group;
}

uint32_t ext2_locate_inode_index(ext2_context* ctx, uint32_t inode) {
    return (inode - 1) % ctx->_sb.inodes_per_group;
}

BOOL ext2_read_bgdt(ext2_context* ctx, uint32_t index, ext2_bgd* out) {
    // entry size is 32 bytes so -> 16 entries per sector

    uint32_t bgd_addr = 0;

    if (ctx->block_size == 1024) {
        // the first block is the bootsector
        bgd_addr += (1024 + 1024);
    }
    else {
        // the first block is the bootsector + superblock
        bgd_addr += ctx->block_size;
    }

    uint32_t bgd_count = (
        (ctx->_sb.blocks_count + ctx->_sb.blocks_per_group - 1) / ctx->_sb.blocks_per_group
    );

    if (index >= bgd_count) return FALSE;

    bgd_addr += index * sizeof(ext2_bgd);

    uint64_t lba = bgd_addr / SECTOR_SIZE;
    uint64_t bgd_offset = (bgd_addr % SECTOR_SIZE) / sizeof(ext2_bgd);

    ext2_bgd* bgdt = io_read(
        ctx->_device, 
        ctx->_buffer, 
        lba,
        1);

    if (!bgdt) return FALSE;

    (*out) = *(bgdt + bgd_offset);

    return TRUE;
}

ext2_context* ext2_init(io_device* device) {
    ext2_context* ctx = kmalloc(sizeof(ext2_context));
    BOOL res;

    ctx->_device = device;
    ctx->_buffer = io_alloc_buffer(1);

    res = ext2_parse_superblock(ctx);

    if (!res) return ctx;

    ctx->block_size = 1024 << ctx->_sb.log_block_size;
    ctx->sectors_per_block = ctx->block_size / SECTOR_SIZE;

    return ctx;
}

BOOL ext2_read_inode(ext2_context* ctx, ext2_bgd* bgd, uint32_t index, ext2_inode* out) {
    uint32_t inode_addr = bgd->inode_table_ba * (1024 << ctx->_sb.log_block_size);
    inode_addr += index * ctx->_esb.inode_size;

    uint64_t lba = inode_addr / SECTOR_SIZE;
    uint64_t inode_offset = inode_addr % SECTOR_SIZE;

    uint8_t* inodes = io_read(
        ctx->_device, 
        ctx->_buffer, 
        lba,
        1);
    
    if (!inodes) return FALSE;

    (*out) = *(ext2_inode*)(inodes + inode_offset);

    return TRUE;
}

void* load_indirect_block(ext2_context* ctx, uint32_t indirect_block_address) {
    uint64_t lba = indirect_block_address * ctx->sectors_per_block;
    
    return (uint32_t*)io_read(
        ctx->_device, 
        ctx->_buffer,
        lba,
        ctx->sectors_per_block);
    
}

void* read_indirect_block(ext2_context* ctx, uint32_t* bps, uint32_t index, io_buffer* buffer) {
    if (!bps) return NULL;
    if (bps[index] == 0) return NULL;

    uint64_t lba = bps[index] * ctx->sectors_per_block;

    return io_read(
        ctx->_device,
        buffer ? buffer : ctx->_buffer,
        lba,
        ctx->sectors_per_block
    );
}


void* ext2_read_block(ext2_context* ctx, ext2_inode* inode, uint64_t block_number, io_buffer* buffer) {
    // assert block_number < int32.max
    const uint32_t DIRECT_BLOCKS_COUNT = 12;

    const uint32_t SINGLY_INDIRECT_BLOCKS_COUNT = ctx->block_size / sizeof(uint32_t);
    const uint32_t DOUBLY_INDIRECT_BLOCKS_COUNT = SINGLY_INDIRECT_BLOCKS_COUNT * SINGLY_INDIRECT_BLOCKS_COUNT;
    const uint32_t TRIPLY_INDIRECT_BLOCKS_COUNT = SINGLY_INDIRECT_BLOCKS_COUNT * DOUBLY_INDIRECT_BLOCKS_COUNT;

    if (block_number >= DIRECT_BLOCKS_COUNT) {
        uint32_t relative_block = block_number - DIRECT_BLOCKS_COUNT;

        // case 1: singly indirect block
        if (relative_block < SINGLY_INDIRECT_BLOCKS_COUNT) {
            return read_indirect_block(ctx, load_indirect_block(ctx, inode->singly_indirect_bp), relative_block, buffer);
        }

        relative_block -= SINGLY_INDIRECT_BLOCKS_COUNT;

        // case 2: doubly indirect block
        if (relative_block < DOUBLY_INDIRECT_BLOCKS_COUNT) {
            void* level1 = read_indirect_block(ctx, load_indirect_block(ctx, inode->doubly_indirect_bp), relative_block / SINGLY_INDIRECT_BLOCKS_COUNT, NULL);
            if (!level1) return NULL;
            return read_indirect_block(ctx, level1, relative_block % SINGLY_INDIRECT_BLOCKS_COUNT, buffer);
        }

        relative_block -= DOUBLY_INDIRECT_BLOCKS_COUNT;

        // case 3: triply indirect block
        if (relative_block < TRIPLY_INDIRECT_BLOCKS_COUNT) {
            void* level1 = read_indirect_block(ctx, load_indirect_block(ctx, inode->triply_indirect_bp), relative_block / DOUBLY_INDIRECT_BLOCKS_COUNT, NULL);
            if (!level1) return NULL;
            relative_block = relative_block % DOUBLY_INDIRECT_BLOCKS_COUNT;
            void* level2 = read_indirect_block(ctx, level1, relative_block / SINGLY_INDIRECT_BLOCKS_COUNT, NULL);
            if (!level2) return NULL;
            return read_indirect_block(ctx, level2, relative_block % SINGLY_INDIRECT_BLOCKS_COUNT, buffer);
        }
    } else {
        if (io_get_size(buffer) < ctx->block_size) return NULL;
        if (inode->direct_bp[block_number] == 0) return NULL;

        uint64_t lba = inode->direct_bp[block_number] * ctx->sectors_per_block;

        return io_read(
            ctx->_device, 
            buffer, 
            lba, 
            ctx->sectors_per_block);
    }

    return NULL;
}

BOOL ext2_get_inode(ext2_context* ctx, uint32_t inode_nr, ext2_inode* out) {
    uint32_t inode_bgrp = ext2_locate_inode_group(ctx, inode_nr);
    uint32_t inode_index = ext2_locate_inode_index(ctx, inode_nr);

    ext2_bgd bgd;

    if (!ext2_read_bgdt(ctx, inode_bgrp, &bgd)) {
        return FALSE;
    }

    ext2_inode inode;
    
    if (!ext2_read_inode(ctx, &bgd, inode_index, &inode)) {
        return FALSE;
    }

    (*out) = inode;
    
    return TRUE;
}

void ext2_release(ext2_context* ctx) {
    io_release_buffer(ctx->_buffer);
    kfree(ctx);
}


BOOL ext2_open(ext2_context* ctx, uint32_t inode_nr, vnode* out) {
    ext2_inode* inode = (ext2_inode*)kmalloc(sizeof(ext2_inode));
    if(!inode) return FALSE;
    
    ext2_vnode_data* data = (ext2_vnode_data*)kmalloc(sizeof(ext2_vnode_data));
    if(!data) return FALSE;

    data->inode = inode;
    data->position = 0;

    if (!ext2_get_inode(ctx, inode_nr, inode)) {
        return FALSE;
    }

    out->data = (void*)data;
    out->size = inode->size_lower32;
    out->buffer = io_alloc_buffer(2); // make page count appropriate to double block size

    return TRUE;
}

BOOL ext2_open_entry(ext2_context* ctx, ventry* entry, vnode* out) {
    return ext2_open(ctx, entry->id, out);
}

BOOL ext2_open_root(ext2_context* ctx, vnode* out) {
    return ext2_open(ctx, 2, out);
}

BOOL ext2_readdir(ext2_context* ctx, vnode* dir, ventry* out) {
    ext2_vnode_data* data = (ext2_vnode_data*)dir->data;

    if (data->position >= dir->size) {
        return FALSE;
    }
    
    if (data->position % ctx->block_size == 0) {
        uint8_t* block = (uint8_t*)ext2_read_block(ctx, data->inode, data->position / ctx->block_size, dir->buffer);

        if (!block) {
            return FALSE;
        }
    }

    uint16_t block_position = data->position % ctx->block_size;

    ext2_dir_entry* entry = (ext2_dir_entry*)((uint8_t*)io_get(dir->buffer) + block_position);
    
    data->position += entry->size;

    for (size_t i = 0; i < entry->name_len; i++)
    {
        out->name[i] = entry->name[i];
    }

    out->name[entry->name_len] = '\0';
    out->id = entry->inode;

    return TRUE;
}

void ext2_closedir(vnode* dir) {
    ext2_close(dir);
}

void ext2_close(vnode* node) {
    ext2_vnode_data* data = (ext2_vnode_data*)node->data;
    
    kfree(data->inode);
    kfree(node->data);
    kfree(node->name);
    io_release_buffer(node->buffer);
}

uint64_t ext2_readfile(ext2_context* ctx, vnode* node, void* buffer, uint64_t len) {
    ext2_vnode_data* data = (ext2_vnode_data*)node->data;
    uint32_t first_block = data->position / ctx->block_size;
    len = min(len, node->size - data->position);
    uint32_t end_block = (data->position + len) / ctx->block_size;
    uint64_t buf_pos = 0;

    uint32_t first_block_offset = data->position % ctx->block_size;
    
    uint8_t* block = (uint8_t*)ext2_read_block(ctx, data->inode, first_block, node->buffer);
    buf_pos += copy(block + first_block_offset, buffer, min(ctx->block_size - first_block_offset, len));

    for (uint32_t i = first_block + 1; i < end_block; i++) {
        uint8_t* block = (uint8_t*)ext2_read_block(ctx, data->inode, i, node->buffer);
        buf_pos += copy_to(block, buffer, buf_pos, ctx->block_size);
    }

    if (first_block != end_block) {
        uint8_t* block = (uint8_t*)ext2_read_block(ctx, data->inode, end_block, node->buffer);
        copy_to(block, buffer, buf_pos, len - buf_pos);
    }

    data->position += len;

    return len;
}

uint64_t ext2_seek(vnode* node, uint64_t offset, seek_mode whence) {
    ext2_vnode_data* data = (ext2_vnode_data*)node->data;

    if (whence == SEEK_SET) {
        if (offset > node->size) {
            return -1;
        }

        data->position = offset;
    }
    else if (whence == SEEK_CUR) {
        if (data->position + offset > node->size) {
            return -1;
        }

        data->position += offset;
    }
    else if (whence == SEEK_END) {
        if (data->position + offset > node->size) {
            return -1;
        }

        data->position = node->size + offset;
    }

    return 0;
}

uint64_t ext2_tell(vnode* node) {
    return ((ext2_vnode_data*)node->data)->position;
}

vfs_ops ext2_ops = {
    .open_root = (open_root_callback)ext2_open_root,
    .open = (open_callback)ext2_open_entry,
    .readdir = (readdir_callback)ext2_readdir,
    .readfile = (readfile_callback)ext2_readfile,
    .seek = (seek_callback)ext2_seek,
    .tell = (tell_callback)ext2_tell,
    .close = (close_callback)ext2_close
};