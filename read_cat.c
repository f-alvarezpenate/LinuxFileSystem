int read_file(char *descriptor, char *bytes)
{
    int fd = atoi(descriptor), nbytes = atoi(bytes); 
    char *buf[BLKSIZE];
    //printf("Enter an fd: ");
    //scanf("%d", &fd);
    //printf("Enter the number of bytes you would like to read: ");
    //scanf("%d", &nbytes);

    if(fd < 0 || fd > NFD-1)
    {
        printf("read_file error: invlaid file descriptor\n");
        return -1;
    }

    if(running->fd[fd]->mode != 0 && running->fd[fd]->mode != 2)
    {
        printf("read_file error: file is not opened for RD or RW\n");
        return -1;
    }
    printf("\n==================================");
    printf("\nSTART------------OF------------CAT\n");
    printf("==================================\n\n");
    if (fd >= 0){
        n = myread(fd, buf, nbytes);   
        buf[n] = 0;
        printf("%s", buf); // just printing the entirety of buf instead of going byte by byte 
    }
    printf("\n\n");
    printf("read %d chars from fd: %d\n", nbytes, fd);
    //return myread(fd, buf, nbytes);
}

int myread(int fd, char *buf, int nbytes)
{
    int count = 0;
    int lbk, start, remain, avil, blk;
    OFT *oftp = running->fd[fd];
    MINODE *mip = oftp->minodePtr;
    char *buf12, *buf13;
    avil = mip->INODE.i_size - oftp->offset;
    int intbuf12[BLKSIZE];
    int intbuf13[BLKSIZE];
    if(nbytes > avil)
    {
        nbytes = avil;
    }

    while(nbytes && avil)
    {
        lbk = oftp->offset / BLKSIZE;
        start = oftp->offset % BLKSIZE;

        if(lbk < 12) // direct blocks
        {
            blk = mip->INODE.i_block[lbk];
        }
        else if(lbk >= 12 && lbk < 256 + 12) // indirect blocks             // is responsible for "large" file
        {
            get_block(mip->dev, mip->INODE.i_block[12], intbuf12);
            blk = intbuf12[lbk - 12];
        }
        else // double indirect blocks                                      // is responsible for "huge" file
        {                                                                   // Hint: Mailman’s algorithm
            char* tempbuf[BLKSIZE];
            get_block(mip->dev, mip->INODE.i_block[13], tempbuf);    
            int block_size = (BLKSIZE / 4);
            lbk = lbk - block_size - 12;
            blk = tempbuf[lbk / block_size];
            get_block(mip->dev, blk, intbuf13);
            blk = intbuf13[lbk % block_size];
        }
         /* get the data block into readbuf[BLKSIZE] */
        char readbuf[BLKSIZE];
        get_block(mip->dev, blk, readbuf);

        /* copy from startByte to buf[ ], at most remain bytes in this block */
        char* cq = buf;             // cq points at buf[]
        char *cp = readbuf + start;   
        remain = BLKSIZE - start;   // number of bytes remain in readbuf[]

        if (nbytes <= remain)
        {                               // OPTIMIZATION: 
            memcpy(cq, cp, nbytes);     // copy nbytes worth of bytes from cp(readbuf) to cq(buf)
            cq += nbytes;               // rather than going one byte at a time
            cp += nbytes;               // than increment everything by nbytes instead of just by one
            count += nbytes;
            oftp->offset += nbytes;
            avil -= nbytes;
            remain -= nbytes;
            nbytes = 0;
        }
        else
        {
            memcpy(cq, cp, remain);
            cq += remain;
            cp += remain;
            count += remain;
            oftp->offset += remain;
            avil -= remain;
            nbytes -= remain;
            remain = 0;
        }
        // if one data block is not enough, loop back to OUTER while for more ...
    }
    //printf("myread: read %d char from file descriptor %d\n", count, fd);  
    return count;   // count is the actual number of bytes read
}

int cat_file(char* filename)
{
    char mybuf[1024], dummy = 0;
    int n;
    int total = 0;
    char mode[BLKSIZE];
    char* cp;
    strcpy(mode, "0");
    printf("Mode: %s", mode);
    int fd = open_file(filename, mode);
    printf("\n==================================");
    printf("\nSTART------------OF------------CAT\n");
    printf("==================================\n\n");
    if (fd >= 0){
        while(n = myread(fd, mybuf, BLKSIZE))
        {   
            total += n;
            mybuf[n] = 0;
            printf("%s", mybuf); // just printing the entirety of buf instead of going byte by byte
            // cp = mybuf;
            // while(*cp != '\0') // means we've reached the end
            // {
            //     if(*cp == '\n') // handles new lines
            //     {
            //         printf("\n");
            //     }
            //     else
            //     {
            //         printf("%c", *cp);  // printing what cp is pointing at in mybuf
            //     }
            //     cp++;   // incrementing by each byte
            // }
        }
    }
    printf("\n\n");
    printf("cat read %d chars from fd: %d\n", total, fd);
    sprintf(mode, fd);
    close_file(fd);
    return 0;
    

}