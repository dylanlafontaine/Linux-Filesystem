/*********** util.c file ****************/

int get_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   read(dev, buf, BLKSIZE);
}
int put_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   write(dev, buf, BLKSIZE);
}

int tokenize(char *pathname)
{
  int i;
  char *s;
  //printf("\n\ntokenize %s\n", pathname);

  strcpy(gpath, pathname);   // tokens are in global gpath[ ]
  n = 0;

  s = strtok(gpath, "/");
  while(s){
    name[n] = s;
    n++;
    s = strtok(0, "/");
  }

  //for (i= 0; i<n; i++)
  //  printf("%s  ", name[i]);
  //printf("\n");
}

// return minode pointer to loaded INODE
MINODE *iget(int dev, int ino)
{
  int i;
  MINODE *mip;
  char buf[BLKSIZE];
  int blk, offset;
  INODE *ip;

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->dev == dev && mip->ino == ino){
       mip->refCount++;
       //printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
       return mip;
    }
  }

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount == 0){
       //printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
       mip->refCount = 1;
       mip->dev = dev;
       mip->ino = ino;

       // get INODE of ino into buf[ ]
       blk    = (ino-1)/8 + inode_start;
       offset = (ino-1) % 8;

       //printf("iget: ino=%d blk=%d offset=%d\n", ino, blk, offset);

       get_block(dev, blk, buf);
       ip = (INODE *)buf + offset;
       // copy INODE to mp->INODE
       mip->INODE = *ip;
       return mip;
    }
  }
  printf("PANIC: no more free minodes\n");
  return 0;
}

void iput(MINODE *mip)
{
 int i, block, offset, iblock;
 char buf[BLKSIZE];
 INODE *ip;

 mip->refCount--;

 if (mip->refCount > 0) {  // minode is still in use
    return;
  }
 if (!mip->dirty) {       // INODE has not changed; no need to write back
    return;
  }

 /* write INODE back to disk */
 /***** NOTE *******************************************
  For mountroot, we never MODIFY any loaded INODE
                 so no need to write it back
  FOR LATER WROK: MUST write INODE back to disk if refCount==0 && DIRTY

  Write YOUR code here to write INODE back to disk
 ********************************************************/
 //printf("Writing INODE to disk\n");
 iblock = inode_start;
 block = (mip->ino - 1) / 8 + iblock;
 offset = (mip->ino - 1) % 8;
 //get block containing this inode
 get_block(mip->dev, block, buf);
 ip = (INODE *)buf + offset;
 *ip = mip->INODE;
 put_block(mip->dev, block, buf);
 midalloc(mip);
}

int search(MINODE *mip, char *name)
{
   char *cp, c, sbuf[BLKSIZE], temp[256];
   DIR *dp;
   INODE *ip;

   //printf("search for %s in MINODE = [%d, %d]\n", name, mip->dev, mip->ino);
   ip = &(mip->INODE);

   /*** search for name in mip's data blocks: ASSUME i_block[0] ONLY ***/

   get_block(mip->dev, ip->i_block[0], sbuf);
   dp = (DIR *)sbuf;
   cp = sbuf;
   //printf("  ino   rlen  nlen  name\n");

   while (cp < sbuf + BLKSIZE){
     strncpy(temp, dp->name, dp->name_len);
     temp[dp->name_len] = 0;
     //printf("%4d  %4d  %4d    %s\n",
           //dp->inode, dp->rec_len, dp->name_len, temp);
     if (strcmp(temp, name)==0){
        //printf("found %s : ino = %d\n", temp, dp->inode);
        return dp->inode;
     }
     cp += dp->rec_len;
     dp = (DIR *)cp;
   }
   return 0;
}

