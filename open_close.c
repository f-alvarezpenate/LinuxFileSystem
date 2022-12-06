//#include "type.h"
int open_file(char* pathname, char* mode)
{
    int imode = atoi(mode);
    char ptemp[BLKSIZE], buf[BLKSIZE];
    if (imode < 0 || imode >= 4)
    {
        printf("open_file error: mode not recognized\n");
        return -1;
    }
    char* pathtemp[128];
    strcpy(pathtemp, pathname);

    if(pathname[0] == '/')
    {
        dev = root->dev;
    }
    else
    {
        dev = running->cwd->dev;
    }

    int ino = getino(pathname);

    if(ino == -1) // must creat the file
    {
        strcpy(buf, pathname);
        strcpy(ptemp, dirname(buf));
        int pino = getino(ptemp);
        if(pino == -1)
        {
            printf("open_file error: couldn't find parent inode\n");
            return -1;
        }
        MINODE *pmip = iget(dev, pino);

        mycreat(pathname);
        ino = getino(pathname);
    }

    MINODE *mip = iget(dev, ino);

    if(!S_ISREG(mip->INODE.i_mode))
    {
        iput(mip);
        printf("open_file error: file is not regular\n");
        return -1;
    }
    OFT *oftp = (OFT *)malloc(sizeof(OFT));
    oftp->mode = imode;
    oftp->refCount = 1;
    oftp->minodePtr = mip;
    int offset;
    switch(imode){
        case 0 : offset = 0;      // R: offset = 0
                 break;
        case 1 : truncate(mip);         // W: truncate file to 0 size
                 offset = 0;
                 break;
        case 2 : offset = 0;      // RW: do NOT truncate file
                 break;
        case 3 : offset = mip->INODE.i_size;  // APPEND mode
                 break;
        default: printf("invalid mode\n");
                 return(-1);
    }
    oftp->offset = offset;
    int result = -1;
    int i;
    for (i = 0; i < NFD; i++)
    {
        if (running->fd[i] == NULL) //empty process found. We can open
        {
            running->fd[i] = oftp;
            result = i;
            break;
        }
        else if(running->fd[i]->minodePtr == mip) // file already opened. If not opened for read ret error bc files can only be open for more than 1 mode if one of them is read
        {
            if(imode != 0)
            {
                printf("open_file error: file is already open with incompatible mode\n");
                return -1;
            }
        }
    }
    if(result == -1){ // no open process was found. Result variable not updated. Cant open file
        printf("open_file error: no avail processes.\n");
        iput(mip);
        return -1;
    }
    //add file to oft array created in main so that we can keep track of opened files easily with pfd() function
    int j;
    int found;
    for (j = 0; j < NOFT; j++){
        if (oft[j].refCount==0){
            oft[j].mode = imode;
            oft[j].minodePtr = mip;
            oft[j].refCount++;
            found = 1; // found is true. Dont give error
            break;
        }
    }
    if (!found){
        printf("open_file error: Too many files opened at once.\n");
        iput(mip);
        return;
    }
    //printf("INSIDE OPEN(), OUTSIDE OF LOOP. REFCOUNT = %d\n", oft[i].refCount);
    mip->INODE.i_atime = time(NULL);
    mip->dirty = 1;
    // iput(mip); //kc online thingy said not to put() here bc we have to wait until the file is closed

    return result;
}

int truncate(MINODE* mip){
    //1. release mip->INODEs data block
    // a file may have 12 direct data blocks, 256 indirect data blocks and 256*256
    // double inidirect data blocks. release them all
    //2. update INODE's time field
    //3. set INODE's size to 0 and mark Minode[] dirty

    char buf12[BLKSIZE], buf13[BLKSIZE]; 
    INODE *ip = &mip->INODE;

    for (int i = 0; i < 12; i++) // 12 first direct blocks
    {
        if(ip->i_block[i] == 0)
        {
            break;
        }

        bdalloc(dev, ip->i_block[i]);
        ip->i_block[i] = 0;
    }

    if(ip->i_block[12] != NULL) // first indirect blocks
    {
        get_block(dev, ip->i_block[12], buf12);
        int* buf_ref12 = (int *) buf12;

        for (int i = 0; i < BLKSIZE / sizeof(int); i++)
        {
            if(buf_ref12[i] == 0)
            {
                break;
            }
            bdalloc(dev, buf_ref12[i]);
            buf_ref12[i] = 0;
        }
        bdalloc(dev, ip->i_block[12]);
        ip->i_block[12] = 0;
    }

    if(ip->i_block[13] != NULL) // doubly indirect blocks need to be dealt with
    {
        get_block(dev, ip->i_block[13], buf13);
        int* buf_ref13 = (int *)buf13;
        for(int i = 0; i < BLKSIZE / sizeof(int); i++)
        {
            if(buf_ref13[i] == 0)
            {
                break;
            }
            bdalloc(dev, buf_ref13[i]);
            buf_ref13[i] = 0;
        }

        bdalloc(dev, ip->i_block[13]);
        ip->i_block[13] = 0;
    }

    mip->INODE.i_blocks = 0;
    mip->INODE.i_size = 0;
    mip->dirty = 1;
    iput(mip);

}

