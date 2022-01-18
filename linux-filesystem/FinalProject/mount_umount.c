
int mount(char *filesys, char *mountpt) {
    int i, fd, ino, dev;
    MTABLE *mountEntry;
    MINODE *mip;

    if (mountpt[0] == '/')
    dev = root->dev;
  else
    dev = running->cwd->dev;

    if (running->uid != 0) {
        printf("Must be Superuser to mount\n");
        return -1;
    }

    if (filesys == 0 && mountpt == 0) {
        for (i = 0; i < NMP; i++) {
            if (mtable[i].dev != 0) {
                printf("Mounted Filesystems:\n%s\n", mtable[i].name);
            }
        }
        return 1;
    }
    for (i = 0; i < NMP; i++) {
        if (strcmp(filesys, mtable[i].name) == 0) {
            printf("%s is already mounted\n", mtable[i].name);
            return -1;
        }
    }
    for (i = 0; i < NMP; i++) {
        if (mtable[i].dev == 0) {
            break;
        }
    }
    mountEntry = &mtable[i];
    fd = open(filesys, O_RDWR); //Maybe use Linux open?
    strncpy(mountEntry->name, filesys, 64);
    mountEntry->dev = fd;

    printf("checking EXT2 FS ....");
    if (fd < 0) {
        printf("open %s failed\n", filesys);
        return -1;
    }

    ino = getino(mountpt, &dev);
    if (!ino) {
        printf("Mount point %s does not exist\n", mountpt);
        return -1;
    }

    mip = iget(dev, ino);

    if ((mip->INODE.i_mode & 0xF000) != 0x4000) {
        printf("Mount point %s is not a directory\n", mountpt);
        return -1;
    }
    if (mip->refCount > 1) {
        printf("Mount point %s is in use\n", mountpt);
        return -1;
    }

    mountEntry->mptr = mip;
    mip->mounted = 1;
    mip->mptr = mountEntry;

    return 0;
}

int umount(char *filesys) {
    int i, j;

    if (running->uid != 0) {
        printf("Must be Superuser to umount\n");
        return -1;
    }

    for (i = 0; i < NMP; i++) { //Check that filesys is mounted
        if (strcmp(mtable[i].name, filesys) == 0) {
            printf("%s is mounted\n", filesys);
            break;
        }
    }
    for (j = 0; j < NMINODE; j++) {
        if (minode[j].dev == mtable[i].dev && minode[j].refCount > 1) {
            printf("%s is busy cannot umount\n", filesys);
            return -1;
        }
    }

    mtable[i].mptr->mounted = 0;
    iput(mtable[i].mptr);
    mtable[i].dev = 0;
    mtable[i].mptr = NULL;
    strcpy(mtable[i].name, "");

    return 0;
}