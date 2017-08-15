#include"app.h"
#include<iostream>
using namespace std;
using namespace mysys;

void fomattedFileInfo(int inode){
    char i[8] = "inode";
    char q[3] ="";
    sprintf(q,"%d",inode);
    strcat(i,q);
    int fd = open(i,O_RDONLY);
    fileNode a;
    read(fd,&(a.uid),sizeof(int));
	lseek(fd,sizeof(int),SEEK_CUR);
	read(fd,&(a.length),sizeof(int));
	lseek(fd,sizeof(int),SEEK_CUR);
	read(fd,&(a.mode),sizeof(short));
	lseek(fd,sizeof(short),SEEK_CUR);
	read(fd,&(a.pos),sizeof(int));
	lseek(fd,sizeof(int),SEEK_CUR);//文件指针移动到文件末尾
	close(fd);
    printf(" ");
    printf("file%d  ",a.pos);
    char w[11];
    w[10] = '\0';
    a.fomattedMod(w);
    printf("%s",w);
    printf("  %d\n",a.length);
}


int main(){
    int fd_ = open("file0",O_RDONLY);
    myDir* b = new myDir();
    read(fd_,b->name,sizeof(b->name));
	lseek(fd_,sizeof(b->name),SEEK_CUR);
	read(fd_,&(b->curr),sizeof(int));
	lseek(fd_,sizeof(int),SEEK_CUR);
    read(fd_,&(b->next),sizeof(int));
	lseek(fd_,sizeof(int),SEEK_CUR);
	read(fd_,&(b->parent),sizeof(int));
	lseek(fd_,sizeof(int),SEEK_CUR);
	//读取目录项
	for(int i=0; i < b->curr; i++){
			b->dir_[i] = new dirNode();
			read(fd_,&(b->dir_[i]->inode),sizeof(int));
			lseek(fd_,sizeof(int),SEEK_CUR);
			read(fd_,(b->dir_[i]->name),sizeof(b->dir_[i]->name));
			lseek(fd_,sizeof(int),SEEK_CUR);
			read(fd_,&(b->dir_[i]->type),sizeof(int));
			lseek(fd_,sizeof(int),SEEK_CUR);
	}	
    for(int i = 0; i < 10; i++){
        if(b->dir_[i] != NULL){
            printf("%s",b->dir_[i]->name);
            fomattedFileInfo(b->dir_[i]->inode);
        }
    }

    close(fd_);
    cin.get();
}