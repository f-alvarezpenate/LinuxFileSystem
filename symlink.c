// symlink, readlink

int symlink(char *old_file, char *new_file)
{
    char oldtemp[128], newtemp[128];
    strcpy(oldtemp, old_file);
    strcpy(newtemp, new_file);
    // check if starting at root or relative to current directory
    if(oldtemp[0] == '/')
    {
        dev = root->dev;
    }
    else
    {
        dev = running->cwd->dev;
    }
    
    int old_ino = getino(oldtemp);
    // old file must exist
    if(old_ino == -1)
    {
        printf("%s does not exist\n", oldtemp);
        return -1;
    }
    // same as above but for newfile
    if(newtemp[0] == '/')
    {
        dev = root->dev;
    }
    else
    {
        dev = running->cwd->dev;
    }
    if(getino(newtemp)!=-1){//aka it exists
        printf("new file %s cannot already exist\n", newtemp);
        return -1;
    }
    mycreat(newtemp); // create the newfile

    int new_ino = getino(newtemp);
    if(new_ino == -1)
    {
        printf("%s does not exist", newtemp);
        return -1;
    }

    MINODE *mip = iget(dev, new_ino);
    mip->INODE.i_mode = 0120000; // from man pages
    mip->dirty = 1;
    //store old_file name in newfile's INODE.iblock area
    strncpy(mip->INODE.i_block, oldtemp, 60); // assume old_file name <= 60 char
    
    mip->INODE.i_size = strlen(oldtemp) + 1; // accounting for the null char

    mip->dirty = 1; //mark newfiles minode dirty

    //get parent ino and minode
    int parent_ino = findino(running->cwd, &new_ino);
    MINODE *pmip = iget(mip->dev, parent_ino);
    
    pmip->dirty = 1; // mark new file's parent minode dirty

    iput(mip); //put back newfiles minode back

    iput(pmip); // put back newfiles parent minode back

    return 0;
}

int readlink(char* filename){
    char mybuf[BLKSIZE];
    int size = myreadlink(filename, mybuf);
    printf("%s file size = %d\n", filename, size);
    return 0;
}

int myreadlink(char* filename, char *buffer){
    //get file's INODE in memory 
    int ino = getino(filename);
    //verify its a LNK file
    MINODE *mip = iget(dev, ino);
    if (!S_ISLNK(mip->INODE.i_mode)){
        printf("%s is not a LNK file.\n", filename);
        return -1;
    }
    buffer = mip->INODE.i_block; // copy target filename from INODE.i_block into buffer
    int file_size = mip->INODE.i_size;
    iput(mip);
    return file_size; //ret filesize
}