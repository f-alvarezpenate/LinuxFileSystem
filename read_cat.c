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
    char buf12[BLKSIZE], buf13[BLKSIZE];
    int intbuf12[BLKSIZE], intbuf13[BLKSIZE];
    avil = mip->INODE.i_size - oftp->offset;    // total file size minus the number of bytes we have already written

    if(nbytes > avil)                           // keeps on going forever with out this code segment
    {
        nbytes = avil;
    }

    // this entire loop is to get the correct blk number that we want to read
    // we use lbk (logical block) to do this   
    while(nbytes && avil)
    {
        lbk = oftp->offset / BLKSIZE;           // basically amounts to how many times we have written BLKZISE number of bytes
                                                // because: i 1: offset = 0, i 2: offset = 1024, i 3: offset = 
                                                // so keeps track of number of loops

        start = oftp->offset % BLKSIZE;         // will only come into account on the last loop when offset isnt a multiple of BLKSIZE
                                                // every other time will just be 0 so we start from the first byte in the block

        if(lbk < 12) // direct blocks
        {
            blk = mip->INODE.i_block[lbk];                                  // the blk number we want is just at mip->INODE.i_block[0 through 11]
        }
        else if(lbk >= 12 && lbk < 256 + 12) // indirect blocks             // is responsible for "large" file
        {
            get_block(mip->dev, mip->INODE.i_block[12], intbuf12);          // NEEDS TO BE A INT BUFFER, DOES NOT WORK WITH CHAR BUFFER
                                                                            // inside of intbuf12 is a bunch of addresses
                                                                            // still the size of a block becuase ints take up 4 bytes

            blk = intbuf12[lbk - 12];                                       // do minus 12 to zero lbk out and start from 0 essentially 
                                                                            
        }
        else // double indirect blocks                                      // is responsible for "huge" file
        {                                                                   // Hint: Mailman’s algorithm

            int tempbuf[BLKSIZE];                                          // WORKS JUST AS WELL WITH INT RATHER THAN CHAR*
                                                                             // HE MIGHT ASK ABOUT THIS
                                                                             // if he asks we can just say something like:
                                                                             // when we were trying to figure out how to access the double indirect blocks
                                                                             // we ended up with this but we know it would've worked just as fine with int buf

            get_block(mip->dev, mip->INODE.i_block[13], tempbuf);            // inside tempbuf is 256 addresses to blocks that each contain 
                                                                             // 256 addresses


            //int block_size = (BLKSIZE / 4);                                // we maybe want to clean this up in the repo that we show KC
            lbk = lbk - 268;                                                 // account for the 12 direct blocks and 256 indirect blocks direct blocks 
                                                                             // so lbk - BLKSIZE / 4 - 12.
                                                                             // reseting to 0 because we need to count from 0 in tempbuf


            blk = tempbuf[lbk / 256];                                        // blk will only get to access the next spot in tempbuf every 256 iterations
                                                                             // i.e: at lbk = 256 we get 256 / 256 = 1
                                                                             // then we're in the next block that has pointers to other blocks
                                                                             // first lbk = 0 through 255 we are in tempbuf[0]

                                                                              
            get_block(mip->dev, blk, intbuf13);                               // we then get that buf in (i.e tempbuf[0]) into intbuf13
                                                                              // this still has pointers to bufs so we loop through them by using mod
            blk = intbuf13[lbk % 256];                                        // so 0 mod 256 = 0, 1 mod 256 = 1 etc..
                                                                              // then when we get to 256 we will be in tempbuf[1]
                                                                              // and since itll be 256 MOD 256, we will be back to 0 and will 
                                                                              // access intbuf13[0] AKA tempbuf[1][0].
                                                                              // then 257 mod 256 which will be be intbuf13[1] and so forth
        }
         /* get the data block into readbuf[BLKSIZE] */
        char readbuf[BLKSIZE];
        get_block(mip->dev, blk, readbuf);                                    // once we get here we know we have the right blk number 
                                                                              // so we get the block from that location into readbuf
                                                                              // and thats ultimately what we read

        /* copy from startByte to buf[ ], at most remain bytes in this block */
        char* cq = buf;             // cq points at buf[]
        char *cp = readbuf + start;   
        remain = BLKSIZE - start;   // number of bytes remain in readbuf[] // this will only ever change on the final loop since we're not going byte by byte

        // if the number of bytes is less than or equal to remain then we increment/decrement everything by nbyes
        if (nbytes <= remain)           
        {                               // OPTIMIZATION: 
            memcpy(cq, cp, nbytes);     // copy nbytes worth of bytes from cp(readbuf) into cq(buf)
                                        // rather than going one byte at a time
                                        // then increment everything by nbytes instead of just by one

            //cq += nbytes;               // REALIZED INCREMENTING CP AND CQ DOESNT DO ANYTHING
            //cp += nbytes;               // COULD CERTAINLY BE ASKED ABOUT THIS IF WE DONT CHANGE IT

            count += nbytes;            // count is just the number of bytes that that we have read so keep incrementing it by nybtes AKA BLKSIZE

            oftp->offset += nbytes;     // offset is the number of bytes that we've already read from the file

            avil -= nbytes;             // number of bytes left to read decremented by nbytes

                                        // I think this is the number of bytes left in the block and isnt used if we're not going byte by byte
                                        // could also ask about that
            nbytes = 0;
        }
        else // if nbytes is larger than remain then we increment/decrement everything by remain
        {
            memcpy(cq, cp, remain);
            //cq += remain;
            //cp += remain;
            count += remain;
            oftp->offset += remain;
            avil -= remain;
            nbytes -= remain;
            remain = 0;
        }
        // if one data block is not enough, loop back to OUTER while for more ... // comment no longer applicaple 
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