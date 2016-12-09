#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <stdbool.h>
#include "filesys/off_t.h"
#include "devices/disk.h"
#include "threads/synch.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44
#define DN 10
#define IDN 10

struct inode_disk
  {
    //disk_sector_t start;                /* First data sector. */
    off_t length;                       /* File size in bytes. */
    unsigned magic;                     /* Magic number. */
    disk_sector_t direct_sector[DN];
    disk_sector_t indirect_sector[IDN];
    disk_sector_t d_indirect_sector;
    disk_sector_t  sector;
    bool is_dir;
    disk_sector_t parent;
    uint32_t unused[122-DN-IDN];               /* Not used. */
  };

/* In-memory inode. */
struct inode 
  {
    struct list_elem elem;              /* Element in inode list. */
    disk_sector_t sector;               /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct inode_disk data;             /* Inode content. */
    //struct lock ilock; 
  };

struct lock inode_lock;
struct bitmap;
void inode_init (void);
bool inode_create (disk_sector_t, off_t, bool);
struct inode *inode_open (disk_sector_t);
struct inode *inode_reopen (struct inode *);
disk_sector_t inode_get_inumber (const struct inode *);
void inode_close (struct inode *);
void inode_remove (struct inode *);
off_t inode_read_at (struct inode *, void *, off_t size, off_t offset);
off_t inode_write_at (struct inode *, const void *, off_t size, off_t offset);
void inode_deny_write (struct inode *);
void inode_allow_write (struct inode *);
off_t inode_length (const struct inode *);
bool inode_is_dir(struct inode *);

#endif /* filesys/inode.h */
