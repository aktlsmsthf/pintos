#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "devices/disk.h"
#include "filesys/cache.h"
#include "threads/thread.h"

/* The disk that contains the file system. */
struct disk *filesys_disk;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  filesys_disk = disk_get (0, 1);
  if (filesys_disk == NULL)
    PANIC ("hd0:1 (hdb) not present, file system initialization failed");

  inode_init ();
  free_map_init ();

  if (format) 
    do_format ();

  free_map_open ();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  write_behind_all();
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size) 
{
  disk_sector_t inode_sector = 0;
  //struct dir *dir = dir_open_root ();
  char *real_name;
  struct dir *dir = lowest_dir(name, &real_name);

       bool success = (dir != NULL
                  && free_map_allocate (1, &inode_sector)
                  && inode_create (inode_sector, initial_size, false)
                  && dir_add (dir, real_name, inode_sector));
  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  dir_close (dir);

  return success;
}

bool
filesys_create_dir (const char *name) 
{
  disk_sector_t inode_sector = 0;
  //struct dir *dir = dir_open_root ();
  char *real_name;
  struct dir *dir = lowest_dir(name, &real_name);
  bool success;

     success= (dir!=NULL
                    && free_map_allocate(1, &inode_sector)
                    && dir_create(inode_sector, 16)
                    && dir_add(dir, real_name, inode_sector));
  

  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  dir_close (dir);
  return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
  //struct dir *dir = dir_open_root ();
  struct inode *inode = NULL;
  char *real_name;
  struct dir *dir;
  
  dir = lowest_dir(name, &real_name);
  if (dir != NULL){
     if(real_name ==NULL){
        dir_close(dir);
        return NULL;
     }
     if(strcmp(real_name, ".") ==0 || strcmp(real_name, "..")==0){
       dir_close(dir);
        return NULL;
    }
    dir_lookup (dir, real_name, &inode);
  }
  dir_close (dir);
  if(inode == NULL || inode_is_dir(inode)){
     return NULL;
  }
      
  return file_open (inode);
}

struct dir*
filesys_open_dir(const char *name){
   struct inode *inode = NULL;
  char *real_name;
  struct dir *dir;
  dir = lowest_dir(name, &real_name);
  if (dir != NULL){
    if(real_name ==NULL){
      return dir;
    }
    if(strcmp(real_name, ".") ==0 || strcmp(real_name, "..")==0 ){
       return dir;
    }
    dir_lookup (dir, real_name, &inode);
  }
  dir_close (dir);
  
  if(inode == NULL || !inode_is_dir(inode)){
     return NULL;
  }
   
  return dir_open (inode);
}
/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
  //struct dir *dir = dir_open_root ();
  char * real_name;
   struct inode *inode = NULL;
  struct dir *dir = lowest_dir(name, &real_name);
   bool success = true; 
  if (dir != NULL)
    dir_lookup (dir, real_name, &inode);
   printf("%s\n", real_name);
   if(!inode_is_dir(inode)){
      success = dir != NULL && dir_remove (dir, real_name);
   }
   else{
      struct dir *rdir = dir_open(inode);

      if(thread_current()->current_dir==NULL){
         struct dir *root = dir_open_root();
         if(get_sector_dir(root) == get_sector_dir(rdir)) {
            success = false;
         }
         dir_close(root);
      }
      else{
         if(get_sector_dir(rdir) == get_sector_dir(thread_current()->current_dir)) {
            }
      }
      if(success){
         success = dir != NULL && dir_remove (dir, real_name);
      }
      dir_close(rdir);
   }
   
  dir_close (dir); 

  return success;
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}

