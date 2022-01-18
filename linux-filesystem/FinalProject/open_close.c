
int open_file(char *pathname, int mode) {
  int ino, i, dev;
  MINODE *mip;
  time_t curtime;
  OFT *oftp;
  char charmode;

  if (pathname[0] == '/') {
    dev = root->dev;
  }
  else {
    dev = running->cwd->dev;
  }

  ino = getino(pathname, &dev);

  if (ino == 0) {
    creat_file(pathname);
    ino = getino(pathname, &dev);
  }
  mip = iget(dev, ino);

  if (mode == 0) {
    charmode = 'r';
  }
  else {
    charmode = 'w';
  }

  if (!maccess(mip, charmode)) {
    iput(mip);
    return -1;
  }

  if ((mip->INODE.i_mode & 0xF000) != 0x8000) {
    printf("File must be regular type\n");
    iput(mip);
    return -1;
  }
  if (running->uid != 0 && mip->INODE.i_uid != running->uid) {
    printf("Invalid user permissions\n");
    iput(mip);
    return -1;
  }

  //for (i = 0; i < NFD; i++) {
  //  if (running->fd[i] != NULL && running->fd[i]->mptr == mip && running->fd[i]->mode > 0 || running->fd[i]->mode == 0 && mode != 0) {
  //    printf("File is already open and incompatible mode\n");
  //    return -1;
  //  }
  //}

  for (i = 0; i < NFD; i++) {
    if (running->fd[i] == NULL) {
      break;
    }
  }

  oftp = (OFT *)malloc(sizeof(OFT));

  oftp->mode = mode;
  oftp->mptr = mip;
  oftp->refCount = 1;

  switch(mode) {
    case 0: oftp->offset = 0;
            break;
    case 1: truncate(mip);
            oftp->offset = 0;
            break;
    case 2: oftp->offset = 0;
            break;
    case 3: oftp->offset = mip->INODE.i_size;
            break;
    default: printf("Invalid mode\n");
             return -1;
  }
  running->fd[i] = oftp;

  time(&curtime);
  if (mode == 0) {
    mip->INODE.i_atime = curtime;
  }
  else {
    mip->INODE.i_atime = curtime;
    mip->INODE.i_mtime = curtime;
  }

  mip->dirty = 1;

  return i;
}

int close_file(int fd) {
  OFT *oftp;
  MINODE *mip;

  if (fd >= NFD || fd < 0) {
    printf("FD is not in range\n");
    return -1;
  }
  if (running->fd[fd] == NULL) {
    printf("FD is not an open file\n");
    return -1;
  }

  oftp = running->fd[fd];
  running->fd[fd] = 0;
  oftp->refCount--;
  if (oftp->refCount > 0) {
    return 0;
  }
  mip = oftp->mptr;
  iput(mip);

  return 0;
}

int mylseek(int fd, int pos) {
  if (running->fd[fd] == NULL) {
    printf("File is not open\n");
    return -1;
  }
  if (pos < 0 || pos > running->fd[fd]->mptr->INODE.i_size) {
    printf("Position is out of bounds\n");
    return -1;
  }
  running->fd[fd]->offset = pos;
  return pos;
}

int pfd() {
  printf("fd\tmode\toffset\tINODE\n");
  printf("------------------------------\n");
  for (int i = 0; i < NFD; i++) {
    if (running->fd[i] != NULL) {
      printf("%d\t", i);
      switch(running->fd[i]->mode) {
        case 0: printf("READ\t");
                break;
        case 1: printf("WRITE\t");
                break;
        case 2: printf("R/W\t");
                break;
        case 3: printf("APPEND\t");
                break;
        default: printf("Invalid\t");
      }
      printf("%d\t[%d,%d]\n", running->fd[i]->offset, running->fd[i]->mptr->dev, running->fd[i]->mptr->ino);
    }
  }
}
