#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>  
#include <stdio.h>
#include <cstring>
#include <string.h>
#include <stdlib.h>
#include <iostream>
using namespace std;

namespace mysys{
	//相当于文件节点inode，用于储存文件属性 
	#ifndef FILENODE_H
	#define FILENODE_H
	class fileNode{
	public:
		int uid; //0表示root用户
		int length;
		unsigned short mode;
		int pos; //文件位置
		fileNode(){
			mode = 0;
		}
		~fileNode(){
			;
		}
		void fomattedMod(char mod[10]){
			mod[0] = (((mode>>9)&1)?'d':'-');
			mod[1] = ((mode & 0x100)?'r':'-');
			mod[2] = ((mode & 0x080)?'w':'-');
			mod[3] = ((mode & 0x040)?'x':'-');
			mod[4] = ((mode & 0x020)?'r':'-');
			mod[5] = ((mode & 0x010)?'w':'-');
			mod[6] = ((mode & 0x008)?'x':'-');
			mod[7] = ((mode & 0x004)?'r':'-');
			mod[8] = ((mode & 0x002)?'w':'-');
			mod[9] = ((mode & 0x001)?'x':'-');    
		}		
	};
	#endif

	//子目录节点，用于储存文件和子目录信息
	#ifndef DIRNODE_H
	#define DIRNODE_H
	class dirNode{
		public:
			int inode; //文件节点号
			char name[256]; //文件名
			int type;//文件类型，1=目录，2=文件
			dirNode(){
				;
			}
			~dirNode(){
				;
			}
	};
	#endif 

	//主目录，用于存储子目录或者文件的信息
	#ifndef MYDIR_H
	#define MYDIR_H
	class myDir{
		public:
			char name[256]; //目录名
			dirNode* dir_[10]; //目录项
			int curr;//现有文件数
			int next;//剩余的目录项inode,-1表示没有
			int parent;//父目录
			myDir(){
				curr = 0;
				next = -1;
				parent = -1;
				for(int i = 0; i < 10; ++i)
					dir_[i] = NULL;
			}
			~myDir(){
				for(int i = 0; i < 10; ++i){
					delete dir_[i];
					dir_[i] = NULL;
				}
			}

			int freeEntity(){
				if(curr == 10)
					return -1; 
				for(int i = 0; i < 10; ++i)
					if(dir_[i] == NULL)
						return i;
			}
	};
	#endif

	//文件系统
	#ifndef MYFILESYS_H
	#define MYFILESYS_H
	class myFileSys{
		private:
			int currUser; //当前用户，-1表示没有登录
			fileNode* list[10]; //最大文件节点打开数：10
			myDir* curr;	//当前目录
			fileNode* currINode;
			bool fblock[40]; //模拟磁盘块
			bool iblock[100];	//模拟inode块

			void clearblock(){
				for(int i = 0; i < 40; ++i){
					fblock[i] = false;
				}
				for(int i = 0; i < 100; ++i){
					iblock[i] = false;
				}
			}