int maccess(MINODE *mip, char mode) {
  if (running == &proc[0]) {
    return 1;
  }
  
  if (mip->INODE.i_uid == running->uid) {
    switch(mode) {
      case 'r':
        if ((mip->INODE.i_mode & 0x100) == 0x100) {
          return 1;
        }
        break;
      case 'w':
        if ((mip->INODE.i_mode & 0x80) == 0x80) {
          return 1;
        }
        break;
      case 'x':
        if ((mip->INODE.i_mode & 0x40) == 0x40) {
          return 1;
        }
        break;
    }
  }
  else if (mip->INODE.i_gid == running->gid) {
    switch(mode) {
      case 'r':
        if ((mip->INODE.i_mode & 0x20) == 0x20) {
          return 1;
        }
        break;
      case 'w':
        if ((mip->INODE.i_mode & 0x10) == 0x10) {
          return 1;
        }
        break;
      case 'x':
        if ((mip->INODE.i_mode & 0x8) == 0x8) {
          return 1;
        }
        break;
    }
  }
  else {
    switch(mode) {
      case 'r':
        if ((mip->INODE.i_mode & 0x4) == 0x4) {
          return 1;
        }
        break;
      case 'w':
        if ((mip->INODE.i_mode & 0x2) == 0x2) {
          return 1;
        }
        break;
      case 'x':
        if ((mip->INODE.i_mode & 0x1) == 0x1) {
          return 1;
        }
        break;
    }
  }
  return 0;
}


int getino(char *pathname, int *dev)
{
  int i, j, ino, blk, disp;
  char buf[BLKSIZE];
  INODE *ip;
  MINODE *mip, *newmip;

  //printf("getino: pathname=%s\n", pathname);
  if (strcmp(pathname, "/")==0)
      return 2;

  // starting mip = root OR CWD
  if (pathname[0]=='/')
     mip = root;
  else
     mip = running->cwd;

  mip->refCount++;         // because we iput(mip) later

  tokenize(pathname);

  for (i=0; i<n; i++){
      if (!maccess(mip, 'x')) {
        printf("NO ACCESS\n");
        iput(mip);
        return 0;
      }
      //printf("===========================================\n");
      //printf("getino: i=%d name[%d]=%s\n", i, i, name[i]);
      if (strcmp(name[i], "..") == 0 && mip->dev != root->dev && mip->ino == 2) {
        printf("UP cross mounting point\n");
        for (j = 0; j < NMP; j++) {
          if (mtable[j].dev == mip->dev) {
            newmip = mtable[j].mptr;
            iput(mip);
            mip = newmip;
            *dev = newmip->dev;
            continue;
          }
        }
      }

      ino = search(mip, name[i]);
      if (ino==0){
         iput(mip);
         //printf("name %s does not exist\n", name[i]);
         return 0;
      }
      newmip = iget(*dev, ino);
      iput(mip);
      mip = newmip;

      if (mip->mounted) {
        printf("DOWN cross mounting point\n");
        *dev = mip->mptr->dev;

        iput(mip);

        mip = iget(*dev, 2);
        mountdev = mip->dev;
        mountino = mip->ino;
      }
      //iput(mip);                // release current mip
      //mip = iget(dev, ino);     // get next mip
   }

  iput(mip);                   // release mip
  return mip->ino;
}

int findmyname(MINODE *parent, u32 myino, char *myname)
{
  // WRITE YOUR code here:
  // search parent's data block for myino;
  // copy its name STRING to myname[ ];
}

int findino(MINODE *mip, u32 *myino) // myino = ino of . return ino of ..
{
  char buf[BLKSIZE], *cp;
  DIR *dp;

  get_block(mip->dev, mip->INODE.i_block[0], buf);
  cp = buf;
  dp = (DIR *)buf;
  *myino = dp->inode;
  cp += dp->rec_len;
  dp = (DIR *)cp;
  return dp->inode;
}

int tst_bit(char *buf, int bit)
{
  int i = bit / 8;
  int j = bit % 8;
  return buf[i] & (1 << j);
}

int set_bit(char *buf, int bit)
{
  int i = bit / 8;
  int j = bit % 8;
  return buf[i] |= (1 << j);
}

int clr_bit(char *buf, int bit) {
  int i = bit / 8;
  int j = bit % 8;
  return buf[i] &= ~(1 << j);
}

int decFreeInodes(int dev) {
  char buf[BLKSIZE];
  //dec free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count--;
  put_block(dev, 1, buf);
  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count--;
  put_block(dev, 2, buf);
}

int decFreeBlocks(int dev) {
  char buf[BLKSIZE];
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_blocks_count--;
  put_block(dev, 1, buf);
  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_blocks_count--;
  put_block(dev, 2, buf);
}