int close_file(int fd){ //PROBLEM: if you open 2 files and close 1 of them. Closing the second one as well gives segmentation fault. SOLVED!
    
    //printf("INSIDE CLOSE: fd=%d\n", fd);
    //printf("FD: %d\n", fd);
    int i;
    OFT *oftp, *temp;
    MINODE* mip;
    //1. verify fd is within range.
    
    if (fd<0 || fd >= NFD){
        printf("close_file error: not in range.\n");
    }
    //2. verify running->fd[fd] is pointing at OFT entry
    int found;
    for (i=0; i < NOFT; i++){
        oftp = &oft[i]; //oftp points at address of ith element in open file table 
        if (oftp->minodePtr == running->fd[fd]->minodePtr){
            oftp->refCount--; // do this to update oft global array var so that the file is no longer displayed by pfd()
            found = 1;
            break;
        }
    }
    if (!found){
        printf("close_file error: file is not an OFT entry.\n");
        return -1;
    }
    //printf("refcount = %d\n", oftp->refCount);
    //3. The following code segments should be fairly obvious:
    oftp = running->fd[fd];
    //printf("fd[fd]: refcount = %d\n", oftp->refCount); // refcount is unaffected by line 196 decrease since this refers to running->fd[fd] and not just the array keeping track
    oftp->refCount--; //                                  so we decrease it again in this line 
    //printf("refcount-- = %d\n", oftp->refCount);

    running->fd[fd] = 0;
    if(oftp->refCount>0) return 0;//statement below is not printing. for some reason it thinks that its greater than 0. SOLVED
    //printf("REFCOUNT NOT > ZERO LIKE IT SHOULD BE\n");
    //last user of this OFT entry ==> dispose of the Minode[]
    mip = oftp->minodePtr;
    iput(mip); // here's when we finally put the minode back from when we opened the file!

    return 0;
}

int terminal_close_file(char *pathname){
    //if user wants to clse file from terminal using an fd as input we need this function bc terminal text is type char*
    if(!pathname){
        printf("close_file error: no file name given");
        return -1;
    }
    else{
        close_file(atoi(pathname));
        return 0;
    }
}

int mylseek(int fd, int position){
   
    OFT *oftp = running->fd[fd];

    if(oftp->refCount > 0)
    {
        if (position >= 0 && position <= oftp->minodePtr->INODE.i_size)
        {
            oftp->offset = position;
            return position;
        }
        else
        {
            printf("lseek error: position '%d' is out of bounds\n");
            return -1;
        }
    }

}

int pfd(){
    /*displays the currently opened files as follows
    fd      mode        offset      INODE
    --      ----        ------      -----
    0       READ        1234        [dev,ino]
    1       WRITE       0           [dev,ino]
    -----------------------------------------
    to help users know what files have been opened
    */
    printf("fd      mode        offset      INODE\n");
    printf("--      ----        ------      -----\n");
    for(int i=0; i < NOFT; i++){
        if(oft[i].refCount > 0){
            printf("%d    ", i);
            switch(oft[i].mode){
                case 0: 
                    printf("   READ");
                    break;
                case 1:
                    printf("   WRITE");
                    break;
                case 2:
                    printf("   READ/WRITE");
                    break;
                case 3:
                    printf("   APPEND");
                    break;
            }
            printf("         %d         [%d,%d]\n", oft[i].offset, oft[i].minodePtr->dev, oft[i].minodePtr->ino);
            //printf("refcount: %d\n", oft[i].refCount); //refcount for each file is just blank for some reason. SOLVED
        }
        
       
        
    }
     printf("-------------------------------------------\n");
    return 0;
}

dup(int fd){

    if (running->fd[fd] == NULL)
    {
        printf("dup error: fd is not an open descriptor\n");
    }
    
    OFT *oftp = running->fd[fd];
    for(int i = 0; i < NFD; i++)
    {
        if(running->fd[i] == NULL)
        {
            running->fd[i] = oftp;
            oftp->refCount++;
            return 0;
        }
    }

}

dup2(int fd, int gd){
    
    if (running->fd[gd] != NULL) // needs to be closed
    {
        close_file(gd);
    }
    
    OFT *oftp = running->fd[fd];
    running->fd[gd] = oftp;
    oftp->refCount++;
    return 0;

}
