#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
#include "filesys/cache.h"
#include "threads/synch.h"
#include "userprog/syscall.h"
#include "devices/disk.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44
#define DN 10
#define IDN 10

/* On-disk inode.
   Must be exactly DISK_SECTOR_SIZE bytes long. */
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

/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, DISK_SECTOR_SIZE);
}

bool inode_extension(struct inode_disk *disk_inode, disk_sector_t sectors, disk_sector_t sectors2);

/* Returns the disk sector that contains byte offset POS within
   INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static disk_sector_t
byte_to_sector (const struct inode *inode, off_t pos) 
{
  ASSERT (inode != NULL);

  /**if (pos < inode->data.length)
    return inode->data.start + pos / DISK_SECTOR_SIZE;
  else
    return -1;**/
   
   off_t sectors = pos/DISK_SECTOR_SIZE;
   if(pos>=inode->data.length){
      
      return -1;
   }
   disk_sector_t ret;
   
   if(sectors<DN){
      ret = inode->data.direct_sector[sectors];
   }
   else if(sectors>=(DN+(IDN*128))){
      disk_sector_t indirect_sectors[128];
      disk_read(filesys_disk, inode->data.d_indirect_sector, indirect_sectors);
      disk_sector_t i = (sectors-(DN+(IDN*128)))/128;
      disk_sector_t sector[128];
      disk_read(filesys_disk, indirect_sectors[i], sector);
      ret = sector[(sectors-(DN+(IDN*128)))%128];
   }
   else{
      disk_sector_t sector[128];
      disk_sector_t i = (sectors-DN)/128;
      disk_read(filesys_disk, inode->data.indirect_sector[i], sector);
      ret = sector[(sectors-DN)%128];
   }
   return ret;
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void) 
{
  list_init (&open_inodes);
  lock_init(&inode_lock);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   disk.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (disk_sector_t sector, off_t length, bool is_dir)
{
  struct inode_disk *disk_inode = NULL;
  bool success = true;

  ASSERT (length >= 0);
   int i=0;
  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == DISK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
    {
      success = true;
      //size_t sectors = bytes_to_sectors (length);
      disk_inode->length = length;
      disk_inode->magic = INODE_MAGIC;
      disk_inode->sector = sector;
      disk_inode->is_dir = is_dir;
      disk_sector_t sectors = -1;
      disk_sector_t sectors2 = bytes_to_sectors(length);
      
      lock_acquire(&inode_lock);
     
      success =  inode_extension(disk_inode, sectors, sectors2);
      /**static char zeros[DISK_SECTOR_SIZE];  
      while(sectors!=sectors2){
         sectors++;
         
         if(sectors<DN){
            success = free_map_allocate(1, &disk_inode->direct_sector[sectors]);
            disk_write(filesys_disk, disk_inode->direct_sector[sectors], zeros);
            
         }
         else if(sectors>=(DN+(IDN*128))){
            disk_sector_t indirects[128];
            if(sectors==(DN+(IDN*128))){
               success = free_map_allocate(1, &disk_inode->d_indirect_sector);
            }
            else{
               disk_read(filesys_disk, disk_inode->d_indirect_sector, indirects);
            }
            disk_sector_t sectordi[128];
            disk_read(filesys_disk, indirects[(sectors-(DN+(IDN*128)))/128], sectordi);
            success = free_map_allocate(1, &sectordi[(sectors-(DN+(IDN*128)))%128]);
            disk_write(filesys_disk, sectordi[(sectors-(DN+(IDN*128)))%128], zeros);
            disk_write(filesys_disk, indirects[(sectors-(DN+(IDN*128)))/128], sectordi);
            disk_write(filesys_disk, disk_inode->d_indirect_sector, indirects);
         }
         else{
            disk_sector_t sectori[128];
            if((sectors-DN)%128==0){
               success = free_map_allocate(1, &disk_inode->indirect_sector[(sectors-DN)/128]);
            }
            else{
               disk_read(filesys_disk, disk_inode->indirect_sector[(sectors-DN)/128], sectori);
            }
            success = free_map_allocate(1, &sectori[(sectors-DN)%128]);
            disk_write(filesys_disk, sectori[(sectors-DN)%128], zeros);
            disk_write(filesys_disk, disk_inode->indirect_sector[(sectors-DN)/128], sectori);
            
         }
      }   **/
     disk_write(filesys_disk, sector, disk_inode);
     
     lock_release(&inode_lock);       
      free (disk_inode);
     
    }
  return success;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (disk_sector_t sector) 
{
  struct list_elem *e;
  struct inode *inode;
  
  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e)) 
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector) 
        {
          inode_reopen (inode);
          return inode; 
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL){
    return NULL;
  }

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  //lock_init(&inode->ilock);
  disk_read (filesys_disk, inode->sector, &inode->data);
  
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
disk_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode) 
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;
   
  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);
 
      /* Deallocate blocks if removed. */
      if (inode->removed) 
        {
         lock_acquire(&inode_lock);
         free_map_release (inode->sector, 1);   
         
         int i;
         int sectors = bytes_to_sectors(inode->data.length);
         for(i=0; i<DN; i++){
            free_map_release(inode->data.direct_sector[i], 1);
            if(--sectors==0){
               break;
            }
         }
         if(sectors>0){
            int n = sectors/128;
            for(i=n; i<IDN; i++){
               disk_sector_t sector[128];
               disk_read(filesys_disk, inode->data.indirect_sector[i], sector);
               int j;
               for(j=0; j<128; j++){
                  free_map_release(sector[j], 1);
                  if(--sectors==0){
                     break;
                  }
               }
               if(sectors==0){
                  break;
               }
            }
         }
         if(sectors>0){
            disk_sector_t indirects[128];
            disk_read(filesys_disk, inode->data.d_indirect_sector, indirects);
            for(i=0;i<128;i++){
               disk_sector_t sector[128];
               int j;
               for(j=0;j<128;j++){
                  disk_read(filesys_disk, indirects[i], sector);
                  free_map_release(sector[j], 1);
                  if(--sectors==0){
                     break;
                  }
               }
               free_map_release(indirects[i], 1);
               if(sectors==0){
                  break;
               }
            }
            free_map_release(inode->data.d_indirect_sector, 1);
         }
         lock_release(&inode_lock); 
        }
      free (inode); 
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode) 
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) 
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  uint8_t *bounce = NULL;
  //lock_acquire(&inode_lock);
  while (size > 0) 
    {
      /* Disk sector to read, starting byte offset within sector. */
      disk_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % DISK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = DISK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;
      
       //
       struct cache_entry *c = read_to_cache(sector_idx, true);
       //lock_acquire(&cache_lock);
       memcpy(buffer+bytes_read, c->cache+sector_ofs, chunk_size);
       //lock_release(&cache_lock);
      
      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  //lock_release(&inode_lock);
  free (bounce);

  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset) 
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  uint8_t *bounce = NULL;
  
      
  if (inode->deny_write_cnt)
    return 0;
   
   if(size+offset>inode->data.length){
      //inode_deny_write (inode); 
      //lock_acquire(&inode->ilock);
      lock_acquire(&inode_lock);
      
      disk_sector_t sectors = bytes_to_sectors(inode->data.length);
      disk_sector_t sectors2 = bytes_to_sectors(size+offset);
      bool success = true;
      inode_extension(&inode->data, sectors, sectors2);
      //printf("%d\n", inode->data.test);
      /**static char zeros[DISK_SECTOR_SIZE];
      while(sectors!=sectors2){
         sectors++;
         if(sectors<DN){
            free_map_allocate(1, &inode->data.direct_sector[sectors]);
            disk_write(filesys_disk, inode->data.direct_sector[sectors], zeros);
            
         }
         else if(sectors>=(DN+(IDN*128))){
            disk_sector_t indirects[128];
            if(sectors==(DN+(IDN*128))){
               free_map_allocate(1, &inode->data.d_indirect_sector);
            }
            else{
               disk_read(filesys_disk, inode->data.d_indirect_sector, indirects);
            }
            disk_sector_t sectordi[128];
            disk_read(filesys_disk, indirects[(sectors-(DN+(IDN*128)))/128], sectordi);
            free_map_allocate(1, &sectordi[(sectors-(DN+(IDN*128)))%128]);
            disk_write(filesys_disk, sectordi[(sectors-(DN+(IDN*128)))%128], zeros);
            disk_write(filesys_disk, indirects[(sectors-(DN+(IDN*128)))/128], sectordi);
            disk_write(filesys_disk, inode->data.d_indirect_sector, indirects);
         }
         else{
            disk_sector_t sectori[128];
            if((sectors-DN)%128==0){
               free_map_allocate(1, &inode->data.indirect_sector[(sectors-DN)/128]);
            }
            else{
               disk_read(filesys_disk, inode->data.indirect_sector[(sectors-DN)/128], sectori);
            }
            free_map_allocate(1, &sectori[(sectors-DN)%128]);
            disk_write(filesys_disk, sectori[(sectors-DN)%128], zeros);
            disk_write(filesys_disk, inode->data.indirect_sector[(sectors-DN)/128], sectori);
         }
      }**/
      inode->data.length = size+offset;
      disk_write(filesys_disk, inode->data.sector, &inode->data);
      //lock_release(&inode->ilock);
      
      lock_release(&inode_lock);
      //inode_allow_write (inode);
   }
   //lock_acquire(&inode_lock);
  while (size > 0) 
    {
      
      /* Sector to write, starting byte offset within sector. */
      disk_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % DISK_SECTOR_SIZE;
      
      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = DISK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;
       
      
      struct cache_entry *c = find_cache_by_sector(sector_idx);
       if(!c){
          c = read_to_cache(sector_idx, true);
       }
       //lock_acquire(&cache_lock);
       //struct cache_entry *c =  read_to_cache(sector_idx, true);
       memcpy(c->cache+sector_ofs, buffer+bytes_written, chunk_size);
       c->dirty = true;
       //lock_release(&cache_lock);
       
      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }
   //lock_release(&inode_lock);
  free (bounce);
  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode) 
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode) 
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->data.length;
}

