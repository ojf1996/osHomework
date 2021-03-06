#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>  
#include <stdio.h>
#include <cstring>
#include <errno.h> 
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
			//剩余目录项
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
			int back_[10];
			myDir* curr;	//当前目录
			fileNode* currINode; //当前目录节点
			int currINodeNo;
			bool fblock[40]; //模拟磁盘块
			bool iblock[100];	//模拟inode块
			//清空当前块信息，以便初始化
			void clearblock(){
				for(int i = 0; i < 40; ++i){
					fblock[i] = false;
				}
				for(int i = 0; i < 100; ++i){
					iblock[i] = false;
				}
				for(int i=0; i < 10; ++i){
					list[i] = NULL;
					back_[i] = -1;
				}

			}
			//读入根目录信息
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
				currINodeNo = 0;
			}
			//读入磁盘使用情况
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
			//将新磁盘使用信息写回磁盘
			void writeBlock(){
				//模拟磁盘管理
				int fd = open("block",O_WRONLY);
				write(fd,(void *)(fblock),sizeof(fblock));
				lseek(fd,sizeof(fblock),SEEK_SET);
				write(fd,(void *)(iblock),sizeof(iblock));
				close(fd);
			}
			//将目录信息写回磁盘
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
			//空余磁盘
			int freeFBlock(){
				for(int i= 0; i < 40;++i){
					if(!fblock[i])
						return i;
				}
				return -1;
			}
			//空余inode区域
			int freeIBlock(){
				for(int i= 0; i < 100;++i){
					if(!fblock[i])
						return i;
				}
				return -1;
			}
			//空余描述符
			int freeFd(){
				for(int i =0; i < 10; ++i){
					if(list[i]==NULL)
						return i;
				}
				return -1;
			}
			//是否有人注册
			bool empty(){
				if(access("user",F_OK)!=-1)
					return false;
				else
					return true;
			}

			//格式化文件信息
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
				printf(" %d",a.uid);
				printf("  %d",a.length);
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
				for(int i = 0; i < 10; ++i){
					if(list[i]!= NULL){
						delete list[i];
					}
				}
				delete currINode;
				delete curr;
			}
			
			int create_(const char* pathname,int mode){
				int freefd = freeFd();
				//文件描述符已达上限
				if(freefd == -1){
					printf("\nfd %d\n",freefd);
					return -1;
				}
				//没有足够空间可以分配给inode
				int freeinode = freeIBlock();
				if(freeinode == -1){
					printf("\ninode %d\n",freeinode);
					return -1;
				}
				//磁盘空间不足
				int freeblock = freeFBlock(); 
				if(freeblock == -1){
					printf("\nblock %d\n",freeblock);
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
				if(currUser != -1){
					switch(mode){
						case 6:
							a->mode = 0x180;
							break;
						case 4:
							a->mode = 0x100;
							break;
						case 2:
							a->mode = 0x080;
							break;
						case 0:
							break;
						default:
							delete a;
							delete b;
							printf("未知权限");
							return -1;
					}
				}
				else{
					switch(mode){
						case 6:
							a->mode = 0x006;
							break;
						case 4:
							a->mode = 0x004;
							break;
						case 2:
							a->mode = 0x002;
							break;
						case 0:
							break;
						default:
							delete a;
							delete b;
							printf("未知权限");
							return -1;
					}
				}
				//目录项剩余
				int e = 0;
				if( (e = curr->freeEntity()) != -1){
					curr->dir_[e] = b;
					char i[9] = "inode";
					char h[3];
					sprintf(h,"%d",freeinode);
					strcat(i,h);
					int fd = creat(i,S_IRWXU);
					if(fd == -1){
						printf("\n%s\n","create fail");
						return -1;
					}
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
					int fd_ = creat(f,S_IRWXU);
					if(fd_ == -1){
						printf("\n%s\n","create fail");
						return -1;
					}
					close(fd_);
					fblock[freeblock] = true;
					iblock[freeinode] = true;
					this->list[freefd] = a;
					this->back_[freefd] = freeinode;
					curr->curr++;
					writeDir();
					writeBlock();
					return freefd;
				}
				//目录项空间不足，现不考虑在next拼接
				else{
					printf("\n%d next d\n",e);
					delete a;
					delete b;
					return -1;
				}
			}

			void dir_(){
				for(int i = 0; i < 10; i++){
					if(curr->dir_[i] != NULL){
						printf("\n%s",curr->dir_[i]->name);
						fomattedFileInfo(curr->dir_[i]->inode);
					}
				}
			}

			int write_(int fd,const char* buff,int offset,int length){
				if( list[fd]!=NULL){
					if(currUser != -1){
						if( (currUser == list[fd]->uid ) && (list[fd]->mode)&0x080){
							char c[9] = "file";
							char d[3] = "";
							sprintf(d,"%d",list[fd]->pos);
							strcat(c,d);
							int fd_ = open(c,O_WRONLY);
							int i = -1;
							lseek(fd_,offset,SEEK_SET);
							i = write(fd_,buff,length); 
							close(fd_);
							struct stat buf;  
							stat(c, &buf);  
							list[fd]->length = buf.st_size;
							return i;
						}
					}
					else if((list[fd]->mode)&0x002){
						char c[9] = "file";
						char d[3] = "";
						sprintf(d,"%d",list[fd]->pos);
						strcat(c,d);
						int fd_ = open(c,O_WRONLY);
						int i = -1;
						lseek(fd_,offset,SEEK_SET);
						i = write(fd_,buff,length); 
						close(fd_);
						struct stat buf;  
						stat(c, &buf);  
						list[fd]->length = buf.st_size;
						return i;
					}
					else{
						printf("\nnot enough right\n");
						return -1;
					}
				}
				else{
					printf("\nwrong fd\n");
					return -1;
				}
			}
			int read_(int fd,char* buff,int offset,int length){
				if( list[fd]!=NULL){
					if(currUser != -1){
						if( (currUser == list[fd]->uid ) && (list[fd]->mode)&0x080){
							char c[9] = "file";
							char d[3] = "";
							sprintf(d,"%d",list[fd]->pos);
							strcat(c,d);
							int fd_ = open(c,O_RDONLY);
							int i = -1;
							lseek(fd_,offset,SEEK_SET);
							i = read(fd_,buff,length); 
							close(fd_);
							return i;
						}
					}
					else if((list[fd]->mode)&0x002){
						char c[9] = "file";
						char d[3] = "";
						sprintf(d,"%d",list[fd]->pos);
						strcat(c,d);
						int fd_ = open(c,O_RDONLY);
						int i = -1;
						lseek(fd_,offset,SEEK_SET);
						i = read(fd_,buff,length); 
						close(fd_);
						return i;
					}
					else{
						printf("\nnot enough right\n");
						return -1;
					}
				}
				else{
					printf("\nwrong fd\n");
					return -1;
				}
			}	

			int delete_(const char* pathname){
				for(int i = 0; i < 10; ++i){
					if(curr->dir_[i] != NULL){
						//匹配
						if(strncmp(pathname,curr->dir_[i]->name,sizeof(pathname))==0){
							if(curr->dir_[i]->type == 2){
								fileNode* a = new fileNode();
								char c[7] = "inode";
								char d[3] = "";
								sprintf(d,"%d",curr->dir_[i]->inode);
								strcat(c,d);	
								//读取inode
								int fd = open(c,O_RDONLY);
								read(fd,&(a->uid),sizeof(int));
								lseek(fd,sizeof(int),SEEK_CUR);
								read(fd,&(a->length),sizeof(int));
								lseek(fd,sizeof(int),SEEK_CUR);
								read(fd,&(a->mode),sizeof(short));
								lseek(fd,sizeof(short),SEEK_CUR);
								read(fd,&(a->pos),sizeof(int));
								lseek(fd,sizeof(int),SEEK_CUR);
								close(fd);
								char g[9]= "file";
								char h[3]="";
								sprintf(h,"%d",a->pos);
								strcat(g,h);
								//删除文件
								unlink(c);
								unlink(g);
								//修改目录信息
								curr->curr--;
								iblock[curr->dir_[i]->inode] = false;
								fblock[a->pos] = false;
								delete curr->dir_[i];
								curr->dir_[i] = NULL;
								writeDir();
								writeBlock();
								return 1;
							}
							else{
								printf("nworng type: a file is expected\n");
								return -1;
							}
						}
					}
				}
				printf("\nno such file\n");
				return -1;
			}

			int open_(const char* pathname){
				for(int i = 0; i < 10; ++i){
					if(curr->dir_[i] != NULL){
						//匹配
						if(strncmp(pathname,curr->dir_[i]->name,sizeof(pathname))==0){
							if(curr->dir_[i]->type == 2){
								int freefd = freeFd();
								//文件描述符已达上限
								if(freefd == -1){
									printf("\nfd %d\n",freefd);
									return -1;
								}
								//读取inode
								fileNode* a = new fileNode();
								char c[7] = "inode";
								char d[3] = "";
								sprintf(d,"%d",curr->dir_[i]->inode);
								strcat(c,d);	
								int fd = open(c,O_RDONLY);
								read(fd,&(a->uid),sizeof(int));
								lseek(fd,sizeof(int),SEEK_CUR);
								read(fd,&(a->length),sizeof(int));
								lseek(fd,sizeof(int),SEEK_CUR);
								read(fd,&(a->mode),sizeof(short));
								lseek(fd,sizeof(short),SEEK_CUR);
								read(fd,&(a->pos),sizeof(int));
								lseek(fd,sizeof(int),SEEK_CUR);
								close(fd);
								//加入列表
								list[freefd] = a;
								back_[freefd] = curr->dir_[i]->inode;
								return freefd;
							}
							else{
								printf("nworng type: a file is expected\n");
								return -1;
							}
						}
					}
				}
				printf("\nno such file\n");
				return -1;
			}
			
			int close_(int fd){
				if(list[fd] != NULL){
					int p = back_[fd];
					char c[8] = "inode";
					char d[3] = "";
					sprintf(d,"%d",p);
					strcat(c,d);	
					int fd1 = open(c,O_WRONLY);
					write(fd1,&(list[fd]->uid),sizeof(int));
					lseek(fd1,sizeof(int),SEEK_CUR);
					write(fd1,&(list[fd]->length),sizeof(int));
					lseek(fd1,sizeof(int),SEEK_CUR);
					write(fd1,&(list[fd]->mode),sizeof(short));
					lseek(fd1,sizeof(short),SEEK_CUR);
					write(fd1,&(list[fd]->pos),sizeof(int));
					lseek(fd1,sizeof(int),SEEK_CUR);
					close(fd1);
					delete list[fd];
					list[fd] = NULL;
					back_[fd] = -1;
				}
				else{
					printf("\ninvaild fd\n");
					return -1;
				}
			}

			int mkdir_(const char* pathname){
				int freeinode = freeIBlock();
				if(freeinode == -1){
					printf("\ninode %d\n",freeinode);
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
				b->type = 1;
				b->inode = freeinode;
				a->mode = 0x380;
				int e = 0;
				if( (e = curr->freeEntity()) != -1){
					curr->dir_[e] = b;
					char i[9] = "inode";
					char h[3];
					sprintf(h,"%d",freeinode);
					strcat(i,h);
					int fd = creat(i,S_IRWXU);
					if(fd == -1){
						printf("\n%s\n","create fail");
						return -1;
					}
					a->length =sizeof(int)* 3 + sizeof(b->name);
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
					int fd_ = creat(f,S_IRWXU);
					if(fd_ == -1){
						printf("\n%s\n","create fail");
						return -1;
					}
					//写入磁盘信息
					myDir* c = new myDir();
					c->parent = currINode->pos;
					strcpy(c->name,pathname);
					write(fd_,c->name,sizeof(c->name));
					lseek(fd_,sizeof(c->name),SEEK_CUR);
					write(fd_,&(c->curr),sizeof(int));
					lseek(fd_,sizeof(int),SEEK_CUR);
					write(fd_,&(c->next),sizeof(int));
					lseek(fd_,sizeof(int),SEEK_CUR);
					write(fd_,&(c->parent),sizeof(int));
					lseek(fd_,sizeof(int),SEEK_CUR);
					close(fd_);
					delete c;
					fblock[freeblock] = true;
					iblock[freeinode] = true;
					curr->curr++;
					writeDir();
					writeBlock();
					return 1;
				}
				//目录项空间不足，现不考虑在next拼接
				else{
					printf("\nentity %d\n",e);
					delete a;
					delete b;
					return -1;
				}
			}

			int cd_(const char* pathname){
				//迭代比较
				for(int i = 0; i < 10; ++i){
					if(curr->dir_[i] != NULL){
						//匹配
						if(strcmp(pathname,curr->dir_[i]->name)==0){
							//并且是目录
							if(curr->dir_[i]->type == 1){
								myDir* a = new myDir();
								fileNode* b = new fileNode();
								int p = curr->dir_[i]->inode;	
								char c[7] = "inode";
								char d[3] = "";
								sprintf(d,"%d",p);
								strcat(c,d);	
								int fd = open(c,O_RDONLY);
								read(fd,&(b->uid),sizeof(int));
								lseek(fd,sizeof(int),SEEK_CUR);
								read(fd,&(b->length),sizeof(int));
								lseek(fd,sizeof(int),SEEK_CUR);
								read(fd,&(b->mode),sizeof(short));
								lseek(fd,sizeof(short),SEEK_CUR);
								read(fd,&(b->pos),sizeof(int));
								lseek(fd,sizeof(int),SEEK_CUR);
								close(fd);
							

								char e[8] = "file";
								char f[2] = "";
								sprintf(f,"%d",b->pos);
								strcat(e,f);
								int fd_ = open(e,O_RDONLY);
								read(fd_,a->name,sizeof(a->name));
								lseek(fd_,sizeof(a->name),SEEK_CUR);
								read(fd_,&(a->curr),sizeof(int));
								lseek(fd_,sizeof(int),SEEK_CUR);
								read(fd_,&(a->next),sizeof(int));
								lseek(fd_,sizeof(int),SEEK_CUR);
								read(fd_,&(a->parent),sizeof(int));
								lseek(fd_,sizeof(int),SEEK_CUR);
								//读取目录项
								for(int i=0; i < a->curr; i++){
									a->dir_[i] = new dirNode();
									read(fd_,&(a->dir_[i]->inode),sizeof(int));
									lseek(fd_,sizeof(int),SEEK_CUR);
									read(fd_,(a->dir_[i]->name),sizeof(a->dir_[i]->name));
									lseek(fd_,sizeof(int),SEEK_CUR);
									read(fd_,&(a->dir_[i]->type),sizeof(int));
									lseek(fd_,sizeof(int),SEEK_CUR);
								}
								close(fd_);
								delete curr;
								delete currINode;
								curr = a;
								currINode  = b;
								currINodeNo = p;
								
								return 1;
							}
							else{
								printf("\nworng type");
								return -1;
							}
						}
					}
				}
				return -1;
			}

			int cd_last(){
				if(curr->parent != -1){
					myDir* a = new myDir();
					fileNode* b = new fileNode();
					int p = curr->parent;	
					char c[7] = "inode";
					char d[3] = "";
					sprintf(d,"%d",p);
					strcat(c,d);	
					int fd = open(c,O_RDONLY);
					read(fd,&(b->uid),sizeof(int));
					lseek(fd,sizeof(int),SEEK_CUR);
					read(fd,&(b->length),sizeof(int));
					lseek(fd,sizeof(int),SEEK_CUR);
					read(fd,&(b->mode),sizeof(short));
					lseek(fd,sizeof(short),SEEK_CUR);
					read(fd,&(b->pos),sizeof(int));
					lseek(fd,sizeof(int),SEEK_CUR);
					close(fd);
					char e[8] = "file";
					char f[2] = "";
					sprintf(f,"%d",b->pos);
					strcat(e,f);
					int fd_ = open(e,O_RDONLY);
					read(fd_,a->name,sizeof(a->name));
					lseek(fd_,sizeof(a->name),SEEK_CUR);
					read(fd_,&(a->curr),sizeof(int));
					lseek(fd_,sizeof(int),SEEK_CUR);
					read(fd_,&(a->next),sizeof(int));
					lseek(fd_,sizeof(int),SEEK_CUR);
					read(fd_,&(a->parent),sizeof(int));
					lseek(fd_,sizeof(int),SEEK_CUR);
					//读取目录项
					for(int i=0; i < a->curr; i++){
						a->dir_[i] = new dirNode();
						read(fd_,&(a->dir_[i]->inode),sizeof(int));
						lseek(fd_,sizeof(int),SEEK_CUR);
						read(fd_,(a->dir_[i]->name),sizeof(a->dir_[i]->name));
						lseek(fd_,sizeof(int),SEEK_CUR);
						read(fd_,&(a->dir_[i]->type),sizeof(int));
						lseek(fd_,sizeof(int),SEEK_CUR);
					}
					close(fd_);
					delete curr;
					delete currINode;
					curr = a;
					currINode  = b;
					currINodeNo = p;	
					return 1;
				}
				else{
					printf("\nalready top dir");
					return -1;
				}
			}

			int cd_root(){
				if(currINodeNo != 0){
					myDir* a = new myDir();
					fileNode* b = new fileNode();
					int p = 0;	
					char c[7] = "inode";
					char d[3] = "";
					sprintf(d,"%d",p);
					strcat(c,d);	
					int fd = open(c,O_RDONLY);
					read(fd,&(b->uid),sizeof(int));
					lseek(fd,sizeof(int),SEEK_CUR);
					read(fd,&(b->length),sizeof(int));
					lseek(fd,sizeof(int),SEEK_CUR);
					read(fd,&(b->mode),sizeof(short));
					lseek(fd,sizeof(short),SEEK_CUR);
					read(fd,&(b->pos),sizeof(int));
					lseek(fd,sizeof(int),SEEK_CUR);
					close(fd);
					char e[8] = "file";
					char f[2] = "";
					sprintf(f,"%d",b->pos);
					strcat(e,f);
					int fd_ = open(e,O_RDONLY);
					read(fd_,a->name,sizeof(a->name));
					lseek(fd_,sizeof(a->name),SEEK_CUR);
					read(fd_,&(a->curr),sizeof(int));
					lseek(fd_,sizeof(int),SEEK_CUR);
					read(fd_,&(a->next),sizeof(int));
					lseek(fd_,sizeof(int),SEEK_CUR);
					read(fd_,&(a->parent),sizeof(int));
					lseek(fd_,sizeof(int),SEEK_CUR);
					//读取目录项
					for(int i=0; i < a->curr; i++){
						a->dir_[i] = new dirNode();
						read(fd_,&(a->dir_[i]->inode),sizeof(int));
						lseek(fd_,sizeof(int),SEEK_CUR);
						read(fd_,(a->dir_[i]->name),sizeof(a->dir_[i]->name));
						lseek(fd_,sizeof(int),SEEK_CUR);
						read(fd_,&(a->dir_[i]->type),sizeof(int));
						lseek(fd_,sizeof(int),SEEK_CUR);
					}
					close(fd_);
					delete curr;
					delete currINode;
					curr = a;
					currINode  = b;
					currINodeNo = p;	
					return 1;
				}
				else{
					printf("\nalready top dir");
					return -1;
				}
			}

			//登录
			int login(const char* un,const char* passwd){
				int fd = open("user",O_RDONLY);
				char na[256];
				char pa[256];
				int size = sizeof(na);
				if(read(fd,na,size) != -1){
					int i = strcmp(un,na);
					if(i != 0){
						close(fd);
						return 0;
					}
					else{
						lseek(fd,size,SEEK_CUR);
						read(fd,pa,size);
						i = strcmp(passwd,pa);
						if(i == 0){
							close(fd);
							currUser = 1;
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
			//注册
			int signup(const char* na,const char* pa){
				if(empty()){
					char a[256];
					char b[256];
					strcpy(a,na);
					strcpy(b,pa);
					int fd = creat("user",S_IRWXU);
					int size = sizeof(a);
					write(fd,a,size);
					lseek(fd,size,SEEK_CUR);
					write(fd,b,size);
					close(fd);
				}
			}
	};
	#endif

	#ifndef UI_H
	#define UI_H
	class ui{
		private:
			myFileSys a;
		public:
			ui(){
			}
			~ui(){
			}

			void _dir(){
				a.dir_();
			}

			void _create(){
					printf("\nint create(const char* pathname,int mode);");
					printf("\nreturn the file descriptor of the created file");
					printf("mode means the access bit:\n6:wr-\n4:w--\n2:-r-\n0:---");
					printf("\nplease input pathname,mode\n");
					char na[256];
					int mode;
					if(scanf("%s%d",na,&mode)==2){
						printf("\n return %d",a.create_(na,mode));
					}		
			}

			void _write(){
				printf("\nint write(int fd,char * buff,int offset,int length);");
				printf("\nreturn certain length of data writen");
				printf("\nplease input fd,offset,length\n");
				int fd,offset,length;
				char n[256];
				if(scanf("%d%s%d%d",&fd,n,&offset,&length)==3){
					printf("\n return %d",a.write_(fd,n,offset,length));
				}		
			}

			void _read(){
				printf("\nint read(int fd,char * buff,int offset,int length);");
				printf("\nreturn certain length of data writen");
				printf("\nplease input fd,offset,length\n");
				int fd,offset,length;
				char n[256];
				if(scanf("%d%d%d",&fd,&offset,&length)==3){
					printf("\n return %d",a.read_(fd,n,offset,length));
					strcat(n,"\0");
					printf("\nreads %s",n);
				}	
			}

			void _delete(){
				printf("\ndelete(const char* pathname);");
				printf("\ndelete file in current dir\n");
				char n[256];
				if(scanf("%s",n)==1){
					printf("\nresult %d", a.delete_(n));
				}
			}	

			void _open(){
				printf("\nopen(const char* pathname);");
				printf("\nopen file in current dir\n");
				char n[256];
				if(scanf("%s",n)==1){
					printf("\nfd is %d", a.open_(n));
				}
			}

			void _close(){
				printf("\nclose(int fd);");
				printf("\nclose fd namely\n");
				int b;
				if(scanf("%d",&b)==1){
					printf("\nfd is %d", a.close_(b));
				}
			} 

			void _cd(){
				int t;
				printf("\ncd has 3 type:\n1:cd \n2:cd ..\n3:cd dirname\n");
				if(scanf("%d",&t)==1){
					switch(t){
						case 1:
							a.cd_root();
							break;
						case 2:
							a.cd_last();
							break;
						case 3:
							char d[256];
							if(scanf("%s",d)==1){
								a.cd_(d);
							}
							break;
					}
				}
			}

			void _login(){
				printf("\nplease input username,password\n");
				char n[256];
				char p[256];
				if(scanf("%s%s",n,p)==2){
					a.login(n,p);
				}
			}

			void _signup(){
				printf("\nplease input username,password\n");
				char n[256];
				char p[256];
				if(scanf("%s%s",n,p)==2){
					a.signup(n,p);
				}
			}

			void _mkdir(){
				printf("\nmkdir(const cha* pathname)");
				printf("\ncreate dir in current dir\n");
				char n[256];
				if(scanf("%s",n)==1){
					a.mkdir_(n);
				}
			}

			void main_(){
				int CMD =-1;
				while(true){
					printf("\nplease input the command you want:");
					printf("\n1:login");
					printf("\n2:dir");
					printf("\n3:mkdir");
					printf("\n4:create");
					printf("\n5:open");
					printf("\n6:read");
					printf("\n7:write");
					printf("\n8:close");
					printf("\n9:delete");
					printf("\n10:cd");
					printf("\nothers:quit\n");
					cin>>CMD;
					switch(CMD){
						case 1:
							_login();
							break;
						case 2:
							_dir();
							break;
						case 3:
							_mkdir();
							break;
						case 4:
							_create();
							break;
						case 5:
							_open();
							break;
						case 6:
							_read();
							break;
						case 7:
							_write();
							break;
						case 8:
							_close();
							break;
						case 9:
							_delete();
							break;
						case 10:
							_cd();
							break;
						default:
							return;
					}
					cin.clear();
				}
			}

	};
	#endif
};