			void loadRoot(){	
				fileNode* a = new fileNode();
				myDir* b = new myDir();

				if(access("inode0",F_OK) != -1){
					printf("\nload root\n");
					//只读形式打开根目录inode
					int fd = open("inode0",O_RDONLY);
					read(fd,&(a->uid),sizeof(int));
					lseek(fd,sizeof(int),SEEK_CUR);
					read(fd,&(a->length),sizeof(int));
					lseek(fd,sizeof(int),SEEK_CUR);
					read(fd,&(a->mode),sizeof(short));
					lseek(fd,sizeof(short),SEEK_CUR);
					read(fd,&(a->pos),sizeof(int));
					lseek(fd,sizeof(int),SEEK_CUR);//文件指针移动到文件末尾
					close(fd);
					//读取根目录
					int fd_ = open("file0",O_RDONLY);
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
					close(fd_);
				}	
				else{
					printf("\ninit root\n");		
					//初始化根目录inode
					int fd = creat("inode0",S_IRWXU);
					int size = sizeof(int)* 3 + sizeof(b->name);
					printf("/n%d/n",size);
					a->uid = 0;
					a->length = size;
					a->pos = 0;
					a->mode = 0x384;
					write(fd,&(a->uid),sizeof(int));
					lseek(fd,sizeof(int),SEEK_CUR);
					write(fd,&(a->length),sizeof(int));
					lseek(fd,sizeof(int),SEEK_CUR);
					write(fd,&(a->mode),sizeof(short));
					lseek(fd,sizeof(short),SEEK_CUR);
					write(fd,&(a->pos),sizeof(int));
					lseek(fd,sizeof(int),SEEK_CUR);
					close(fd);
					//初始化根目录文件
					int fd_ = creat("file0",S_IRWXU);
					strcpy(b->name,"root");
					write(fd_,b->name,sizeof(b->name));
					lseek(fd_,sizeof(b->name),SEEK_CUR);
					write(fd_,&(b->curr),sizeof(int));
					lseek(fd_,sizeof(int),SEEK_CUR);
					write(fd_,&(b->next),sizeof(int));
					lseek(fd_,sizeof(int),SEEK_CUR);
					write(fd_,&(b->parent),sizeof(int));
					close(fd_);
				}
				this->curr = b;
				this->currINode = a;
			}

			void loadBlock(){
				if( access("block",F_OK)!=-1){
					printf("\nload block\n");
					//读取磁盘情况
					int fd = open("block", O_RDONLY);
					read(fd,(void *)(fblock),sizeof(fblock));
					lseek(fd,sizeof(fblock),SEEK_SET);
					read(fd,(void *)(iblock),sizeof(iblock));
					close(fd);
				}
				else{
					printf("\ninit block\n");
					fblock[0] = iblock[0] = true;
					int fd = creat("block",S_IRWXU);
					write(fd,(void *)(fblock),sizeof(fblock));
					lseek(fd,sizeof(fblock),SEEK_SET);
					write(fd,(void *)(iblock),sizeof(iblock));
					close(fd);
				}
			}

			void writeBlock(){
				//模拟磁盘管理
				int fd = open("block",O_WRONLY);
				write(fd,(void *)(fblock),sizeof(fblock));
				lseek(fd,sizeof(fblock),SEEK_SET);
				write(fd,(void *)(iblock),sizeof(iblock));
				close(fd);
			}

			void writeDir(){
				int p = currINode->pos;
				char a[6] = "file";
				char b[2] ="";
				sprintf(b,"%d",p);
				strcat(a,b);
				int fd = open(a,O_WRONLY);
				write(fd,curr->name,sizeof(curr->name));
				lseek(fd,sizeof(curr->name),SEEK_CUR);
				write(fd,&(curr->curr),sizeof(int));
				lseek(fd,sizeof(int),SEEK_CUR);
				write(fd,&(curr->next),sizeof(int));
				lseek(fd,sizeof(int),SEEK_CUR);
				write(fd,&(curr->parent),sizeof(int));
				lseek(fd,sizeof(int),SEEK_CUR);
				for(int i=0; i < 10; ++i){
					if(curr->dir_[i] != NULL){
						write(fd,&(curr->dir_[i]->inode),sizeof(int));
						lseek(fd,sizeof(int),SEEK_CUR);
						write(fd,(curr->dir_[i]->name),sizeof(curr->dir_[i]->name));
						lseek(fd,sizeof(int),SEEK_CUR);
						write(fd,&(curr->dir_[i]->type),sizeof(int));
						lseek(fd,sizeof(int),SEEK_CUR);
					}
				}
				close(fd);
			}

			int freeFBlock(){
				for(int i= 0; i < 40;++i){
					if(!fblock[i])
						return i;
				}
				return -1;
			}

			int freeIBlock(){
				for(int i= 0; i < 100;++i){
					if(!fblock[i])
						return i;
				}
				return -1;
			}

			int freeFd(){
				for(int i =0; i < 10; ++i){
					if(list[i]!=NULL)
						return i;
				}
				return -1;
			}

