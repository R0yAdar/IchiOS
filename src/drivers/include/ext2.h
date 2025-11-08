/*
    EXT2 FILESYSTEM Driver
    Only supports LE systems, because who cares.
*/

#include "types.h"
#include "vmm.h"
#include "serial.h"
#include "print.h"

typedef BOOL(*reader_t)(uint64_t lba, uint32_t sector_count, void* pbuffer);
typedef BOOL(*writer_t)(uint64_t lba, uint32_t sector_count, void* pbuffer);

typedef struct {
    uint64_t start_lba;
    uint64_t end_lba;
    reader_t read;
    writer_t write;
} io_device;

typedef struct {
    void* vaddr;
    void* _phys;
    uint64_t _size;
} io_buffer;

io_buffer io_alloc_buffer(uint64_t page_count) {
    void* phys;
    void* vaddr = kpage_alloc_dma(page_count, &phys);
    io_buffer buffer = {0};

    if (vaddr == NULL) return buffer;

    buffer.vaddr = vaddr;
    buffer._phys = phys;
    buffer._size = page_count * PAGE_SIZE;

    return buffer;
}

void* io_read(io_device* dev, io_buffer* buffer, uint64_t lba, uint64_t sector_count) {
    if (buffer->_size < sector_count * SECTOR_SIZE) return NULL;

    if (dev->read(lba + dev->start_lba, sector_count, buffer->_phys)) {
        return buffer->vaddr;
    }

    return NULL;
}

void io_release_buffer(io_buffer* buffer) {
}




#define EXT2_MAGIC 0xEF53
#define EXT2_SYSTEM_STATE_CLEAN 0x1
#define EXT2_SYSTEM_STATE_ERR   0x2

#define EXT2_IGNORE_ERR         0x1
#define EXT2_REMOUNT_FS         0x2
#define EXT2_KERNEL_PANIC       0x3

#define EXT2_OS_LINUX           0
#define EXT2_OS_HURD            1
#define EXT2_OS_MASIX           2
#define EXT2_OS_FREEBSD         3
#define EXT2_OS_LITES           4

#define EXT2_OPT_PREALLOC               0x1
#define EXT2_OPT_AFS_SERVER_INODES      0x2
#define EXT2_OPT_JOURNAL                0x4
#define EXT2_OPT_EXTENDED_ATTRS         0x8
#define EXT2_OPT_RESIZEABLE_FS          0x10
#define EXT2_OPT_HASH_DIR_INDEX         0x20

#define EXT2_REQ_COMPRESSION            0x1
#define EXT2_REQ_DIR_TYPE               0x2
#define EXT2_REQ_JOURNAL_REPLAY         0x4
#define EXT2_REQ_JOURNAL_DEV            0x8

#define EXT2_RO_SPARSE                  0x1
#define EXT2_RO_64_BIT_FILE_SIZES       0x2
#define EXT2_RO_BINARY_TREE_DIR         0x4

#define EXT2_INODE_TYPE_FIFO            0x1000
#define EXT2_INODE_TYPE_CHAR_DEV        0x2000
#define EXT2_INODE_TYPE_DIRECTORY       0x4000
#define EXT2_INODE_TYPE_BLOCK_DEV       0x6000
#define EXT2_INODE_TYPE_REGULAR         0x8000
#define EXT2_INODE_TYPE_SYMLINK         0xA000
#define EXT2_INODE_TYPE_SOCKET          0xC000

#define EXT2_INODE_PERM_O_EXEC            0x001
#define EXT2_INODE_PERM_O_WRITE           0x002
#define EXT2_INODE_PERM_O_READ            0x004
#define EXT2_INODE_PERM_G_EXEC            0x008
#define EXT2_INODE_PERM_G_WRITE           0x010
#define EXT2_INODE_PERM_G_READ            0x020
#define EXT2_INODE_PERM_U_EXEC            0x040
#define EXT2_INODE_PERM_U_WRITE           0x080
#define EXT2_INODE_PERM_U_READ            0x100
#define EXT2_INODE_PERM_STICKY            0x200
#define EXT2_INODE_PERM_SET_GID           0x400
#define EXT2_INODE_PERM_SET_UID           0x800

#define EXT2_TYPE_UNKNOWN           0
#define EXT2_TYPE_REGULAR_FILE      1
#define EXT2_TYPE_DIRECTORY         2
#define EXT2_TYPE_CHAR_DEVICE       3
#define EXT2_TYPE_BLOCK_DEVICE      4
#define EXT2_TYPE_FIFO              5
#define EXT2_TYPE_SOCKET            6
#define EXT2_TYPE_SYMLINK           7

#pragma pack (push, 1)

