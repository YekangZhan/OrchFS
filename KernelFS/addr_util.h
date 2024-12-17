#ifndef ADDR_UTIL_H
#define ADDR_UTIL_H

#define INODE_DEVADDR_TO_BLKID(addr)         ((addr - OFFSET_INODE) / ORCH_INODE_SIZE)
#define INODE_BLKID_TO_DEVADDR(blkid)        (blkid * ORCH_INODE_SIZE + OFFSET_INODE)

#define IDX_DEVADDR_TO_BLKID(addr)           ((addr - OFFSET_INDEX) / ORCH_IDX_SIZE)
#define IDX_BLKID_TO_DEVADDR(blkid)          (blkid * ORCH_IDX_SIZE + OFFSET_INDEX)

#define VIRND_DEVADDR_TO_BLKID(addr)         ((addr - OFFSET_VIRND) / ORCH_VIRND_SIZE)
#define VIRND_BLKID_TO_DEVADDR(blkid)        (blkid * ORCH_VIRND_SIZE + OFFSET_VIRND)

#define BUFMETA_DEVADDR_TO_BLKID(addr)       ((addr - OFFSET_BUFMETA) / ORCH_BUFMETA_SIZE)
#define BUFMETA_BLKID_TO_DEVADDR(blkid)      (blkid * ORCH_BUFMETA_SIZE + OFFSET_BUFMETA)

#define PAGE_DEVADDR_TO_BLKID(addr)          ((addr - OFFSET_PAGE) / ORCH_PAGE_SIZE)
#define PAGE_BLKID_TO_DEVADDR(blkid)         (blkid * ORCH_PAGE_SIZE + OFFSET_PAGE)

#define BLOCK_DEVADDR_TO_BLKID(addr)         ((addr - OFFSET_BLOCK) / ORCH_BLOCK_SIZE)
#define BLOCK_BLKID_TO_DEVADDR(blkid)        (blkid * ORCH_BLOCK_SIZE + OFFSET_BLOCK)



#endif