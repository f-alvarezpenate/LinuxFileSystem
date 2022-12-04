int read_file()
{
    int fd = 0, nbytes = 0; 
    char *buf[BLKSIZE];
    printf("Enter an fd: ");
    scanf("%d", &fd);
    printf("Enter the number of bytes you would like to read: ");
    scanf("%d", &nbytes);

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

    return myread(fd, buf, nbytes);
}

int myread(int fd, char *buf, int nbytes)
{
    int count = 0;
    int lbk, start, remain, avil, blk;
    OFT *oftp = running->fd[fd];
    MINODE *mip = oftp->minodePtr;
    char *buf12, *buf13;
    avil = mip->INODE.i_size - oftp->offset;
    int ibuf12[BLKSIZE], ibuf13[BLKSIZE];
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
        else if(lbk >= 12 && lbk < 256 + 12) // indirect blocks                     
        {
            get_block(mip->dev, mip->INODE.i_block[12], buf12);
            blk = ibuf12[lbk - 12];
        }
        else // double indirect blocks
        {
            get_block(mip->dev, mip->INODE.i_block[13], (char *)ibuf12);     // Hint: Mailman’s algorithm
            int block_size = (BLKSIZE / sizeof(int));
            lbk = lbk - block_size - 12;
            blk = ibuf13[lbk / block_size];
            get_block(mip->dev, blk, ibuf13);
            blk = ibuf13[lbk % block_size];
        }
         /* get the data block into readbuf[BLKSIZE] */
        char readbuf[BLKSIZE];
        get_block(mip->dev, blk, readbuf);

        /* copy from startByte to buf[ ], at most remain bytes in this block */
        char* cq = buf;
        char *cp = readbuf + start;   
        remain = BLKSIZE - start;   // number of bytes remain in readbuf[]

        if (nbytes <= remain)
        {
            memcpy(cq, cp, nbytes);
            cq += nbytes;
            cp += nbytes;
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
            char* cp = mybuf;
            while(*cp != '\0') // means we've reached the end
            {
                if(*cp == '\n') // handles new lines
                {
                    printf("\n");
                }
                else
                {
                    printf("%c", *cp);
                }
                cp++;
            }
        }
    }
    printf("\n\n");
    printf("cat read %d chars from fd: %d\n", total, fd);
    sprintf(mode, fd);
    close_file(fd);
    return 0;
    

}