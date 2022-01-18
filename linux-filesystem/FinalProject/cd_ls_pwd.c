//Dylan LaFontaine & Hunter Ferrell
//On top of your hard copy: MARK yes or no
//  1. does your ls work?  ls; ls /dir1; ls /dir1/dir3_________YES______________
//  2. does your cd work?  cd /dir1; cd /dir1/dir3 ___________YES_______________
//  3. does your pwd work? ____________________YES______________________________
// NEED to check how to fix mount & umount

char lname[256];
bool flag;



/************* cd_ls_pwd.c file **************/

int chdir(char *pathname)
{
  int dev;

  if (pathname[0] == '/')
    dev = root->dev;
  else
    dev = running->cwd->dev;

  printf("chdir %s\n", pathname);
  //printf("under construction READ textbook HOW TO chdir!!!!\n");
  // READ Chapter 11.7.3 HOW TO chdir
  int ino = getino(pathname, &dev);
  if (!ino) {
    printf("ERROR: No pathname found\n");
    return -1;
  }
  MINODE *mip = iget(dev, ino);
  if ((mip->INODE.i_mode & 0x4000) != 0x4000) {
    printf("ERROR: Not a directory\n");
    return -1;
  }
  
  iput(running->cwd);
  running->cwd = mip;
}

char * readlink(char *pathname) {
  int ino, dev;
  MINODE *mip;
  char buf[BLKSIZE], *cp;
  DIR *dp;

  if (pathname[0] == '/')
    dev = root->dev;
  else
    dev = running->cwd->dev;

  ino = getino(pathname, &dev);
  if (!ino) {
    //printf("File does not exist\n");
    return -1;
  }
  mip = iget(dev, ino);

  if ((mip->INODE.i_mode & 0xF000) != 0xA000) {
    //printf("Not a symbolic link file\n");
    return -1;
  }

  get_block(dev, mip->INODE.i_block[0], buf);
  cp = buf;
  dp = (DIR *)buf;
  flag = 1;
  for (int i = 0; i < strlen(dp->name); i++) {
    if (!isalpha(dp->name[i]) && !isdigit(dp->name[i])) {
      flag = 0;
      break;
    }
  }

  if (0) {
    strcpy(lname, dp->name);
  }
  else {
    strcpy(lname, mip->INODE.i_block);
  }
  iput(mip);
  return lname;
}

int ls_file(MINODE *mip, char *name) {
  char *t1 = "xwrxwrxwr-------";
  char *t2 = "----------------";
  char *dtime;

  if ((mip->INODE.i_mode & 0xF000) == 0x8000)
    printf("%c", '-');
  if ((mip->INODE.i_mode & 0xF000) == 0x4000)
    printf("%c", 'd');
  if ((mip->INODE.i_mode & 0xF000) == 0xA000)
    printf("%c", 'l');
  for (int i=8; i >= 0; i--) {
    if (mip->INODE.i_mode & (1 << i))
      printf("%c", t1[i]);
    else
      printf("%c", t2[i]);
  }

  printf(" %d %d %-d \t%-d ", mip->INODE.i_uid, mip->INODE.i_gid, mip->INODE.i_size, mip->INODE.i_links_count);
  time_t *date = mip->INODE.i_ctime;
  dtime = ctime(&date);
  dtime[strlen(dtime) - 1] = 0;

  printf("\t%s %s", dtime, name);
  if ((mip->INODE.i_mode & 0xF000) == 0xA000) {
    printf(" -> %s", lname);

  }
  printf("\n");
}

int ls(char *pathname)
{
  int ino, dev;
  MINODE *mip;
  struct ext2_dir_entry_2 *dp;
  char *cp, buf[BLKSIZE], name[256], pathnamecpy[256];

  if (pathname[0] == '/')
    dev = root->dev;
  else
    dev = running->cwd->dev;

  if (strlen(pathname) == 0) {
    mip = running->cwd;
  } else {
    ino = getino(pathname, &dev);
    mip = iget(dev, ino);
  }

  get_block(dev, mip->INODE.i_block[0], buf);
  dp = (struct ext2_dir_entry_2 *)buf;
  cp = buf;

  while (cp < buf + BLKSIZE) {
    strncpy(name, dp->name, dp->name_len);
    name[dp->name_len] = 0;
    MINODE *pip = iget(dev, dp->inode);
    strcpy(pathnamecpy, pathname);
    strcat(pathnamecpy, "/");
    strcat(pathnamecpy, name);
    if ((pip->INODE.i_mode & 0xF000) == 0xA000) {
      readlink(pathnamecpy);
    }
    ls_file(pip, name);
    iput(pip);
    cp += dp->rec_len;
    dp = (struct ext2ext2_dir_entry_2 *)cp;
  }
  if (strlen(pathname) != 0) {
    iput(mip);
  }
}

char *pwd(MINODE *wd)
{
  //printf("pwd: READ HOW TO pwd in textbook!!!!\n");
  if (wd->dev != root->dev) {
    if (wd->dev == mountdev && wd->ino == mountino) {
      printf("/\n");
      return 0;
    }
    else {
      rpwd(wd);
    }
    printf("\n");
  }
  else {
    if (wd == root){
      printf("/\n");
      return 0;
    } 
    else {
      rpwd(wd);
    }
    printf("\n");
  }
}

void rpwd(MINODE *wd) {
  char buf[BLKSIZE], temp[256];
  struct ext2_dir_entry_2 *dp; //dir_entry pointer
  char *cp; //char pointer
  int my_ino, parent_ino;
  if (wd->dev != root->dev) {
    if (wd->dev == mountdev && wd->ino == mountino)
      return 0;
  }
  else {
    if (wd == root)
      return 0;
  }

  get_block(wd->dev, wd->INODE.i_block[0], buf); //gets block data into buf[]
  dp = (struct ext2_dir_entry_2 *)buf;
  cp = buf;
  my_ino = dp->inode;
  cp = buf + dp->rec_len;
  dp = (struct ext2_dir_entry_2 *)cp;
  parent_ino = dp->inode;

  MINODE *pip = iget(wd->dev, parent_ino);

  get_block(pip->dev, pip->INODE.i_block[0], buf);
  dp = (struct ext2_dir_entry_2 *)buf;
  cp = buf;

  while (cp < buf + BLKSIZE) {
    if (dp->inode == wd->ino) {
      strncpy(temp, dp->name, dp->name_len);
      temp[dp->name_len] = 0;
      break;
    }
    cp += dp->rec_len;
    dp = (struct ext2_dir_entry_2 *)cp;
  }

  rpwd(pip);
  printf("/%s", temp);
}