bool inode_is_dir(struct inode *inode){
   return inode->data.is_dir;
}

disk_sector_t inode_disk_sector(struct inode *inode){
   return inode->data.sector;
}

disk_sector_t inode_parent(struct inode *inode){
   return inode->data.parent;
}

int inode_open_cnt(struct inode *inode){
   return inode->open_cnt;
}
/*
void ilock_acquire(struct inode *inode){
   lock_acquire(&inode->ilock);
}
void ilock_release(struct inode *inode){
   lock_release(&inode->ilock);
}*/
void set_parent(disk_sector_t parent_sector, disk_sector_t child_sector){
  lock_acquire(&inode_lock);
  struct inode_disk *disk_inode = malloc(sizeof (struct inode_disk));
  disk_read(filesys_disk, child_sector, disk_inode);
  disk_inode->parent = parent_sector;
  disk_write(filesys_disk, child_sector, disk_inode);
  free(disk_inode);
  lock_release(&inode_lock);
}

bool inode_extension(struct inode_disk *disk_inode, disk_sector_t sectors, disk_sector_t sectors2){
      bool success = true;
      static char zeros[DISK_SECTOR_SIZE];  
      while(sectors!=sectors2){
         sectors++;
         
         if(sectors<DN){
            success = free_map_allocate(1, &disk_inode->direct_sector[sectors]);
            disk_write(filesys_disk, disk_inode->direct_sector[sectors], zeros);
            
         }
         else if(sectors>=(DN+(IDN*128))){
            disk_sector_t indirects[128];
            if(sectors==(DN+(IDN*128))){
               success = free_map_allocate(1, &disk_inode->d_indirect_sector);
            }
            else{
               disk_read(filesys_disk, disk_inode->d_indirect_sector, indirects);
            }
            disk_sector_t sectordi[128];
            disk_read(filesys_disk, indirects[(sectors-(DN+(IDN*128)))/128], sectordi);
            success = free_map_allocate(1, &sectordi[(sectors-(DN+(IDN*128)))%128]);
            disk_write(filesys_disk, sectordi[(sectors-(DN+(IDN*128)))%128], zeros);
            disk_write(filesys_disk, indirects[(sectors-(DN+(IDN*128)))/128], sectordi);
            disk_write(filesys_disk, disk_inode->d_indirect_sector, indirects);
         }
         else{
            disk_sector_t sectori[128];
            if((sectors-DN)%128==0){
               success = free_map_allocate(1, &disk_inode->indirect_sector[(sectors-DN)/128]);
            }
            else{
               disk_read(filesys_disk, disk_inode->indirect_sector[(sectors-DN)/128], sectori);
            }
            success = free_map_allocate(1, &sectori[(sectors-DN)%128]);
            disk_write(filesys_disk, sectori[(sectors-DN)%128], zeros);
            disk_write(filesys_disk, disk_inode->indirect_sector[(sectors-DN)/128], sectori);
            
         }
      }   
     //disk_inode->length = sectors2;
     //disk_write(filesys_disk, disk_inode->sector, disk_inode);
     
   return success;
}
