int write_file(char* descriptor, char* text){
    //1. ask for a fd and a text string to write;
    //e.g. write 1 123455009483059
    //2. verify fd is indeed opened for WR or RW or APPEND mode
    //3. copy the text string into a buf[] and get its length as nbytes.
    int fd = atoi(descriptor), nbytes = strlen(text);
    //char tempbuf[BLKSIZE];
    //printf("Enter a file descriptor: ");
    //scanf("%d", &fd);
    //printf("Enter what you would like to write: ");
    //scanf("%s", &tempbuf);
    
    //nbytes = strlen(tempbuf);
    printf("nbytes: %d\n", nbytes);
    OFT *oftp;
    //1. verify fd is within range.
    
    if (fd<0 || fd >= NFD){
        printf("write_file error: not in range.\n");
    }
    //2. verify running->fd[fd] is pointing at OFT entry
    int found;
    for (int i=0; i < NOFT; i++){
        oftp = &oft[i]; //oft = openfiletable. 
        if (oftp->minodePtr == running->fd[fd]->minodePtr){
            if(oftp->mode == 1 || oftp->mode == 2 || oftp->mode == 3){
                found = 1;
                break;
            }
            else{
                printf("write_file error: file is not open for WR, RW, or APPEND mode.\n");
            }
            
        }
        //check if no file was found by using ith index
    }
    if(!found){
        printf("write_file error: file not pointing at OFT entry.\n");
        return -1;
    }
    return mywrite(fd, text, nbytes);
}

int mywrite(int fd, char buf[], int nbytes){
    int count = 0;
    int lbk, start, remain, avil, blk;
    OFT *oftp = running->fd[fd];
    MINODE *mip = oftp->minodePtr;
    char* cq = buf;
    avil = mip->INODE.i_size - oftp->offset;
    int intbuf12[256];
    int intbuf13[256];
    char wbuf[BLKSIZE];
    

    // very similar to read except when you need to allocate space for blocks that dont already exist
    while (nbytes > 0){
        //compute logical block (lbk) and the startByte in that lbk:
        lbk = oftp->offset / BLKSIZE;
        start = oftp->offset % BLKSIZE;
        //only showed how to write DIRECT data blocks, we have to figure out how to 
        // write indirect data blocks and double indirect data blocks
    

        if (lbk < 12){ //direct blocks
            if (mip->INODE.i_block[lbk] == 0){//if no data block yet
                mip->INODE.i_block[lbk] = balloc(mip->dev); // MUST allocate a block
            }
            blk = mip->INODE.i_block[lbk]; // blk should be a disk block now
        }
        else if(lbk>= 12 && lbk < 256+12){ // indirect blocks       // not working for large file. SOLVED
            //HELP INFO:
            if (mip->INODE.i_block[12] == 0)                        // means we need to allocate space for it
            {
                mip->INODE.i_block[12] = balloc(mip->dev);          // set mip->INODE.i_block[12] = to a newly allocated block
                get_block(mip->dev, mip->INODE.i_block[12], intbuf12); // now go get that block into buf12
                for(int i = 0; i < BLKSIZE; i++)
                {
                    intbuf12[i] = 0;                                   // now loop through and set everything to 0
                }
                put_block(mip->dev, mip->INODE.i_block[12], intbuf12); // put the block back
            }
            get_block(mip->dev, mip->INODE.i_block[12], intbuf12);  // more general case, get the block wether its been zero'd out or not
            blk = intbuf12[lbk - 12];                                // doing minus twelve to start from zero 

            if(blk == 0)                                             // if the block there is 0 then we need to allocate a new block
                                                                     // means this blk location hasn't been written to yet
                                                                     // i think this happens everytime when we're writing to a new file
                                                                     // like we do with cp but if we were to overwrite a file it would
                                                                     // be skipping this pretty much everytime
            {
                blk = balloc(mip->dev);                              // set blk to the newly allocated block
                intbuf12[lbk - 12] = blk;                            // essentially saying "this is the block it points too now"
            }
            put_block(mip->dev, mip->INODE.i_block[12], intbuf12);   // put the block back
        }
        else{   //double inderect blocks    // not working for huge file 
            lbk = lbk - 268;                                          // zeroing out lbk like we did in read
            int dbuf[BLKSIZE];
            if(mip->INODE.i_block[13] == 0)                           // if it doesnt exist we need to allocate a block for it
            {
                mip->INODE.i_block[13] = balloc(mip->dev);            // set i_block13 to the new block
                get_block(mip->dev, mip->INODE.i_block[13], intbuf13);// then get that block into intbuf13

                for(int i = 0; i < BLKSIZE; i++)    
                {
                    intbuf13[i] = 0;                                   // zero out int buf
                }
                put_block(mip->dev, mip->INODE.i_block[13], intbuf13); // put the fresh, zero'd out buf back
            }
            get_block(mip->dev, mip->INODE.i_block[13], intbuf13);     // general case, get that same block

            blk = intbuf13[lbk / 256];                                 // similar to read, blk will only get to access the next spot in the intbuf array every 256 iterations
            if(blk == 0)                                               // if the blk at this doesnt exist then we need to allocate it
            {
                blk = balloc(mip->dev);                                 // here is where i think we should be setting to 0 again because it is
                                                                        // another block of pointers
                                                                        // its probably why we get some weird behavior but we can just tell KC we
                                                                        // figured it out after the fact
                                                                        // worst case scenario is he doesnt give us credit for CP but I really dont think he'll even do that

                intbuf13[lbk / 256] = blk;                              // this is the blk that this lbk location points to
            }

            put_block(mip->dev, mip->INODE.i_block[13], intbuf13);      // put thatt block back

            get_block(mip->dev, intbuf13[lbk / 256], dbuf);             // now grab the newly allocated block and put it into dbuf
            blk = dbuf[lbk % 256];                                      // doing the same thing as read where we'll loop through this 256 times before we go into another iteration of 
                                                                        // the previous block

            if(blk == 0)                                                 // if the block there is 0 then we need to allocate a new block
                                                                        // means this blk location hasn't been written to yet
                                                                        // i think this happens everytime when we're writing to a new file
                                                                        // like we do with cp but if we were to overwrite a file it would
                                                                        // be skipping this pretty much everytime
            {
                blk = balloc(mip->dev);                                     
                dbuf[lbk % 256] = blk;
            }
            put_block(mip->dev, intbuf13[lbk / 256], dbuf);               // then we put it back
            
        }
        // all cases come to here : write to the data block
        get_block(mip->dev, blk, wbuf); //read disk block into wbuf[]
        char *cp = wbuf + start; //cp points at startByte in wbuf[]
        remain = BLKSIZE - start; //number of BYTEs remain in this block

        if (remain <= nbytes){
            memcpy(cp, cq, remain); 
            //cq += remain;                             // again, incrementing these two when we're going blk by blk doesnt make 
            //cp += remain;                             // sense 
            oftp->offset += remain;                     // updating how far we've written to by the amount of bytes were about to write
            nbytes -= remain;                           
        }
        else
        {
            memcpy(cp, cq, remain);
            //cq += nbytes;
            //cp += nbytes;
            oftp->offset += nbytes;                      // updating how far we've written to by the amount of bytes were about to write
            nbytes -= nbytes;
        }
        if(oftp->offset > mip->INODE.i_size)            // if the offset of the file is bigger than the size of the inode then we just update the size of the file 
        {
            mip->INODE.i_size = oftp->offset;           // each time, probably a better way to do this but it works
        }
                
        put_block(mip->dev, blk, wbuf); //write wbuf[] to disk
    }
    // loop back to outer while to write more .... until nbytes are written
    mip->dirty = 1; //mark mip dirty for iput()
    //  `printf("wrote %d char into file descriptor fd=%d\n", nbytes, fd);
    return nbytes;
}

