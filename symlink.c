// symlink, readlink

int symlink(char *old_file, char *new_file)
{
    char oldtemp[128], newtemp[128];
    strcpy(oldtemp, old_file);
    strcpy(newtemp, new_file);

    if(oldtemp[0] == '/')
    {
        dev = root->dev;
    }
    else
    {
        dev = running->cwd->dev;
    }
    
    int old_ino = getino(oldtemp);

    if(old_ino == -1)
    {
        printf("%s does not exist\n", oldtemp);
        return -1;
    }

    if(newtemp[0] == '/')
    {
        dev = root->dev;
    }
    else
    {
        dev = running->cwd->dev;
    }

    mycreat(newtemp);

    int new_ino = getino(newtemp);
    if(new_ino == -1)
    {
        printf("%s does not exist", newtemp);
        return -1;
    }

    MINODE *mip = iget(dev, new_ino);
    mip->INODE.i_mode = 0120000; // from man pages
    mip->dirty = 1;

    strncpy(mip->INODE.i_block, oldtemp, 84); // might need more room
    
    mip->INODE.i_size = strlen(oldtemp) + 1; // accounting for the null char

    mip->dirty = 1;
    iput(mip);
    
}