			int login(char un[256],char passwd[256]){
				int fd = open("user",S_IRWXU);
				char na[256];
				char pa[256];
				if(read(fd,na,sizeof(na)) != -1){
					int i = strcmp(un,na);
					if(i != 0){
						close(fd);
						return 0;
					}
					else{
						lseek(fd,sizeof(na),SEEK_CUR);
						read(fd,pa,sizeof(pa));
						i = strcmp(passwd,pa);
						if(i == 0){
							close(fd);
							return 1;
						}
						else{
							close(fd);
							return 0;
						}
					}
				}
				else{
					close(fd);
					return -1;
				}
			}

			int signup(char na[256],char pa[256]){
				int fd = open("user",O_WRONLY);
				write(fd,na,sizeof(na));
				lseek(fd,sizeof(na),SEEK_CUR);
				write(fd,pa,sizeof(pa));
				close(fd);
			}

			bool empty(){
				if(access("user",F_OK)!=-1)
					return false;
				else
					return true;
			}

		public:
			myFileSys(){
				//初始化
				currUser = -1;
				clearblock();
				loadRoot();
				loadBlock();
			}

			~myFileSys(){
				writeBlock();
			}
	
			int create_(const char* pathname,int mode){
				printf("\n%s\n","what?");
				bool err = false;
				int freefd = freeFd();
				//文件描述符已达上限
				if(freefd == -1){
					printf("\n%d\n",freefd);
					return -1;
				}
				//没有足够空间可以分配给inode
				int freeinode = freeIBlock();
				if(freeinode == -1){
					printf("\n%d\n",freeinode);
					return -1;
				}
				//磁盘空间不足
				int freeblock = freeFBlock(); 
				if(freeblock == -1){
					printf("\n%d\n",freeblock);
					return -1;
				}
				fileNode* a = new fileNode();
				dirNode* b = new dirNode();
				a->pos = freeblock;
				a->uid = currUser;
				a->length = 0;
				strcpy(b->name,pathname);
				b->type = 2;
				b->inode = freeinode;
				switch(mode){
					case 6:
						a->mode = 0x184;
						break;
					case 4:
						a->mode = 0x104;
						break;
					case 2:
						a->mode = 0x084;
						break;
					default:
						delete a;
						delete b;
						printf("未知权限");
						return -1;
				}
				//目录项剩余
				int e = 0;
				if( (e = curr->freeEntity()) != -1){
					printf("\n%s\n","left entity");
					curr->dir_[e] = b;
					char i[9] = "inode";
					char h[3];
					sprintf(h,"%d",freeinode);
					strcat(i,h);
					printf("\n%s\n",i);
					int fd = creat(i,S_IRWXU);
					if(fd == -1)
						err = true;
					write(fd,&(a->uid),sizeof(int));
					lseek(fd,sizeof(int),SEEK_CUR);
					write(fd,&(a->length),sizeof(int));
					lseek(fd,sizeof(int),SEEK_CUR);
					write(fd,&(a->mode),sizeof(short));
					lseek(fd,sizeof(short),SEEK_CUR);
					write(fd,&(a->pos),sizeof(int));
					lseek(fd,sizeof(int),SEEK_CUR);
					close(fd);

					char f[8] = "file";
					char g[2];
					sprintf(g,"%d",freeblock);
					strcat(f,g);
					printf("\n%s\n",f);
					int fd_ = creat(f,S_IRWXU);
					if(fd_ == -1){
						printf("\n%s\n","create fail");
						return -1;
					}
					fblock[freeblock] = true;
					iblock[freeinode] = true;
					this->list[freefd] = a;
					curr->curr++;
					writeDir();
					writeBlock();
					return freefd;
				}
				//目录项空间不足，现不考虑在next拼接
				else{
					printf("\n%d\n",e);
					delete a;
					delete b;
					return -1;
				}
			}

			int write_(int fd,char* buff,int length){

			}

			int read_(int fd,char* buff,int length){

			}

			int delete_(char pathnamep[256]){

			}

			int open_(char pathnamep[256],int mod){

			}
			
			int close_(int fd){

			}

			int mkdir_(char pathname[256]){

			}

			int cd_(char pathname[256]){

			}
	};
	#endif
};