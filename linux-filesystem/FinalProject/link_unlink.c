int link(char *file1, char *file2) {
  int oino, pino, ino, dev;
  MINODE *omip, *pmip;
  char *parent, *child, file2cpy1[256], file2cpy2[256];

  if (file1[0] == '/')
    dev = root->dev;
  else
    dev = running->cwd->dev;

  strcpy(file2cpy1, file2);
  strcpy(file2cpy2, file2);
  parent = dirname(file2cpy1);
  child = basename(file2cpy2);

  oino = getino(file1, &dev);
  omip = iget(dev, oino);

  if ((omip->INODE.i_mode & 0xF000) == 0x4000) { //check for directory
    printf("Cannot link directories\n");
    return -1;
  }

  if (getino(file2, &dev)) {
    printf("File %s already exists\n", file2);
    return -1;
  }

  if (!getino(parent, &dev)) {
    printf("Directory does not exist\n");
    return -1;
  }

  pino = getino(parent, &dev);
  pmip = iget(dev, pino);
  //creat entry in new parent DIR with same inode number of the old file
  enter_name(pmip, oino, child);

  omip->INODE.i_links_count++;
  omip->dirty = 1;
  iput(omip);
  iput(pmip);
}

int unlink(char *pathname) {
  int ino, pino, dev;
  MINODE *mip, *pmip;
  char *parent, *child, pathcpy1[256], pathcpy2[256];

  if (pathname[0] == '/')
    dev = root->dev;
  else
    dev = running->cwd->dev;

  ino = getino (pathname, &dev);
  mip = iget(dev, ino);

  if (running != &proc[0] && mip->INODE.i_uid != running->uid) {
    printf("Ownership does not match\n");
    return -1;
  }

  if (!ino) {
    printf("File does not exist\n");
    return -1;
  }

  if ((mip->INODE.i_mode & 0xF000) == 0x4000) {
    printf("Cannot be a directory\n");
    return -1;
  }

  strcpy(pathcpy1, pathname);
  strcpy(pathcpy2, pathname);
  child = basename(pathcpy1);
  parent = dirname(pathcpy2);

  printf("PARENT = %s, CHILD = %s\n", parent, child);

  pino = getino(parent, &dev);
  pmip = iget(dev, pino);
  rm_child(pmip, child);
  pmip->dirty = 1;
  iput(pmip);
  mip->INODE.i_links_count--;
  if (mip->INODE.i_links_count > 0) {
    mip->dirty = 1;
  }
  else {
    //Dealloc all data blocks in inode
    //deallocate inode
    truncate(mip);
  }
  iput(mip);
}
