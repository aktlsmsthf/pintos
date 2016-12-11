#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <stdbool.h>
#include "filesys/off_t.h"
#include "devices/disk.h"
#include "threads/synch.h"





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
disk_sector_t inode_disk_sector(struct inode *);
disk_sector_t inode_parent(struct inode*);
int inode_open_cnt(struct inode*);
void set_parent(disk_sector_t parent_sector, disk_sector_t child_sector);
void ilock_acquire(struct inode*);
void ilock_release(struct inode*);

#endif /* filesys/inode.h */
