int rmdir(char *pathname) {
  int ino, numents, i, pino, dev;
  MINODE *mip, *pip;
  char *cp, buf[BLKSIZE], pathnamecpy[256], *child;
  DIR *dp;

  if (pathname[0] == '/')
    dev = root->dev;
  else
    dev = running->cwd->dev;

  strcpy(pathnamecpy, pathname);
  child = basename(pathnamecpy);

  //get inumber of pathname
  ino = getino(pathname, &dev);
  //get minode[ ] pointer
  mip = iget(dev, ino);
  
  if (running != &proc[0] && running->uid != mip->INODE.i_uid) {
    printf("Ownership does not match\n");
    return -1;
  }

  if ((mip->INODE.i_mode & 0xF000) != 0x4000) { //NOT a Directory
    printf("Not a directory\n");
    iput(mip);
    return -1;
  }
  printf("REFCOUNT = %d\n", mip->refCount);
  if (mip->refCount != 1) {
    printf("INODE is busy, cannot rmdir\n");
    iput(mip);
    return -1;
  }

  if (mip->INODE.i_links_count > 2) {
    printf("Directory not empty\n");
    iput(mip);
    return -1;
  }

  get_block(dev, mip->INODE.i_block[0], buf);
  dp = (DIR *)buf;
  cp = buf;
  numents = 0;

  while (cp < buf + BLKSIZE) {
    numents++;
    cp += dp->rec_len;
    dp = (DIR *)cp;
  }

  if (numents > 2) {
    printf("Directory not empty\n");
    iput(mip);
    return -1;
  }

  for (i=0; i<12; i++) {
    if (mip->INODE.i_block[i] != 0) {
      bdalloc(mip->dev, mip->INODE.i_block[i]);
    }
  }
  idalloc(mip->dev, mip->ino);
  iput(mip);

  pino = findino(mip, &ino);
  pip = iget(mip->dev, pino);

  rm_child(pip, child);

  pip->INODE.i_links_count--;
  pip->dirty = 1;
  iput(pip);
  return 1;
}
