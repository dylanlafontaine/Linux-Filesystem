/****************************************************************************
*                   KCW  Implement ext2 file system                         *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>

#include "type.h"
//#include "cd_ls_pwd.c"
//#include "util.c"

// global variables
MINODE minode[NMINODE];
MINODE *root;

int mountdev, mountino;

PROC   proc[NPROC], *running;
MTABLE mtable[NMP];

char gpath[256]; // global for tokenized components
char *name[256];  // assume at most 32 components in pathname
int   n;         // number of component strings

int fd;
int nblocks, ninodes, bmap, imap, inode_start; // disk parameters

#include "util.c"
#include "cd_ls_pwd.c"
#include "mkdir_creat.c"
#include "rmdir.c"
#include "link_unlink.c"
#include "symlink.c"
#include "open_close.c"
#include "read_cat.c"
#include "write_cp.c"
#include "mount_umount.c"

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
    p->pid = i;
    p->uid = p->gid = 0;
    p->cwd = 0;
    p->status = FREE;
    for (j=0; j<NFD; j++)
      p->fd[j] = 0;
  }
  proc[0].uid = 0;
  proc[1].uid = 1;
  for (i = 0; i < NFD; i++) {
    mtable[i].dev = 0; //Free
  }
}

// load root INODE and set root pointer to it
int mount_root(int dev)
{
  printf("mount_root()\n");
  root = iget(dev, 2);
  root->INODE.i_mode |= 0x1FF;
}

char *disk = "disk3.1";
int main(int argc, char *argv[ ])
{
  int ino, dev;
  char buf[BLKSIZE];
  char line[128], cmd[32], pathname[128], pathname2[128];

  printf("checking EXT2 FS ....");
  if ((fd = open(disk, O_RDWR)) < 0){
    printf("open %s failed\n", disk);
    exit(1);
  }
  dev = fd;    // fd is the global dev

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
  inode_start = gp->bg_inode_table;
  printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, inode_start);

  init();
  mount_root(dev);
  printf("root refCount = %d\n", root->refCount);

  printf("creating P0 as running process\n");
  running = &proc[0];
  running->status = READY;
  running->cwd = iget(dev, 2);
  printf("root refCount = %d\n", root->refCount);

  // WRTIE code here to create P1 as a USER process
  proc[1].status = READY;
  proc[1].cwd = iget(dev, 2);

  while(1){
    printf("input command : [ls|cd|pwd|mkdir|creat|rmdir|link|unlink|symlink|open|lseek|pfd|read|cat|write|cp|mount|umount|sw|quit] ");
    fgets(line, 128, stdin);
    line[strlen(line)-1] = 0;

    if (line[0]==0)
       continue;
    pathname[0] = 0;
    pathname2[0] = 0;
    sscanf(line, "%s %s %s", cmd, pathname, pathname2);
    printf("cmd=%s pathname=%s pathname2=%s\n", cmd, pathname, pathname2);

    if (strcmp(cmd, "ls")==0)
       ls(pathname);
    else if (strcmp(cmd, "cd")==0)
       chdir(pathname);
    else if (strcmp(cmd, "pwd")==0)
       pwd(running->cwd);
    else if (strcmp(cmd, "quit")==0)
       quit();
    else if (strcmp(cmd, "mkdir")==0) {
       make_dir(pathname);
     }
    else if (strcmp(cmd, "creat")==0) {
       creat_file(pathname);
     }
     else if (strcmp(cmd, "rmdir")==0) {
       rmdir(pathname);
     }
     else if (strcmp(cmd, "link")==0) {
       link(pathname, pathname2);
     }
     else if (strcmp(cmd, "unlink")==0) {
       unlink(pathname);
     }
     else if (strcmp(cmd, "symlink")==0) {
       symlink(pathname, pathname2);
     }
     else if (strcmp(cmd, "open")==0) {
       printf("%d\n",open_file(pathname, atoi(pathname2)));
     }
     else if (strcmp(cmd, "close")==0) {
       close_file(atoi(pathname));
     }
     else if (strcmp(cmd, "lseek")==0) {
       printf("%d\n", mylseek(atoi(pathname), atoi(pathname2)));
     }
     else if (strcmp(cmd, "pfd")==0) {
       pfd();
     }
     else if (strcmp(cmd, "read")==0) {
       read_file(atoi(pathname), atoi(pathname2));
     }
     else if (strcmp(cmd, "cat")==0) {
       cat_file(pathname);
     }
     else if (strcmp(cmd, "write")==0) {
       write_file(atoi(pathname), pathname2);
     }
     else if (strcmp(cmd, "cp")==0) {
       cp(pathname, pathname2);
     }
     else if (strcmp(cmd, "mount")==0) {
       mount(pathname, pathname2);
     }
     else if (strcmp(cmd, "umount")==0) {
       umount(pathname);
     }
     else if (strcmp(cmd, "sw")==0) {
       sw();
     }
  }
}

int quit()
{
  int i;
  MINODE *mip;

  if (running->uid != 0) {
    printf("Must be Superuser to quit\n");
    return -1;
  }

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount > 0)
      iput(mip);
  }
  exit(0);
}
