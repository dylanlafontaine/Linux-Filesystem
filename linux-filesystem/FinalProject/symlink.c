int symlink(char *file1, char *file2) {
  int oino, pino, ino, bno, dev;
  MINODE *omip, *pmip, *mip;
  char *parent1, *child1, *child2, *parent2, file2cpy1[256], file2cpy2[256], file1cpy1[256], file1cpy2[256], buf[BLKSIZE], *cp;
  DIR *dp;

  if (file1[0] == '/')
    dev = root->dev;
  else
    dev = running->cwd->dev;

  strcpy(file2cpy1, file2);
  strcpy(file2cpy2, file2);
  strcpy(file1cpy1, file1);
  strcpy(file1cpy2, file1);
  parent2 = dirname(file2cpy1);
  child2 = basename(file2cpy2);
  parent1 = dirname(file1cpy1);
  child1 = basename(file1cpy2);

  oino = getino(file1, &dev);
  if (!oino) {
    printf("File does not exist\n");
    return -1;
  }
  omip = iget(dev, oino);

  if (getino(file2, &dev)) {
    printf("File %s already exists\n", file2);
    return -1;
  }

  if (!getino(parent1, &dev)) {
    printf("Directory does not exist\n");
    return -1;
  }

  pino = getino(parent2, &dev);
  pmip = iget(dev, pino);

  creat_file(file2);
  ino = getino(file2, &dev);
  mip = iget(dev, ino);
  printf("refCount = %d\n", mip->refCount);
  mip->dirty = 1;

  mip->INODE.i_mode = (mip->INODE.i_mode & 0x0FFF) | 0xA000;

  if (strlen(child1) <= 60) {
    strncpy(&mip->INODE.i_block, child1, 60);
    mip->INODE.i_block[strlen(file1)] = 0;
    printf("\n\n%s\n\n", mip->INODE.i_block);
    mip->INODE.i_size = strlen(file1);
    mip->dirty = 1;
    iput(mip);
  }
  else {
  //need to create a block now for mip
    bno = balloc(dev);
    mip->INODE.i_block[0] = bno;
    mip->INODE.i_size = strlen(child1);
    mip->dirty = 1;
    iput(mip);

    bzero(buf, BLKSIZE);
    dp = (DIR *)buf;
    dp->inode = ino;
    dp->rec_len = strlen(child1);
    dp->name_len = strlen(child1);
    strcpy(dp->name, child1);
    put_block(dev, bno, buf);
  }
  //iput(mip);
  pmip->dirty = 1;
  iput(pmip);
  iput(omip);
}