typedef struct {
    uint32_t inodes_count;
    uint32_t blocks_count;
    uint32_t r_blocks_count;
    uint32_t free_blocks_count;
    uint32_t free_inodes_count;
    uint32_t first_data_block;
    uint32_t log_block_size; // log2 (block size) - 10. (In other words, the number to shift 1,024 to the left by to obtain the block size)
    uint32_t log_frag_size; // log2 (fragment size) - 10. (In other words, the number to shift 1,024 to the left by to obtain the fragment size)
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

typedef struct {
    uint32_t first_non_resv_inode; // default: 11
    uint16_t inode_size; // default: 128
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

typedef struct {
    uint32_t block_bitmap_ba;
    uint32_t inode_bitmap_ba;
    uint32_t inode_table_ba;
    uint16_t free_blocks_count;
    uint16_t free_inodes_count;
    uint16_t dirs_nr;
    uint8_t  unused[14];
} ext2_bgd;

typedef struct {
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
    uint32_t direct_bp0; // block pointers...
    uint32_t direct_bp1;
    uint32_t direct_bp2;
    uint32_t direct_bp3;
    uint32_t direct_bp4;
    uint32_t direct_bp5;
    uint32_t direct_bp6;
    uint32_t direct_bp7;
    uint32_t direct_bp8;
    uint32_t direct_bp9;
    uint32_t direct_bp10;
    uint32_t direct_bp11;
    uint32_t singly_indirect_bp;
    uint32_t doubly_indirect_bp;
    uint32_t triply_indirect_bp;
    uint32_t generation; // NFS
    uint32_t file_acl;
    uint32_t dir_acl_or_size_upper32; // dir/file
    uint32_t fragment_ba;
    uint8_t  os_specific2[12];
} ext2_inode;

typedef struct {
    uint32_t inode;
    uint16_t size;
    uint8_t name_len; // lsb
    uint8_t file_type; // msb (or type indicator if feature supported)
    char name[1];
} ext2_dir_entry;

#pragma pack (pop)

typedef struct {
    io_device* _device;
    io_buffer _buffer;
    superblock _sb;
    extended_superblock _esb;
    BOOL _is_valid;
} ext2_context;


BOOL ext2_parse_superblock(ext2_context* ctx) {
    // read superblock (sectors 2-3)
    void* data = io_read(ctx->_device, &ctx->_buffer, 2, 2);
    
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

uint32_t ext2_locate_inode_block(ext2_context* ctx, uint32_t index) {
    // if _esb is not valid - INODE_SIZE is 128
    return (index * ctx->_esb.inode_size) / (1024 << ctx->_sb.log_block_size);
}

ext2_bgd* ext2_read_bgdt(ext2_context* ctx, uint32_t index) {
    // entry size is 32 bytes so -> 16 entries per sector

    uint32_t block_size = 1024 << ctx->_sb.log_block_size;
    uint32_t bgd_addr = 0;

    if (block_size == 1024) {
        // the first block is the bootsector
        bgd_addr += (1024 + 1024);
    }
    else {
        // the first block is the bootsector + superblock
        bgd_addr += block_size;
    }

    uint32_t bgd_count = (
        (ctx->_sb.blocks_count + ctx->_sb.blocks_per_group - 1) / ctx->_sb.blocks_per_group
    );

    if (index >= bgd_count) return NULL;

    bgd_addr += index * sizeof(ext2_bgd);

    uint64_t lba = bgd_addr / SECTOR_SIZE;
    uint64_t bgd_offset = (bgd_addr % SECTOR_SIZE) / sizeof(ext2_bgd);

    ext2_bgd* bgdt = io_read(
        ctx->_device, 
        &ctx->_buffer, 
        lba,
        1);

    if (!bgdt) return NULL;

    return bgdt + bgd_offset;
}


ext2_context ext2_init(io_device* device) {
    ext2_context ctx = {0};
    BOOL res;

    ctx._device = device;
    ctx._buffer = io_alloc_buffer(1);

    res = ext2_parse_superblock(&ctx);

    if (!res) return ctx;

    ctx._is_valid = TRUE;

    return ctx;
}

BOOL ext2_is_valid(ext2_context* ctx) {
    return ctx->_is_valid == TRUE;
}

ext2_inode* ext2_read_inode(ext2_context* ctx, ext2_bgd* bgd, uint32_t index) {
    uint32_t inode_addr = bgd->inode_table_ba * (1024 << ctx->_sb.log_block_size);
    inode_addr += index * ctx->_esb.inode_size;

    uint64_t lba = inode_addr / SECTOR_SIZE;
    uint64_t inode_offset = inode_addr % SECTOR_SIZE;

    uint8_t* inodes = io_read(
        ctx->_device, 
        &ctx->_buffer, 
        lba,
        1);
    
    if (!inodes) return NULL;

    return (ext2_inode*)(inodes + inode_offset);
}

void ext2_root(ext2_context* ctx) {
    uint32_t inode_nr = 2;
    uint32_t inode_block = ext2_locate_inode_block(ctx, inode_nr);
    uint32_t inode_index = ext2_locate_inode_index(ctx, inode_nr);
    uint8_t  sectors_per_block = (1024 << ctx->_sb.log_block_size) / SECTOR_SIZE;

    ext2_bgd* bgd = ext2_read_bgdt(ctx, inode_block);
    ext2_inode* inode = ext2_read_inode(ctx, bgd, inode_index);
    
    uint64_t lba = inode->direct_bp0 * sectors_per_block;
    
    uint8_t* entries = (uint8_t*)io_read(
        ctx->_device, 
        &ctx->_buffer, 
        lba, 
        sectors_per_block);

    qemu_log("Root directory: ");

    for (size_t i = 0; i < sectors_per_block * SECTOR_SIZE;)
    {
        ext2_dir_entry* entry = (ext2_dir_entry*)(entries + i);
        
        qemu_log(entry->name);

        i += entry->size;
    }   
}


void ext2_release(ext2_context* ctx) {
    io_release_buffer(&ctx->_buffer);
}