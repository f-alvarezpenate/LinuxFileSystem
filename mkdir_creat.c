//#include "type.h"
// mkdir, creat, enter_name
int mymkdir(char *pathname)
{
    MINODE *start;
    
    char temp1[256], temp2[256];
    strcpy(temp1, pathname);
    strcpy(temp2, pathname);

    //sprintf(c, "%c", name[0]);
    
    if(pathname[0] == '/') // root
    {
        start = root;
        dev = root->dev;
        
    }
    else // not root
    {
        start = running->cwd;
        dev = running->cwd->dev;
        
    }
    //seg fault here
    char *parent = dirname(temp1);
    char *child = basename(temp2);

    
    printf("parent=%s child =%s\n", parent, child);
    
    // getting minode of parent
    int pino = getino(parent);
    
    MINODE *pip = iget(dev, pino);
    
    // verifying that parent INODE is a DIR and child does not 
    // already exist in the parent directory
    if(S_ISDIR(pip->INODE.i_mode) && search(pip, child) == 0)
    {
        kmkdir(pip, child); 

        pip->INODE.i_links_count++; // incrementing parent inodes link count

        pip->INODE.i_mtime = time(0L);

        pip->dirty = 1; // marking as dirty since a change has been made

        iput(pip);
    }
    
}

int kmkdir(MINODE* pip, char* name)
{
    char buf[BLKSIZE];

    MINODE *mip;
    int ino = ialloc(dev);
    int bno = balloc(dev);
    printf("ino= %d, bno= %d\n", ino, bno);

    mip = iget(dev, ino);
    INODE *ip = &mip->INODE;
    ip->i_mode = 040755; //040755: DIR type and permissions
    ip->i_uid = running->uid; // owner uid
    ip->i_gid = running->gid; // group id
    ip->i_size = BLKSIZE; // size in bytes
    ip->i_links_count = 2; // links count = 2 because of . and ..
    ip->i_atime = time(0L);
    ip->i_ctime = time(0L);
    ip->i_mtime = time(0L);
    ip->i_blocks = 2; // Linux: blocks count in 512 byte chunks
    ip->i_block[0] = bno; // new DIR has one data block
    for (int i = 1; i <= 14; i++)
    {
        ip->i_block[i] = 0;
    }
    
    mip->dirty = 1; // mark minode as dirty
    iput(mip); // write INODE to disk

    bzero(buf, BLKSIZE); //clears the buf to 0
    DIR *dp = (DIR *)buf;
    // make . entry
    dp->inode = ino;
    dp->rec_len = 12;
    dp->name_len = 1;
    // setting name to "."
    dp->name[0] = '.';
    //make .. entry: pino=parent DIR ino, blk=allocated block
    dp = (char *)dp + 12;
    dp->inode = pip->ino;
    dp->rec_len = BLKSIZE-12; //rec_len spans block
    dp->name_len = 2;
    //set name to ".."
    dp->name[0] = '.';
    dp->name[1] = '.';
    put_block(dev, bno, buf); //write to blk on disk
    
    enter_name(pip, ino, name);

}

int enter_name(MINODE *pip, int myino, char *myname)
{
    INODE *ip = &pip->INODE;
    char buf[BLKSIZE], *cp;
    int ideal_length, need_length, remain;

    for(int i = 0; i < 12; i++)
    {
        if(ip->i_block[i] == 0) // no space existing in data block
        {
            int new_block = balloc(pip->dev);
            ip->i_blocks++;
            ip->i_block[i] = new_block;
            ip->i_size += BLKSIZE;

            get_block(pip->dev, pip->INODE.i_block[i], buf);

            dp = (DIR *)buf;

            dp->inode = myino;
            dp->rec_len = BLKSIZE;
            dp->name_len = strlen(myname);
            strcpy(dp->name, myname);

            put_block(pip->dev, pip->INODE.i_block[i], buf);
            break;
        }
        else
        {
            get_block(pip->dev, pip->INODE.i_block[i], buf); //getting parents data block into buf

            dp = (DIR *) buf;

            cp = buf;

            while(cp + dp->rec_len < buf + BLKSIZE)
            {
                cp += dp->rec_len;
                dp = (DIR *) cp;
            }
            // after loop, dp will point at last entry in the block
            ideal_length = 4 * ((8 + dp->name_len + 3)/4);
            remain = dp->rec_len - ideal_length;
            need_length = 4 * ((8 + strlen(myname) + 3) / 4);

            if(remain >= need_length) //enter the new entry as the LAST entry and trim the previous entry rec len to its ideal length
            {
                dp->rec_len = ideal_length;

                cp += dp->rec_len;
                dp = (DIR *)cp;

                dp->inode = myino;
                dp->rec_len = remain;
                dp->name_len = strlen(myname);
                strcpy(dp->name, myname);

                put_block(pip->dev, pip->INODE.i_block[i], buf);
            }
        }
    }


}
int mycreat()
{

}

int kcreat(MINODE* pip, char * name)
{

}