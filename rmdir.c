// rmdir, rm_child

int myrmdir(){
    //get in-memory inode of pathname
    char buf[BLKSIZE];
    int ino, pino;
    MINODE* mip, *pmip;
    char *cp, dname[124];
    DIR *dp;

    ino = getino(pathname);
    mip = iget(dev,ino);
    //verify its a DIR (by INODE.i_mode)
    if(!S_ISDIR(mip->INODE.i_mode)){
        printf("Pathname does not lead to a DIR.\n");
        return;
    }
    //check refCount = 1
    if (mip->refCount > 1){
        printf("Directory is being used by more than one user.\n");
        return;
    }
    //check its empty --> (traverse data blocks for number of entries = 2)
    if(mip->INODE.i_links_count > 2){
        printf("linkcount>2 Pathname DIR is not empty.\n"); 
        return;
    }
    else if(mip->INODE.i_links_count == 2){
        if(mip->INODE.i_block[1]){ //check to make sure block isnt empty
            get_block(dev, mip->INODE.i_block[1], buf);
            cp = buf;
            dp = (DIR*) buf;
            while(cp < buf + BLKSIZE){
                strncpy(dname, dp->name, dp->name_len);
                dname[dp->name_len] = 0; //kill last character
                printf("DIRECTORY NAMES: %s, ", dname); //its displaying empty strings: SOLVED block was empty
                if(strcmp(dname,".") != 0 && strcmp(dname, "..") != 0){
                    printf("Pathname DIR is not empty.\n"); // ERROR here: it keeps saying its not empty even though it is SOLVED
                    return;
                }
            }
        }
    }
    //get parent's ino and inode 
    pino = findino(mip, &ino); // get pino from .. entry in INODE.i_block[0]
    pmip = iget(mip->dev, pino);

    //get name from parent DIR's data block
    findmyname(pmip, ino, name); // find name from parent DIR

    // remove name from parent directory 
    rm_child(pmip, name);

    //dec parent links_count by 1; mark parent pmip dirty;
    pmip->INODE.i_links_count--;
    pmip->dirty = 1;
    iput(pmip);

    //deallocate its data blocks and inode
    bdalloc(mip->dev, mip->INODE.i_block[0]);
    idalloc(mip->dev, mip->ino);
    iput(mip);
}

int rm_child(MINODE *pmip, char *name){
    //search parent INODE's data blocks for the entry of name
    //Delete name entry from parent directory
    char buf[BLKSIZE];
    int i;
    char *cp, *temp_cp, temp[64];
    DIR *dp, *temp_dp, *prev;
    
    for(i = 0; i < 12; i++){
        if(pmip->INODE.i_block[i]==0)
            return;
        get_block(pmip->dev, pmip->INODE.i_block[i], buf);
        cp = buf;
        dp = (DIR*) buf;

        while(cp < buf + BLKSIZE){
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0; //kill the end
            if(strcmp(temp, name)==0){
                //found
                int ideal_length = 4 *((8 + dp->name_len + 3)/4);
                if(dp->rec_len == BLKSIZE)
                {  //first and only entry
                    bdalloc(pmip->dev, ip->i_block[i]);
                    pmip->INODE.i_size -= BLKSIZE;
                    
                    // while(pmip->INODE.i_block[i+1] && i + 1 < 12){
                    //     get_block(pmip->dev, pmip->INODE.i_block[i+1],buf);
                    //     put_block(pmip->dev, pmip->INODE.i_block[i],buf);
                    // }
                }
                else if(dp ->rec_len > ideal_length){
                    //remove last entry
                    prev->rec_len += dp->rec_len;
                    put_block(pmip->dev, pmip->INODE.i_block[i], buf);
                }
                else{
                    // entry is first but not the only entry or in the middle of a block
                    temp_dp = (DIR *) buf;
                    temp_cp = buf;
                    while(temp_cp + temp_dp->rec_len < buf + BLKSIZE){
                        temp_cp += temp_dp->rec_len;
                        temp_dp = (DIR *) temp_cp;
                    }
                    temp_dp->rec_len += dp->rec_len;
                    int block_length = (buf + BLKSIZE) - (cp + dp->rec_len);
                    memmove(cp, cp + dp->rec_len, block_length);
                    put_block(pmip->dev, pmip->INODE.i_block[i], buf);
                }

                pmip->dirty = 1;
                //iput(pmip);
                return;
            }
            prev = dp;
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }
    return;
}