/*
cp src dest: 
1. fd = open src for READ;
2. gd open dst for WR|CREAT;
    NOTE: in the project you may have to creat the dst file first, then open it
    for WR, OR if open fails due to no file yet, creat it and then open it for WR
3. while(n=read(fd, buf[], BLKSIZE)){
    write(gd, buf, n); //notice the n in write()
}
*/
int cp_file(char* pathname, char* filename){
    if (!pathname){
        printf("cp error: no source\n");
        return;
    }
    else if(!filename){
        printf("cp error: no destination\n");
        return;
    }

    char buf[1024];
    int n;
    char modeR[BLKSIZE];
    char modeW[BLKSIZE];
    strcpy(modeR, "0"); //mode for read
    strcpy(modeW, "1"); //mode for write. 

    int fd = open_file(pathname, modeR);
    int gd = open_file(filename, modeW); // IF FILE DOESNT EXIST: open_file() already takes care of that
    
    while(n = myread(fd, buf, BLKSIZE))
    {
        //buf[n] = 0;                               // not needed, dont know what its for
        mywrite(gd, buf, n);
    }
    close_file(fd);
    close_file(gd);
}

/*
mv src dest:
1. verify src exists; get its INODE in ==> you already know its dev
2. check whether src is on the same dev as dest
    CASE 1: same dev:
3. Hard link dst with src (i.e. same INODE number)
4. unlink src (i.e. rm src name from its parent directory and reduce INODE's link count by 1)
    CASE 2: not the same dev:
3. cp src to dest
4. unlink src
*/