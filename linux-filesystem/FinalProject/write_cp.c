
int write_file(int fd, char *text) {
  char buf[BLKSIZE];
  if (running->fd[fd] == NULL) {
    printf("File is not opened\n");
    return -1;
  }
  if (running->fd[fd]->mode != 1) {
    printf("File is not open for write\n");
    return -1;
  }
  return mywrite(fd, buf, strlen(text));
}

int mywrite(int fd, char *buf, int nbytes) {
  int lbk, start, ibuf[256], buf13[256], dbuf[256], remain, dblk, blk, ntemp;
  char wbuf[BLKSIZE];
  char *cq = buf;
  OFT *oftp;
  MINODE *mip;
  oftp = running->fd[fd];
  mip = oftp->mptr;
  ntemp = nbytes;

  while (nbytes > 0) {
    lbk = oftp->offset / BLKSIZE;
    start = oftp->offset % BLKSIZE;

    if (lbk < 12) {
      if (mip->INODE.i_block[lbk] == 0) {
        mip->INODE.i_block[lbk] = balloc(mip->dev);
      }
      blk = mip->INODE.i_block[lbk];
    }
    else if (lbk >= 12 && lbk < 256 + 12) {
      if (mip->INODE.i_block[12] == 0) {
        mip->INODE.i_block[12] = balloc(mip->dev);
      }
      get_block(mip->dev, mip->INODE.i_block[12], ibuf);
      blk = ibuf[lbk - 12];
      if (blk == 0) {
        ibuf[lbk - 12] = balloc(mip->dev);
        blk = ibuf[lbk - 12];
        put_block(mip->dev, mip->INODE.i_block[12], ibuf);
      }
    }
    else {
      if (mip->INODE.i_block[13] == 0) {
        mip->INODE.i_block[13] = balloc(mip->dev);
      }
      lbk -= (12 + 256);
      get_block(mip->dev, mip->INODE.i_block[13], buf13);
      dblk = buf13[lbk / 256];
      if (dblk == 0) {
        buf13[lbk / 256] = balloc(mip->dev);
        dblk = buf13[lbk / 256];
        put_block(mip->dev, mip->INODE.i_block[13], buf13);
      }
      get_block(mip->dev, dblk, dbuf);
      blk = dbuf[lbk % 256];
      if (blk == 0) {
        dbuf[lbk % 256] = balloc(mip->dev);
        blk = dbuf[lbk % 256];
        put_block(mip->dev, buf13[lbk / 256], dbuf);
      }
    }

    get_block(mip->dev, blk, wbuf); //after this wbuf should be all 0s
    char *cp = wbuf + start;
    remain = BLKSIZE - start;

    int min = minimum(nbytes, remain);

    while (remain > 0) {
      *cp++ = *cq++;
      nbytes--;
      remain--;
      oftp->offset++;
      if (oftp->offset > mip->INODE.i_size)
        mip->INODE.i_size++;
      if (nbytes <= 0)
        break;
    }
    put_block(mip->dev, blk, wbuf);
  }
  mip->dirty = 1;
  printf("Wrote %d char into file descriptor fd=%d\n", ntemp, fd);
  return ntemp;
}

int cp(char *src, char *dest) {
  int fd, gd, n;
  char buf[BLKSIZE];

  printf("DEST = %s\n", dest);
  printf("SRC = %s\n", src);

  gd = open_file(dest, 1);
  fd = open_file(src, 0);
  while (n = myread(fd, buf, BLKSIZE)) {
    mywrite(gd, buf, n);
  }
  close_file(gd);
  close_file(fd);
}

int mv(char *src, char *dest) {
  int ino, dev;
  MINODE *mip;

  if (src[0] == '/')
    dev = root->dev;
  else
    dev = running->cwd->dev;

  ino = getino(src, &dev);
  if (ino == 0) {
    printf("src %s does not exist\n", src);
    return -1;
  }
  mip = iget(running->cwd->dev, ino);
}
