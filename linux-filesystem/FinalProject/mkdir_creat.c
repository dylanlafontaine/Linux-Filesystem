int make_dir(char *pathname) {
  char pathnamecpy[256], pathnamecpy2[256], *parent, *child, buf[BLKSIZE], *cp;
  char name[256];
  int pino, dev;
  MINODE *start, *pip;
  struct ext2_dir_entry_2 *dp;

  if (pathname[0] == '/') {
    start = root;
    dev = root->dev;
  } else {
    start = running->cwd;
    dev = running->cwd->dev;
  }

  strcpy(pathnamecpy, pathname);
  strcpy(pathnamecpy2, pathname);
  parent = dirname(pathnamecpy);
  child = basename(pathnamecpy2);

  pino = getino(parent, &dev);
  pip = iget(dev, pino);

  if (!maccess(pip, 'w')) {
    printf("NO WRITE permission in parent DIR\n");
    iput(pip);
    return -1;
  }

  if ((pip->INODE.i_mode & 0xF000) != 0x4000) { //NOT directory
    printf("ERROR: Not a directory\n");
    return -1;
  }

  get_block(dev, pip->INODE.i_block[0], buf);
  dp = (struct ext2_dir_entry_2 *)buf;
  cp = buf;

  while (cp < buf + BLKSIZE) {
    strncpy(name, dp->name, dp->name_len);
    name[dp->name_len] = 0;
    if (strcmp(name, child) == 0) {
      printf("ERROR: Directory already exits\n");
      return -1;
    }
    cp += dp->rec_len;
    dp = (DIR *)cp;
  }

  mymkdir(pip, child);
  pip->INODE.i_links_count++;
  pip->dirty = 1;

  iput(pip);
}

int mymkdir(MINODE *pip, char *name) {
  MINODE *mip;
  int ino, bno;
  char buf[BLKSIZE];

  ino = ialloc(pip->dev);
  bno = balloc(pip->dev);
  printf("ino = %d bno = %d\n", ino, bno);

  //WE are making the INODE here
  mip = iget(pip->dev, ino);
  INODE *ip = &mip->INODE;

  ip->i_mode = 0x41ED;
  ip->i_uid = running->uid;
  ip->i_gid = running->gid;
  ip->i_size = BLKSIZE;
  ip->i_links_count = 2;
  ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
  ip->i_blocks = 2;
  ip->i_block[0] = bno;
  for (int i = 1; i < 15; i++) {
    ip->i_block[i] = 0;
  }
  mip->dirty = 1;
  iput(mip);

  //Create data block for new DIR containing . & ..
  bzero(buf, BLKSIZE);
  DIR *dp = (DIR *)buf;
  //make . entry
  dp->inode = ino;
  dp->rec_len = 12;
  dp->name_len = 1;
  dp->name[0] = '.';
  //make .. entry
  dp = (char *)dp + 12;
  dp->inode = pip->ino;
  dp->rec_len = BLKSIZE - 12;
  dp->name_len = 2;
  dp->name[0] = dp->name[1] = '.';
  put_block(pip->dev, bno, buf);

  enter_name(pip, ino, name);
}

int enter_name(MINODE *pip, int ino, char *name) {
  int i = 0;
  char buf[BLKSIZE], *cp, temp[256];
  int ideal_len, need_length, remain, bno;
  DIR *dp;

  need_length = 4 * ((8 + strlen(name) + 3) / 4);

  for (i; i < 12; i++) {
    if (pip->INODE.i_block[i] == 0)
      break;
    get_block(pip->dev, pip->INODE.i_block[i], buf);
    dp = (DIR *)buf;
    cp = buf;

    printf("step to LAST entry in data block %d\n", i);
    while (cp + dp->rec_len < buf + BLKSIZE) {
      strncpy(temp, dp->name, dp->name_len);
      temp[dp->name_len] = 0;
      printf("%s\n", temp);
      cp += dp->rec_len;
      dp = (DIR *)cp;
    }
    //dp now points to last entry in block
    ideal_len = 4 * ((8 + dp->name_len + 3)/4);
    remain = dp->rec_len - ideal_len;
    if (remain >= need_length) {
      dp->rec_len = ideal_len;
      printf("%s\n", dp->name);
      cp += dp->rec_len;
      dp = (DIR *)cp; //this is the entry we are adding
      dp->inode = ino;
      dp->rec_len = remain;
      dp->name_len = strlen(name);
      strcpy(dp->name, name);
      printf("%s\n", dp->name);
    } else {
      bno = balloc(pip->dev);
      ino = ialloc(pip->dev);
      pip->INODE.i_size += BLKSIZE;
      //create a new data block
      bzero(buf, BLKSIZE);
      DIR *ndp = (DIR *)buf;
      //make new entry
      ndp->inode = ino;
      ndp->rec_len = BLKSIZE;
      ndp->name_len = strlen(name);
      strcpy(ndp->name, name);
      put_block(pip->dev, bno, buf);
    }
    put_block(pip->dev, pip->INODE.i_block[i], buf);
  }
}

int creat_file(char *pathname) {
  char pathnamecpy[256], pathnamecpy2[256], *parent, *child, buf[BLKSIZE], *cp;
  char name[256];
  int pino, dev;
  MINODE *start, *pip;
  struct ext2_dir_entry_2 *dp;

  if (pathname[0] == '/') {
    start = root;
    dev = root->dev;
  } else {
    start = running->cwd;
    dev = running->cwd->dev;
  }

  strcpy(pathnamecpy, pathname);
  strcpy(pathnamecpy2, pathname);
  parent = dirname(pathnamecpy);
  child = basename(pathnamecpy2);

  pino = getino(parent, &dev);
  pip = iget(dev, pino);

  if (!maccess(pip, 'w')) {
    printf("NO WRITE permission in parent DIR\n");
    iput(pip);
    return -1;
  }

  if ((pip->INODE.i_mode & 0xF000) != 0x4000) { //NOT directory
    printf("ERROR: Not a directory\n");
    return -1;
  }

  get_block(dev, pip->INODE.i_block[0], buf);
  dp = (struct ext2_dir_entry_2 *)buf;
  cp = buf;

  while (cp < buf + BLKSIZE) {
    strncpy(name, dp->name, dp->name_len);
    name[dp->name_len] = 0;
    if (strcmp(name, child) == 0) {
      printf("ERROR: File already exits\n");
      return -1;
    }
    cp += dp->rec_len;
    dp = (struct ext2ext2_dir_entry_2 *)cp;
  }

  my_creat(pip, child);
  pip->dirty = 1;

  iput(pip);
}

int my_creat(MINODE *pip, char *name) {
  MINODE *mip;
  int ino;
  char buf[BLKSIZE];

  ino = ialloc(pip->dev);
  printf("ino = %d\n", ino);

  //WE are making the INODE here
  mip = iget(pip->dev, ino);
  INODE *ip = &mip->INODE;

  ip->i_mode = 0x81A4;
  ip->i_uid = running->uid;
  ip->i_gid = running->gid;
  ip->i_size = 0;
  ip->i_links_count = 1;
  ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
  ip->i_blocks = 2;
  ip->i_block[0] = 0;
  for (int i = 1; i < 15; i++) {
    ip->i_block[i] = 0;
  }
  mip->dirty = 1;
  iput(mip);
  enter_name(pip, ino, name);
}
