// ialloc/balloc, idalloc/bdalloc functions
int tst_bit(char *buf, int bit); // Chapter 11.3.1
{
    return buf[bit/8] & (1 << (bit % 8));
}

int set_bit(char *buf, int bit); // Chapter 11.3.1
{
    buf[bit/8] |= (1 << (bit % 8));
}


//FUNCTIONS USED FOR MKDIR

int decFreeInodes(int dev) {
    //dec free inodes count in super and gd
    get_block(dev, 1, buf);
    sp = (SUPER *) buf;
    sp->s_free_inodes_count--;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_inodes_count--;
    put_block(dev, 2, buf);
}

int ialloc(int dev){ // allocate an inode number from inode_bitmap
    int i;
    char buf[BLKSIZE];

    //read inode_bitmap block
    get_block(dev, imap, buf);

    for(i=0; i< ninodes; i++){ //use ninodes from super block
        if(tst_bit(buf,i)==0){
            set_bit(buf, i);
            put_block(dev, imap, buf);

            decFreeInodes(dev);

            printf("allocated ino = %d\n", i+1); // bits count from 0; ino from 1
            return i+1;
        }
    }
    return 0;
}

//WRITE YOUR OWN BALLOC(DEV) FUNCTION, WHICH ALLOCATES A FREE DISK BLOCK NUMBER

//FINISH IPUT(MINODE *MIP) CODE IN UTIL.C



// FUNCTIONS USED FOR RMDIR
 
int clr_bit(char *buf, int bit){
    buf[bit/8] &= ~(1 << (bit % 8));
}

int incFreeInodes(int dev){
    char buf[BLKSIZE];

    // inc free inodes count in SUPER and GD
    get_block(dev, 1, buf);
    sp = (SUPER *) buf;
    sp->s_free_inodes_count++;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf;
    gp->bg_free_inodes_count++;
    put_block(dev, 2, buf);
}

int idalloc(int dev, int ino){
    int i; 
    char buf[BLKSIZE];

    if (ino > ninodes){
        printf("inumber %d out of range \n", ino);
        return -1;
    }

    // get inode bitmap block
    get_block(dev, imap, buf);
    clr_bit(buf, ino-1);

    // write buf back
    put_block(dev, imap, buf);

    //update free inodes count in SUPER and GD
    incFreeInodes(dev);
}

/* WRITE YOUR bdalloc() function which deallocates a block number 
Similar, except it uses the device's blocks bitmap and it increments 
the free blocks count in both super block and groupdescriptor */
// Assume: command line = "rmdir pathname"          ALg of rmdir: Chapter 11.8.4