int ialloc(int dev)  // allocate an inode number from inode_bitmap
{
  int  i;
  char buf[BLKSIZE];
// read inode_bitmap block
  get_block(dev, imap, buf);

  for (i=0; i < ninodes; i++){
    if (tst_bit(buf, i)==0){
        set_bit(buf, i);
        put_block(dev, imap, buf);
        printf("allocated ino = %d\n", i+1); // bits count from 0; ino from 1
        decFreeInodes(dev);
        return i+1;
    }
  }
  return 0; // out of FREE inodes
}

int balloc(int dev) {
  int i, j;
  char buf[BLKSIZE], buf2[BLKSIZE];
  //read block_bitmap
  get_block(dev, bmap, buf);
  //bzero(buf, BLKSIZE);
  for (i=0; i < nblocks; i++) {
    if (tst_bit(buf, i) == 0) {
      set_bit(buf, i);
      put_block(dev, bmap, buf);
      printf("allocated block = %d\n", i + 1);
      decFreeBlocks(dev);
      get_block(dev, i + 1, buf2);
      bzero(buf2, BLKSIZE);
      put_block(dev, i + 1, buf2);
      return i + 1;
    }
  }
  return 0; //out of FREE blocks
}

MINODE *mialloc() {
  int i;
  for (i = 0; i < NMINODE; i++) {
    MINODE *mp = &minode[i];
    if (mp->refCount == 0) {
      mp->refCount = 1;
      return mp;
    }
  }
  printf("FS panic: out of minodes\n");
  return 0;
}

int midalloc(MINODE *mip) {
  mip->refCount = 0;
}

int idalloc(int dev, int ino)  // deallocate an ino number
{
  int i;
  char buf[BLKSIZE];

  if (ino > ninodes){
    printf("inumber %d out of range\n", ino);
    return 0;
  }

  // get inode bitmap block
  get_block(dev, imap, buf);
  clr_bit(buf, ino-1);

  // write buf back
  put_block(dev, imap, buf);
}

int bdalloc(int dev, int blk) { //deallocate a block number
  int i;
  char buf[BLKSIZE];

  if (blk > nblocks) {
    printf("block number %d out of range\n", blk);
    return 0;
  }

  //get block bitmap block
  get_block(dev, bmap, buf);
  clr_bit(buf, blk - 1);

  //write buf back
  put_block(dev, bmap, buf);
}

int rm_child(MINODE *pip, char *name) {
  char buf[BLKSIZE], *cp, dirname[256];
  DIR *dp;
  int temp_rec_len, total_len;

  get_block(pip->dev, pip->INODE.i_block[0], buf);
  dp = (DIR *)buf;
  cp = buf;

  //total_len = 0;
  while (cp < buf + BLKSIZE) {
    strncpy(dirname, dp->name, dp->name_len);
    dirname[dp->name_len] = 0;
    //total_len += dp->rec_len;
    if (strcmp(dirname, name) == 0) {
      printf("Found directory %s\n", name);
      break;
    }
    cp += dp->rec_len;
    dp = (DIR *)cp;
  }

  if (dp->rec_len == BLKSIZE) { //CASE 1
    printf("CASE 1\n");
    bdalloc(pip->dev, pip->INODE.i_block[0]);
    pip->INODE.i_size -= BLKSIZE;
    //Figure out how to compact array
  }
  else if ((cp + dp->rec_len) >= buf + BLKSIZE) {
    char *ncp = buf;
    DIR *ndp = (DIR *)buf;
    printf("CASE 2\n");
    while (ncp + ndp->rec_len != cp) {
      ncp += ndp->rec_len;
      ndp = (DIR *)ncp;
    }
    ndp->rec_len += dp->rec_len;
    put_block(pip->dev, pip->INODE.i_block[0], buf);
  }
  else {
    printf("CASE 3\n");
    char *ncp = buf;
    DIR *ndp = (DIR *)buf;

    while (ncp + ndp->rec_len < buf + BLKSIZE) { //go to last entry
      ncp += ndp->rec_len;
      ndp = (DIR *)ncp;
    }
    ndp->rec_len += dp->rec_len;
    printf("last entry = [%d %d %d]\n", ndp->inode, ndp->rec_len, ndp->name);
    int copySize = (buf + BLKSIZE) - (cp + dp->rec_len);
    printf("copySize = %d\n", copySize);
    memcpy(cp, cp + dp->rec_len, copySize);
    put_block(pip->dev, pip->INODE.i_block[0], buf);
  }
  show(pip);
}

