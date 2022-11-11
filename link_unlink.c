// link, unlink

int link(char* old_file,char* new_file){
    int oino = getino(old_file);
    MINODE* omip = iget(dev,oino);
    if (!S_ISREG(omip->INODE.i_mode)){
        printf("%s is not a file.\n", old_file);
        return;
    }
    if (getino(new_file)!=0){
        printf("%s must not exist yet.\n",new_file);
    }
    char temp1[64], temp2[64];
    strcpy(temp1, new_file);
    strcpy(temp2, new_file);
    char *parent = dirname(temp1);
    char *child = basename(temp2);
    int pino = getino(parent);
    MINODE* pmip = iget(dev,pino);
    enter_name(pmip, oino, child);
    omip->INODE.i_links_count++;
    omip->dirty = 1;
    iput(omip);
    iput(pmip);
}

int unlink(filename){
    int ino = getino(filename);
    MINODE* mip = iget(dev,ino);
    if(!S_ISREG(mip->INODE.i_mode)){
        printf("%s is not a file", filename);
        return;
    }
    char temp1[64], temp2[64];
    strcpy(temp1, filename);
    strcpy(temp2, filename);  

    char *parent = dirname(temp1);
    char *child = dirname(temp2);
    int pino = getino(parent);
    MINODE* pmip = iget(dev,ino);
    //rm_child(pmip,ino,child);
    pmip->dirty=1;
    iput(pmip);
    mip->INODE.i_links_count--;
    if(mip->INODE.i_links_count > 0)
        mip->dirty = 1;
    else{
        //deallocate all data blocks in inode;
        //deallocate inode;
    }
    iput(mip);
    
}