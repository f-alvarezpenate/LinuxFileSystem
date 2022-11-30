/*************** type.h file for LEVEL-1 ****************/
#ifndef TPYE_H
#define TYPE_H
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;

SUPER *sp;
GD    *gp;
INODE *ip;
DIR   *dp;   

// Default dir and regular file modes
#define DIR_MODE 0x41ED
#define FILE_MODE 0x81AE
#define SUPER_MAGIC 0xEF53
#define SUPER_USER 0
// Proc status

#define FREE        0
#define READY       1

#define BLKSIZE  1024
#define NMINODE   128
#define NPROC       2

// file system table sizes // needed for level 2
#define NMINODE 100
#define NMTABLE 10
#define NPROC 2
#define NFD 10
#define NOFT 40

typedef struct minode{
  INODE INODE;           // INODE structure on disk
  int dev, ino;          // (dev, ino) of INODE
  int refCount;          // in use count
  int dirty;             // 0 for clean, 1 for modified

  int mounted;           // for level-3
  struct mntable *mptr;  // for level-3
}MINODE;

// Open file Table // needed for level 2
typedef struct oft{
  int         mode;             // mode of opened file
  int     refCount;         // number of PROCs sharing this instance
  MINODE *minodePtr;    // pointer to minode of file
  int       offset;            // byte offset for R|W
}OFT;

typedef struct proc{
  struct proc *next;
  int          pid;      // process ID  
  int          uid;      // user ID
  int          gid;
  MINODE      *cwd;      // CWD directory pointer  
  OFT     *fd[NFD];
}PROC;

// Mount Table structure // needed for level 3 I believe
typedef struct mtable{
int dev; // device number; 0 for FREE
int ninodes; // from superblock
int nblocks;
int free_blocks; // from superblock and GD
int free_inodes;
int bmap; // from group descriptor
int imap;
int iblock; // inodes start block
MINODE *mntDirPtr; // mount point DIR pointer
char devName[64]; //device name
char mntName[64]; // mount point DIR name
}MTABLE;
#endif