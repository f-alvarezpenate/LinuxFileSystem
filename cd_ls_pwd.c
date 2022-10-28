/************* cd_ls_pwd.c file **************/
int cd()
{
  //printf("cd: under construction READ textbook!!!!\n");

  // READ Chapter 11.7.3 HOW TO chdir
  int ino = getino(pathname); // return error if ino = 0;
  if (ino == 0){
    printf("Error: ino = 0\n");
    return;
  }
  MINODE *mip = iget(dev, ino); 
  //verify mip->INODE is a DIR  MAYBE: if(S_ISDIR(mip->INODE->st_mode))
  
  iput(running->cwd); // release old cwd
  running->cwd = mip; // change cwd to mip
}

char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";

int ls_file(MINODE *mip, char *name)
{ 
  int i;
  char ftime[64];
  
  //printf("ls_file: to be done: READ textbook!!!!\n");
  // READ Chapter 11.7.3 HOW TO ls
  
  INODE *ip = &mip->INODE;
  //use ip-> to access INODE fields for ls -l SAME as using STAT
  //for each file, you shuold also print [dev, ino] of the file
  if ((ip->i_mode & 0xF000) == 0x8000) // if (S_ISREG())
    printf("%c",'-');
  if ((ip->i_mode & 0xF000) == 0x4000) // if (S_ISDIR())
    printf("%c",'d');
  
  for (i = 8; i >= 0; i--) {
    if (ip->i_mode & (1 << i)) // print r|w|x
      printf("%c",t1[i]);
    else 
      printf("%c", t2[i]);  // or print '-'
  }

  printf("%4d ", ip->i_gid); // gid
  printf("%4d ", ip->i_uid); // uid
  printf("%8d ", ip->i_size); // file size
  
  // print time 
  
  strcpy(ftime, ctime(&ip->i_mtime)); // print time in calendar form
  ftime[strlen(ftime)-1] = 0; // kill \n at end
  printf("%s ", ftime); 
  
  // print name
  printf("%s", basename(name)); // print file basename
  
  
   printf("\n");
}

int ls_dir(MINODE *mip)
{
  //printf("ls_dir: list CWD's file names; YOU FINISH IT as ls -l\n");
  int i; //added from pictures
  char buf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;
  MINODE *dip; //added from pictures
  
  printf("ls_dir "); //added from pictures
  
  printf("i_block[0] = %d\n", i, mip->INODE.i_block[0]); //added from pictures
  
  get_block(dev, mip->INODE.i_block[0], buf);
  dp = (DIR *)buf;
  cp = buf;

  while (cp < buf + BLKSIZE){
     strncpy(temp, dp->name, dp->name_len);
     temp[dp->name_len] = 0;
	
     //printf("%s\n",temp); // comment out
     dip = iget(dev, dp->inode); // iget()
     
     ls_file(dip, temp); // call ls_file()
     
     iput(dip); // iput()
     

     cp += dp->rec_len;
     dp = (DIR *)cp;
  }
  //printf("\n"); // comment out
}

int ls()
{
  //printf("ls: list CWD only! YOU FINISH IT for ls pathname\n");
  printf("PATHNAME: %s", pathname);
  if(strcmp(pathname,"")){
	  int ino = getino(pathname);
	  //ls_dir(running->cwd);
	  MINODE *mip = iget(dev, ino); 
	  ls_dir(mip);
  }
  else
  	ls_dir(running->cwd);
}

char *pwd(MINODE *wd)
{
  //printf("pwd: READ HOW TO pwd in textbook!!!!\n");
  if (wd == root){
    printf("/\n");
    return;
  }
  else{
  	rpwd(wd);
  }
  pwd(running->cwd);
}

void rpwd(MINODE *wd){  //DOESN'T WORK
  if (wd==root) return;
   //from wd->INODE.i_block[0], get my_ino and parent_ino
  int *parent_ino;
  int ino = findino(wd->INODE.i_block[0], parent_ino);
  MINODE *pip = iget(dev, parent_ino);
  //from pip->INODE.i_block[]: get my_name string by my_ino as LOCAL 
  char* my_name;
  findmyname(pip, ino, my_name);
  pwd(pip);
  printf("/%s", my_name);
}



