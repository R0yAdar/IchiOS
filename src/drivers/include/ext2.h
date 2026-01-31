
#ifndef EXT2_H
#define EXT2_H

/*
    EXT2 FILESYSTEM Driver
    Only supports LE systems, because who cares.
*/

#include "types.h"
#include "vmm.h"
#include "serial.h"
#include "vfs.h"
#include "math.h"

#define EXT2_MAGIC 0xEF53
#define EXT2_SYSTEM_STATE_CLEAN 0x1
#define EXT2_SYSTEM_STATE_ERR 0x2

#define EXT2_IGNORE_ERR 0x1
#define EXT2_REMOUNT_FS 0x2
#define EXT2_KERNEL_PANIC 0x3

#define EXT2_OS_LINUX 0
#define EXT2_OS_HURD 1
#define EXT2_OS_MASIX 2
#define EXT2_OS_FREEBSD 3
#define EXT2_OS_LITES 4

#define EXT2_OPT_PREALLOC 0x1
#define EXT2_OPT_AFS_SERVER_INODES 0x2
#define EXT2_OPT_JOURNAL 0x4
#define EXT2_OPT_EXTENDED_ATTRS 0x8
#define EXT2_OPT_RESIZEABLE_FS 0x10
#define EXT2_OPT_HASH_DIR_INDEX 0x20

#define EXT2_REQ_COMPRESSION 0x1
#define EXT2_REQ_DIR_TYPE 0x2
#define EXT2_REQ_JOURNAL_REPLAY 0x4
#define EXT2_REQ_JOURNAL_DEV 0x8

#define EXT2_RO_SPARSE 0x1
#define EXT2_RO_64_BIT_FILE_SIZES 0x2
#define EXT2_RO_BINARY_TREE_DIR 0x4

#define EXT2_INODE_TYPE_FIFO 0x1000
#define EXT2_INODE_TYPE_CHAR_DEV 0x2000
#define EXT2_INODE_TYPE_DIRECTORY 0x4000
#define EXT2_INODE_TYPE_BLOCK_DEV 0x6000
#define EXT2_INODE_TYPE_REGULAR 0x8000
#define EXT2_INODE_TYPE_SYMLINK 0xA000
#define EXT2_INODE_TYPE_SOCKET 0xC000

#define EXT2_INODE_PERM_O_EXEC 0x001
#define EXT2_INODE_PERM_O_WRITE 0x002
#define EXT2_INODE_PERM_O_READ 0x004
#define EXT2_INODE_PERM_G_EXEC 0x008
#define EXT2_INODE_PERM_G_WRITE 0x010
#define EXT2_INODE_PERM_G_READ 0x020
#define EXT2_INODE_PERM_U_EXEC 0x040
#define EXT2_INODE_PERM_U_WRITE 0x080
#define EXT2_INODE_PERM_U_READ 0x100
#define EXT2_INODE_PERM_STICKY 0x200
#define EXT2_INODE_PERM_SET_GID 0x400
#define EXT2_INODE_PERM_SET_UID 0x800

#define EXT2_TYPE_UNKNOWN 0
#define EXT2_TYPE_REGULAR_FILE 1
#define EXT2_TYPE_DIRECTORY 2
#define EXT2_TYPE_CHAR_DEVICE 3
#define EXT2_TYPE_BLOCK_DEVICE 4
#define EXT2_TYPE_FIFO 5
#define EXT2_TYPE_SOCKET 6
#define EXT2_TYPE_SYMLINK 7

#pragma pack(push, 1)

