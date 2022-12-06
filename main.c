/****************************************************************************
*                   KCW: mount root file system                             *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include <ext2fs/ext2_fs.h>

#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>

#include "type.h"

extern MINODE *iget();

MINODE minode[NMINODE];
MINODE *root;
PROC   proc[NPROC], *running;
OFT oft[NOFT];

char gpath[128]; // global for tokenized components
char *name[64];  // assume at most 64 components in pathname
int   n;         // number of component strings

int  fd, dev;
int  nblocks, ninodes, bmap, imap, iblk;
char line[128], cmd[32], pathname[128],filename[128];

#include "cd_ls_pwd.c"
//#include "util.c"
#include "mkdir_creat.c"
#include "rmdir.c"
#include "alloc_dalloc.c"
#include "symlink.c"
#include "link_unlink.c"
#include "open_close.c"
#include "read_cat.c"
#include "write_cp.c"
int init()
{
  int i, j;
  MINODE *mip;
  PROC   *p;

  printf("init()\n");

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->refCount = 0;
    mip->mounted = 0;
    mip->mptr = 0;
  }
  for (i=0; i<NPROC; i++){
    p = &proc[i];
    p->pid = i+1;           // pid = 1, 2
    p->uid = p->gid = 0;    // uid = 0: SUPER user
    p->cwd = 0;             // CWD of process
  }
}

// load root INODE and set root pointer to it
int mount_root()
{  
  printf("mount_root()\n");
  root = iget(dev, 2);
}

//char *disk = "disk2";     // change this to YOUR virtual // WE HAVE TO CHANGE THIS ARGV[1] FOR THE DEMO SO TA's AND KC CAN TRY DIFFERENT DISKS!
char *disk; // now whoever is running the code can change disks easily. uncomment when ready to use e.g.    ./a.out disk2
int main(int argc, char *argv[ ])
{
  disk = argv[1]; // has to be assigned arg[1] value inside of main now. Cant be outside like it was before
  int ino;
  char buf[BLKSIZE];

  printf("checking EXT2 FS ....");
  if ((fd = open(disk, O_RDWR)) < 0){
    printf("open %s failed\n", disk);
    exit(1);
  }

  dev = fd;    // global dev same as this fd   

  /********** read super block  ****************/
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;

  /* verify it's an ext2 file system ***********/
  if (sp->s_magic != 0xEF53){
      printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
      exit(1);
  }     
  printf("EXT2 FS OK\n");
  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;

  get_block(dev, 2, buf); 
  gp = (GD *)buf;

  bmap = gp->bg_block_bitmap;
  imap = gp->bg_inode_bitmap;
  iblk = gp->bg_inode_table;
  printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, iblk);

  init();  
  mount_root();
  printf("root refCount = %d\n", root->refCount);

  printf("creating P0 as running process\n");
  running = &proc[0];
  running->cwd = iget(dev, 2);
  printf("root refCount = %d\n", root->refCount);

  // WRTIE code here to create P1 as a USER process
  printf("creating P1 as a USER process \n");
  proc[1].pid = 2;
  proc[1].uid = 1;
  proc[1].cwd = 0;

  printf("root refCount = %d\n",root->refCount);

  while(1){
    printf("input command : [ls|cd|pwd|mkdir|creat|rmdir|link|unlink|symlink|readlink|open|close|pfd|read|cat|write|cp|quit] ");
    fgets(line, 128, stdin);
    line[strlen(line)-1] = 0;

    if (line[0]==0)
       continue;
    pathname[0] = 0;

    sscanf(line, "%s %s %s", cmd, pathname,filename);
    printf("cmd=%s pathname=%s\n", cmd, pathname);

    if (strcmp(cmd, "ls")==0)
       ls(pathname); //add pathname
    else if (strcmp(cmd, "cd")==0)
       cd();
    else if (strcmp(cmd, "pwd")==0)
       pwd(running->cwd);
    else if (strcmp(cmd, "quit")==0)
       quit();
    else if (strcmp(cmd, "mkdir")==0)
        mymkdir(pathname);
    else if (strcmp(cmd, "creat")==0)
        mycreat(pathname);
    else if(strcmp(cmd, "rmdir")==0)
    	  myrmdir();
    else if(strcmp(cmd, "link")==0)
        link(pathname, filename);
    else if(strcmp(cmd, "unlink")==0)
        unlink(pathname);
    else if(strcmp(cmd, "symlink")==0)
        symlink(pathname, filename);
    else if(strcmp(cmd, "readlink")==0)
        readlink(pathname);
    else if(strcmp(cmd, "open")==0)
        open_file(pathname, filename);  
    else if(strcmp(cmd, "close")==0)
        terminal_close_file(pathname);
    else if(strcmp(cmd, "pfd")==0)
        pfd(); 
    else if(strcmp(cmd, "read")==0)
        read_file(pathname, filename);
    else if(strcmp(cmd, "cat")==0)
        cat_file(pathname);
    else if(strcmp(cmd, "write")==0)
        write_file(pathname, filename);
    else if(strcmp(cmd, "cp")==0)
        cp_file(pathname, filename);
    
  }
}

int quit()
{
  int i;
  MINODE *mip;
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount > 0)
      iput(mip);
  }
  printf("see you later, alligator\n");
  exit(0);
}
