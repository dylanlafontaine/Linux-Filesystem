
int minimum(int i, int j) {
  if (i < j) {
    return i;
  }
  else {
    return j;
  }
  return -1;
}

int read_file(int fd, int nbytes) {
  char buf[BLKSIZE];
  if (running->fd[fd] == NULL) {
    printf("File is not open\n");
    return -1;
  }
  if (running->fd[fd]->mode == 1 || running->fd[fd]->mode == 3) {
    printf("File is not open for reading\n");
    return -1;
  }
  return myread(fd, buf, nbytes);
}

int myread(int fd, char *buf, int nbytes) {
  int lbk, start, remain, avail, count, blk, dblk, ibuf[BLKSIZE], buf13[256], dbuf[256];
  char *cq = buf;
  char readBuf[BLKSIZE];
  MINODE *mip;
  OFT *oftp;

  oftp = running->fd[fd];
  mip = oftp->mptr;

  start = oftp->offset % BLKSIZE;
  remain = BLKSIZE - start;
  avail = mip->INODE.i_size - oftp->offset;
  count = 0;

  //printf("REMAIN = %d\n", remain);
  //printf("START = %d\n", start);
  //printf("AVAIL = %d\n", avail);

  while (nbytes && avail) {
    lbk = oftp->offset / BLKSIZE;
    //printf("LBK = %d\n", lbk);
    start = oftp->offset % BLKSIZE;
    //printf("START = %d\n", start);

    if (lbk < 12) {
      blk = mip->INODE.i_block[lbk];
      //printf("BNO = %d\n", blk);
    }
    else if (lbk >= 12 && lbk < 256 + 12) {
      get_block(mip->dev, mip->INODE.i_block[12], ibuf);
      blk = ibuf[lbk - 12];
      //printf("BNO = %d\n", blk);
    }
    else {
      lbk -= (12 + 256);
      get_block(mip->dev, mip->INODE.i_block[13], buf13);
      dblk = buf13[lbk / 256];
      get_block(mip->dev, dblk, dbuf);
      blk = dbuf[lbk % 256];
    }
    get_block(mip->dev, blk, readBuf);
    //printf("%s", buf);
    char * cp = readBuf + start;
    remain = BLKSIZE - start;

    int min = minimum(minimum(nbytes, avail), remain);

    memcpy(cq, cp, min);
    oftp->offset += min;
    count += min;
    avail -= min;
    nbytes -= min;
    remain -= min;
  }
  return count;
}

int cat_file(char *filename) {
  char mybuf[BLKSIZE], dummy = 0;
  int n, fd, i;
  fd = open_file(filename, 0);
  //printf("MYBUF = %s\n", mybuf);
  while (n = myread(fd, mybuf, BLKSIZE)) {
    mybuf[n] = 0;
    i = 0;
    while (mybuf[i]) {
      printf("%c", mybuf[i]);
      if (mybuf[i] == '\n') {
        printf("\n");
      }
      i++;
    }
  }
  close_file(fd);
}