int show(MINODE *mip)
{
  char *cp;
  DIR *dp;
  char buf[BLKSIZE], name[256];
  printf("blk=%d ", mip->INODE.i_block[0]);

  get_block(mip->dev, mip->INODE.i_block[0], buf);

  cp = buf;
  dp = (DIR *)buf;

  while(cp < buf + BLKSIZE){
    strncpy(name, dp->name, dp->name_len);
    name[dp->name_len] = 0;

    printf("[%d %d %d %s] ", dp->inode, dp->rec_len, dp->name_len, name);
    //getchar();
    cp += dp->rec_len;
    dp = (DIR *)cp;
  }
}

int truncate(MINODE *mip) {
  int ibuf[256], blk, buf13[256], dbuf[256];
  for (int i = 0; i < 12; i++) {
    if (mip->INODE.i_block[i] == 0) {
      continue;
    }
    bdalloc(mip->dev, mip->INODE.i_block[i]);
  }
  if (mip->INODE.i_block[12] != 0) {
    get_block(mip->dev, mip->INODE.i_block[12], ibuf); //This causes the crash when unlinking indirect blocks
    for (int i = 0; i < 256; i++) {
      if (ibuf[i] == 0) {
        continue;
      }
      bdalloc(mip->dev, ibuf[i]);
    }
    bdalloc(mip->dev, mip->INODE.i_block[12]);
  }
  if (mip->INODE.i_block[13] != 0) {
    get_block(mip->dev, mip->INODE.i_block[13], buf13);
    for (int i = 0; i < 256; i++) {
      if (buf13[i] == 0) {
        continue;
      }
      get_block(mip->dev, buf13[i], dbuf);
      for (int j = 0; j < 256; j++) {
        if (dbuf[j] == 0) {
          continue;
        }
        bdalloc(mip->dev, dbuf[j]);
      }
      bdalloc(mip->dev, buf13[i]);
    }
    bdalloc(mip->dev, mip->INODE.i_block[13]);
  }
  mip->dirty = 1;
  idalloc(mip->dev, mip->ino);
}

int sw() {
  if (running == &proc[0]) {
    running = &proc[1];
    printf("Switching to User\n");
  }
  else {
    running = &proc[0];
    printf("Switching to Superuser\n");
  }
}

int access(char *pathname, char mode) {
  int ino, dev;
  MINODE *mip;

  if (pathname[0] == '/')
    dev = root->dev;
  else
    dev = running->cwd->dev;

  if (running == &proc[0]) {
    return 1;
  }
  ino = getino(pathname, &dev);
  if (ino == 0) {
    printf("Path %s does not exist\n", pathname);
  }
  mip = iget(dev, ino);
  if (mip->INODE.i_uid == running->uid) {
    switch(mode) {
      case 'r':
        if ((mip->INODE.i_mode & 0x100) == 0x100) {
          return 1;
        }
        break;
      case 'w':
        if ((mip->INODE.i_mode & 0x80) == 0x80) {
          return 1;
        }
        break;
      case 'x':
        if ((mip->INODE.i_mode & 0x40) == 0x40) {
          return 1;
        }
        break;
    }
  }
  else if (mip->INODE.i_gid == running->gid) {
    switch(mode) {
      case 'r':
        if ((mip->INODE.i_mode & 0x20) == 0x20) {
          return 1;
        }
        break;
      case 'w':
        if ((mip->INODE.i_mode & 0x10) == 0x10) {
          return 1;
        }
        break;
      case 'x':
        if ((mip->INODE.i_mode & 0x8) == 0x8) {
          return 1;
        }
        break;
    }
  }
  else {
    switch(mode) {
      case 'r':
        if ((mip->INODE.i_mode & 0x4) == 0x4) {
          return 1;
        }
        break;
      case 'w':
        if ((mip->INODE.i_mode & 0x2) == 0x2) {
          return 1;
        }
        break;
      case 'x':
        if ((mip->INODE.i_mode & 0x1) == 0x1) {
          return 1;
        }
        break;
    }
  }
  return 0;
}
