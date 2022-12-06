// link, unlink

int link(char* old_file,char* new_file){
    //verify old file exists and is not a DIR
    int oino = getino(old_file);
    MINODE* omip = iget(dev,oino);
    if (!S_ISREG(omip->INODE.i_mode)){
        printf("%s is not a file.\n", old_file);
        return;
    }
    if (getino(new_file)!=0){
        printf("%s must not exist yet.\n",new_file);
    }
    //creat new_file with the same inode number of old_file
    char temp1[64], temp2[64];
    strcpy(temp1, new_file);
    strcpy(temp2, new_file);
    char *parent = dirname(temp1);
    char *child = basename(temp2);
    int pino = getino(parent);
    MINODE* pmip = iget(dev,pino);
    //creat entry in parent DIR with same inode number of old_file
    enter_name(pmip, oino, child);
    omip->INODE.i_links_count++; // inc INODE's links_count by 1
    omip->dirty = 1; //mark as dirty 
    iput(omip); //write back
    iput(pmip);
}

int unlink(char* filename){
    //get filename's minode
    int ino = getino(filename);
    MINODE* mip = iget(dev,ino);
    //check its REG or symbolic LNK file; in conclusion can not be a DIR
    if(S_ISDIR(mip->INODE.i_mode)){
        printf("%s is not a file", filename);
        return;
    }
    //remove name entry from parent DIR's data block
    char temp1[64], temp2[64];
    strcpy(temp1, filename);
    strcpy(temp2, filename);  

    char *parent = dirname(temp1);
    char *child = basename(temp2);
    int pino = getino(parent);
    MINODE* pmip = iget(dev,pino);
    rm_child(pmip, child);
    pmip->dirty=1;
    iput(pmip);
    //decrement INODE's link_count by 1
    mip->INODE.i_links_count--;
    if(mip->INODE.i_links_count > 0)
        mip->dirty = 1; // dirty
    else{ //if links count = 0 
        //deallocate all data blocks in inode;
        //deallocate inode;
        for(int i = 0; i < 12; i++)
        {
            if(mip->INODE.i_block[i] == 0)
            {
                break;
            }
            bdalloc(dev, mip->INODE.i_block[i]); //deallocate all data blocks in inode
        }
        idalloc(dev, ino); //deallocate inode
    }
    iput(mip);
    
}