typedef struct
{
    uint32_t inodes_count;
    uint32_t blocks_count;
    uint32_t r_blocks_count;
    uint32_t free_blocks_count;
    uint32_t free_inodes_count;
    uint32_t first_data_block;
    uint32_t log_block_size; // log2 (block size) - 10. (In other words, the number to shift 1,024 to the left by to obtain the block size)
    uint32_t log_frag_size;  // log2 (fragment size) - 10. (In other words, the number to shift 1,024 to the left by to obtain the fragment size)
    uint32_t blocks_per_group;
    uint32_t frags_per_group;
    uint32_t inodes_per_group;
    uint32_t mtime; // POSIX time
    uint32_t wtime; // POSIX time
    uint16_t mnt_count;
    uint16_t max_mnt_count;
    uint16_t magic; // 0xEF53
    uint16_t state;
    uint16_t errors;
    uint16_t minor_ver_level;
    uint32_t lastcheck;
    uint32_t checkinterval;
    uint32_t creator_os;
    uint32_t major_ver_level;
    uint16_t uid;
    uint16_t gid;
} superblock;

typedef struct
{
    uint32_t first_non_resv_inode; // default: 11
    uint16_t inode_size;           // default: 128
    uint16_t block_group_nr;
    uint32_t opt_features;
    uint32_t req_features;
    uint32_t ro_features;
    uint8_t filesystem_id[16];
    char volume_name[16];
    char last_mounted[64];
    uint32_t compression_info;
    uint8_t file_preallo_blocks_nr;
    uint8_t dir_preallo_blocks_nr;
    uint16_t unused1;
    uint8_t journal_id[16];
    uint32_t journal_inode;
    uint32_t journal_device;
    uint32_t last_orphan;
} extended_superblock;

typedef struct
{
    uint32_t block_bitmap_ba;
    uint32_t inode_bitmap_ba;
    uint32_t inode_table_ba;
    uint16_t free_blocks_count;
    uint16_t free_inodes_count;
    uint16_t dirs_nr;
    uint8_t unused[14];
} ext2_bgd;

typedef struct
{
    uint16_t type_perms;
    uint16_t uid;
    uint32_t size_lower32;
    uint32_t last_access_time;
    uint32_t creation_time;
    uint32_t modification_time;
    uint32_t deletion_time;
    uint16_t gid;
    uint16_t hard_links_count;
    uint32_t used_sectors_count;
    uint32_t flags;
    uint32_t os_specific1;
    uint32_t direct_bp[12]; // block pointers...
    uint32_t singly_indirect_bp;
    uint32_t doubly_indirect_bp;
    uint32_t triply_indirect_bp;
    uint32_t generation; // NFS
    uint32_t file_acl;
    uint32_t dir_acl_or_size_upper32; // dir/file
    uint32_t fragment_ba;
    uint8_t os_specific2[12];
} ext2_inode;

typedef struct
{
    uint32_t inode;
    uint16_t size;
    uint8_t name_len;
    uint8_t file_type; // (type indicator if feature supported)
    char name[1];
} ext2_dir_entry;

#pragma pack(pop)

typedef struct ext2_context ext2_context;

BOOL ext2_parse_superblock(ext2_context *ctx);

uint32_t ext2_locate_inode_group(ext2_context *ctx, uint32_t inode);

uint32_t ext2_locate_inode_index(ext2_context *ctx, uint32_t inode);

BOOL ext2_read_bgdt(ext2_context *ctx, uint32_t index, ext2_bgd *out);

ext2_context *ext2_init(io_device *device);

BOOL ext2_read_inode(ext2_context *ctx, ext2_bgd *bgd, uint32_t index, ext2_inode *out);

void *ext2_read_block(ext2_context *ctx, ext2_inode *inode, uint64_t block_number, io_buffer *buffer);

BOOL ext2_get_inode(ext2_context *ctx, uint32_t inode_nr, ext2_inode *out);

void ext2_release(ext2_context *ctx);

BOOL ext2_open(ext2_context *ctx, uint32_t inode_nr, vnode *out);

BOOL ext2_open_entry(ext2_context *ctx, ventry *entry, vnode *out);

BOOL ext2_open_root(ext2_context *ctx, vnode *out);

BOOL ext2_readdir(ext2_context *ctx, vnode *dir, ventry *out);

void ext2_closedir(vnode *dir);

void ext2_close(vnode *node);

uint64_t ext2_readfile(ext2_context *ctx, vnode *node, void *buffer, uint64_t len);

void ext2_root(ext2_context *ctx);

extern vfs_ops ext2_ops;

#endif