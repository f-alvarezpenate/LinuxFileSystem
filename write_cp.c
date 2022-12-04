int write_file(){
    //1. ask for a fd and a text string to write;
    //e.g. write 1 123455009483059
    //2. verify fd is indeed opened for WR or RW or APPEND mode
    //3. copy the text string into a buf[] and get its length as nbytes.
    int fd = 0, nbytes = 0;
    char tempbuf[BLKSIZE];
    printf("Enter a file descriptor: ");
    scanf("%d", &fd);
    printf("Enter what you would like to write: ");
    scanf("%s", &tempbuf);
    
    nbytes = strlen(tempbuf);
    printf("nbytes: %d\n", nbytes);
    OFT *oftp;
    //1. verify fd is within range.
    
    if (fd<0 || fd >= NFD){
        printf("write_file error: not in range.\n");
    }
    //2. verify running->fd[fd] is pointing at OFT entry
    for (int i=0; i < NOFT; i++){
        oftp = &oft[i]; //oft = openfiletable. 
        if (oftp->minodePtr == running->fd[fd]->minodePtr){
            if(oftp->mode == 1 || oftp->mode == 2 || oftp->mode == 3){
                break;
            }
            else{
                printf("write_file error: file is not open for WR, RW, or APPEND mode.\n");
            }
            
        }
        //check if no file was found by using ith index
        if(i==NOFT-1){
                printf("write_file error: file not pointing at OFT entry.\n");
            }
    }

    return mywrite(fd, tempbuf, nbytes);
}

int mywrite(int fd, char buf[], int nbytes){
    int lbk = -1, startByte = -1, blk = -1, remain = -1;
    char buf12[BLKSIZE], buf13[BLKSIZE], wbuf[BLKSIZE];
    char *cq = buf; 
    OFT *oftp = running->fd[fd];
    MINODE *mip = oftp->minodePtr;
    
    while (nbytes > 0){
        //compute logical block (lbk) and the startByte in that lbk:
        lbk = oftp->offset / BLKSIZE;
        startByte = oftp->offset % BLKSIZE;
        //only showed how to write DIRECT data blocks, we have to figure out how to 
        // write indirect data blocks and double indirect data blocks
    

        if (lbk < 12){ //direct block
            if (mip->INODE.i_block[lbk] == 0){//if no data block yet
                mip->INODE.i_block[lbk] = balloc(mip->dev); // MUST allocate a block
            }
            blk = mip->INODE.i_block[lbk]; // blk should be a disk block now
        }
        else if(lbk>= 12 && lbk < 256+12){ // inderect blocks
            //HELP INFO:
            if (mip->INODE.i_block[12] == 0){
                //allocate a block for it;
                mip->INODE.i_block[12] = balloc(mip->dev);
                //zero out the block on disk;
                get_block(mip->dev, mip->INODE.i_block[12], buf12);
                for (int i = 0; i < BLKSIZE; i++)
                    buf12[i] = 0;
                put_block(mip->dev, mip->INODE.i_block[12], buf12);
                mip->INODE.i_blocks++;
            }
            //get i_block[12] into an int ibuf[256];
            get_block(mip->dev, mip->INODE.i_block[12], buf12);
            blk = buf12[lbk-12];
            if (blk==0){
                //allocate a disk block;
                blk = balloc(mip->dev);
                buf12[lbk - 12] = blk;
                mip->INODE.i_blocks++;
                //record it in i_block[12];
            }
            put_block(mip->dev, mip->INODE.i_block[12], buf12);
        }
        else{   //double inderect blocks
            int ibuf[256], dbuf[256];
            if(mip->INODE.i_block[13] == 0)
            {
                mip->INODE.i_block[13] = balloc(mip->dev);
                get_block(mip->dev, mip->INODE.i_block[13], buf13);

                for(int i = 0; i < BLKSIZE; i++)
                {
                    buf13[i] = 0;
                }

                put_block(mip->dev, mip->INODE.i_block[13], buf13);
                mip->INODE.i_blocks++;
            }
            get_block(mip->dev, mip->INODE.i_block[13], ibuf);
            blk = ibuf[(lbk - 256 - 12) / 256];
            if(blk == 0)
            {
                blk = balloc(mip->dev);
                ibuf[(lbk - 256 - 12) % 256] = blk;
            }
            put_block(mip->dev, mip->INODE.i_block[13], ibuf);
            mip->INODE.i_blocks++;

            get_block(mip->dev, ibuf[(lbk - 256 - 12) / 256], dbuf);
            blk = dbuf[(lbk - 256 - 12) % 256];
            if(blk == 0)
            {
                blk = balloc(mip->dev);
                dbuf[(lbk - 256 - 12) % 256] = blk;
            }
            mip->INODE.i_blocks++;
            put_block(mip->dev, ibuf[(lbk - 256 -12) / 256], dbuf);
        }
        // all cases come to here : write to the data block
        get_block(mip->dev, blk, wbuf); //read disk block into wbuf[]
        char *cp = wbuf + startByte; //cp points at startByte in wbuf[]
        remain = BLKSIZE - startByte; //number of BYTEs remain in this block

        while (remain>0){//write as much as remain allows
            *cp++ = *cq++; //cq points at buf[] // SEG FAULT HERE
            nbytes--; remain--; //dec counts
            oftp->offset++; //advance offset
            if(oftp->offset > mip->INODE.i_size) //especially for RW|APPEND mode
                mip->INODE.i_size++; // inc file size (if offset > fileSize)
            if(nbytes<=0) break; // if already nbytes, break
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
        buf[n] = 0;
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