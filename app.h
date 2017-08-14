#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>  
#include <stdio.h>
#include <cstring>
#include <stdlib.h>
using namespace std;

namespace mysys{
	//相当于文件节点inode，用于储存文件属性 
	#ifndef FILENODE_H
	#define FILENODE_H
	class fileNode{
	public:
		int uid;
		int length;
		unsigned short mode;
		int size;
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
			dirNode dir_[10]; //目录项
			int curr;//现有文件数
			int next;//剩余的目录项inode,-1表示没有
			int parent;//父目录
			myDir(){
				curr = 0;
				next = -1;
				parent = -1;
			}
			~myDir(){
				;
			}
	};
	#endif

	//文件系统
	#ifndef MYFILESYS_H
	#define MYFILESYS_H
	class myFileSys{
		public:
			myFileSys(){
				//初始化
				currUser = -1;
				loadRoot();
				int fd = open("block", O_RDONLY);
				read(fd,(void *)(fblock),sizeof(fblock));
				lseek(fd,sizeof(fblock),SEEK_SET);
				read(fd,(void *)(iblock),sizeof(iblock));
				close(fd);
			}


			~myFileSys(){
				//模拟磁盘管理
				int fd = open("block",O_WRONLY);
				write(fd,(void *)(fblock),sizeof(fblock));
				lseek(fd,sizeof(fblock),SEEK_SET);
				write(fd,(void *)(iblock),sizeof(iblock));
				close(fd);
			}
			
		private:
			int currUser; //当前用户，-1表示没有登录
			fileNode* list[10]; //最大文件节点打开数：10
			myDir* curr;	//当前目录
			int currPos;	//当前目录物理地址
			myDir* root;	//根目录
			int rootPos;	//根目录物理地址
			bool fblock[40]; //模拟磁盘块
			bool iblock[100];	//模拟inode块
			//载入根目录
			void loadRoot(){
				//假如存在根目录
				if(access("file0",F_OK)!=-1){
					int fd = open("file0",O_RDONLY);
					myDir* a = new myDir();
					read(fd,a->name,sizeof(a->name));
					lseek(fd,sizeof(a->name),SEEK_CUR);
					read(fd,&(a->curr),sizeof(int));
					lseek(fd,sizeof(int),SEEK_CUR);
					read(fd,&(a->next),sizeof(int));
					lseek(fd,sizeof(int),SEEK_CUR);
					read(fd,&(a->parent),sizeof(int));
					lseek(fd,sizeof(int),SEEK_CUR);
					for(int i =0; i < a->curr; ++i){
						read(fd,&(a->dir_[i].inode),sizeof(int));
						lseek(fd,sizeof(int),SEEK_CUR);
						read(fd,(a->dir_[i].name),sizeof(a->dir_[i].name));
						lseek(fd,sizeof(int),SEEK_CUR);
						read(fd,&(a->dir_[i].type),sizeof(int));
						lseek(fd,sizeof(int),SEEK_CUR);
					}
					close(fd);
					root = a;
				}
				//初始化根目录
				else{
					int fd = creat("file0",O_WRONLY);
					myDir* a = new myDir();
					strcpy(a->name,"root");
					root = a;
					write(fd,a->name,sizeof(a->name));
					lseek(fd,sizeof(a->name),SEEK_CUR);
					write(fd,&(a->curr),sizeof(int));
					lseek(fd,sizeof(int),SEEK_CUR);
					write(fd,&(a->next),sizeof(int));
					lseek(fd,sizeof(int),SEEK_CUR);
					write(fd,&(a->parent),sizeof(int));
					close(fd);
				}
				curr = root;
				rootPos = 0;
				currPos = 0;
			}

			int freeFBlock(){
				for(int i= 0; i < 40;++i){
					if(fblock[i])
						return i;
				}
				return -1;
			}

			int freeIBlock(){
				for(int i= 0; i < 100;++i){
					if(fblock[i])
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
				int fd = open("user",O_RDONLY);
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
	
			int create_(char pathname[256],int mode){

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