/* 
   Author :	 Nitish Shivrayan
   Program 4 :	 Implement an in-memory filesystem (ie, RAMDISK) using FUSE
*/

#define FUSE_USE_VERSION 30
// Added to remove error regrading set FILE_OFFSET_BITS to 64
#define _FILE_OFFSET_BITS  64

// All include files
#include <fuse.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

// Variables for handling current size and total allocated size
unsigned long current_size;
unsigned long total_size; 

// File/Directory Information structure
/*
	Will be a tree like structure with a special pointer to the first child from parent and vice versa
	as well as the left sibling and right sibling information for each node.
*/
typedef struct fileInfo{
   char* fileName;			// Name of the File or directory
   int fileType;			// 0- Directory , 1- File
   unsigned long size;			// Size of the file or directory
   mode_t mode;				// Mode of the file or directory
   //Added to modify the time coming in ls -l for the created files
   time_t a_time;			// Access time of file or directory
   time_t c_time;			// Creation time of file or directory
   time_t m_time;			// Modification time of file or directory
   char* filePath;			// Full path of the file
   char* dataBlock;			// Data buffer for storing data
   struct fileInfo* rightFile;		// For storing link to information of right side file/direc on same level
   struct fileInfo* leftFile;		// For storing link to information of left side file/direc on same level
   struct fileInfo* childFile;		// For storing link to information of child files or directories
   struct fileInfo* parentFile;		// For storing link to information of parent directory
}fileInfo;

fileInfo *root,*current;
  
// Implementation of all supported RAMDISK functions as welll as helper functions : Start

/*
Getting file information from the specified path, returns the whole object.
Done by Applying Depth first search on the tree-like file system structure.
*/
fileInfo* DFS(struct fileInfo *head,const char* path)
{
    if(head == NULL){
    	return NULL;
    }
    fileInfo* output = NULL;
    if(strcmp(head->filePath,path)==0)
    {
	return head;
    }
    if (head->childFile != NULL)
    {
        output = DFS(head->childFile,path);
    }
    if (head->rightFile != NULL && output == NULL)
    {
        output = DFS(head->rightFile,path);
    }
    return output;
}

/*
Calling recursive function for Depth first search.
*/
struct fileInfo* getNode(const char* path)
{
    return DFS(root,path);
}

/*
   userLevelGetAttr() - getting the node and populating stat object information for the file.
		Steps followed are:
			1) Checking whether file exists or not.
			2) if not return -ENOENT
			3) if exists then prepare stat information as per the fileType
*/
static int userLevelGetAttr(const char* path, struct stat* st){
    current = getNode(path);
    if(current != NULL)
    {
		if (current->fileType==0)
		{	
		    st->st_nlink = 2;
		    st->st_mode = S_IFDIR | 0755;
		}else if (current->fileType==1)
		{
		    st->st_nlink = 1;
		    st->st_mode = S_IFREG | 0666;
		} 
		else
		{
		    return -ENOENT;
		}
		st->st_size = current->size;
	    st->st_atime = current->a_time;
	    st->st_mtime = current->m_time;
	    st->st_ctime = current->c_time;
    }
    else
    {
    	return -ENOENT;
    }
    return 0;
}

/*
   userLevelUtimens() - setting the access and modification time of files.
			Added to handle error in setting times while touching a file.
*/
static int userLevelUtimens(const char* path, struct timespec ts[2]){
    current = getNode(path);
    if(current != NULL)
    {
		return 0;
    }
    else
    {
		return -ENOENT;
    }
}

/* userLevelOpen() - Check whether the given path exist or not in RAMDISK 
		if it is then 
			return 0 
		else 
			return file or directory not specified error.
*/
static int userLevelOpen(const char *path, struct fuse_file_info *fi){
    current = getNode(path);
    if(current)
    {
	return 0;
    }
    else
    {
	return -ENOENT;
    }
}

/* userLevelRead() - Check whether the given path exist or not in RAMDISK 
		if it is and is not a directory then 
			Read "size" bytes from the given file into the buffer "buf", beginning "offset" bytes into the file.
			Returns the number of bytes transferred, or 0 if offset was at or beyond the end of the file.
		else 
			return "No such file" or in case it is a directory then "invalid argument" error.
*/
static unsigned long userLevelRead(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi) {
	current = getNode(path);	
	if(current == NULL)
	{
	   return -ENOENT;
	}
	else
	{	
		// If we get directory as fileType then return -EINVAL error as this function is for files
		if(current->fileType == 0)
		{
		   return -EINVAL;
		}
		else
		{
			// If offset provided is more than size of the file then return 0 as number of bytes read.
			if (offset > (current->size)) 
			{
			    return 0;
			}
		   	
			// If size of data to read is more than size of file - offset then read till the file ends
			// else if it is less than size then read till the size	given as argument   	
			if(size > ((current->size) - offset)){
				memcpy(buf,(current->dataBlock)+offset,(current->size)-offset);				
				return (current->size)-offset;
			}
			else
			{
				memcpy(buf,(current->dataBlock)+offset,size);
				return size;
			}
		}
	}
}

/* userLevelwrite() - Steps followed are:
			1) Check whether available space is there.
			2) Check whether given path is a valid file or directory.
			3) If it is first time write then write directly
			4) If already data is present then handle appropriately.
			5) Update total size used as per the write block size.				
*/
static unsigned long userLevelWrite(const char *path, const char *buf, size_t size,off_t offset, struct fuse_file_info *fi)
{
	current = getNode(path);
	if(current!=NULL)
	{
		if(current_size - current->size + offset + size > total_size)
		{
	            return -ENOSPC;
		}
		if(current->dataBlock == NULL)
		{	
			current->dataBlock = (char *)malloc(size);			
			current->size = size;	
			memcpy((current->dataBlock)+offset,buf,size);			
			current_size= current_size + size;
		}
		else
		{
			if(offset == 0)
			{
				current->size = size;
			}
			else
			{
				current->size = offset + size;
			}
			current->dataBlock = realloc(current->dataBlock,current->size);
			memcpy((current->dataBlock)+offset,buf,size);
			current_size= current_size + offset + size;
		}
		return size;	
	}
	else
	{
	    return -ENOENT;
	}	
}

/* 
	userLevelCreate() - Function to create new file , Steps followed are:
			1) Take the path and get the parent directory
			2) If no child node of that parent then add this as child node and update following
				Parent's child node
				Child's parent node
				child's left and right to NULL
				other info of child
			3) If parent has child then traverse those till current->rightFile is NULL
				current = newNode->leftFile
				current->rightFile = newNode
				newNode->rightFile = NULL
*/
static int userLevelCreate(const char *path, mode_t mode, struct fuse_file_info *fi) {
	char tmp_path[strlen(path)];
	strcpy(tmp_path, path);
	char *fileName = malloc(strlen(path) * sizeof(char));
	
	char *temp = strtok(tmp_path,"/");
	while(temp != NULL)
	{
		strcpy(fileName, temp);
		temp = strtok(NULL, "/");
	}
	strcpy(tmp_path, path);
	char *slashIndex = strrchr(tmp_path, '/');
	*slashIndex = 0;
	if(strlen(tmp_path) == 0)
	{
	    strcpy(tmp_path,"/");
	}
	fileInfo *parent = getNode(tmp_path);
	fileInfo *newNode  = (struct fileInfo*) malloc (sizeof(fileInfo));
	newNode->fileName = (char *)malloc(strlen(fileName)*sizeof(char));
	strcpy(newNode->fileName,fileName);
	newNode->fileType = 1;
	newNode->filePath = (char *)malloc(strlen(path)*sizeof(char));
	strcpy(newNode->filePath,path);
	
	if(parent->childFile == NULL)
	{
	  parent->childFile = newNode;
	  newNode->parentFile = parent;
	  newNode->size = 0;
	  newNode->rightFile = NULL;
	  newNode->leftFile = NULL;
	  newNode->childFile = NULL;
	}
	else
	{
	  current = parent->childFile;
	  while(current->rightFile != NULL)
	  {
	       current = current->rightFile;
	  }
	  newNode->leftFile = current;
	  current->rightFile = newNode;
	  newNode->size = 0;
	  newNode->rightFile = NULL;
	  newNode->childFile = NULL;
	  newNode->parentFile = NULL;
	}
	if (mode == -1) 
        {
           newNode -> mode = 0666 | S_IFREG;
        } 
        else 
    	{
    	    newNode -> mode = mode;
     	}
    	newNode -> a_time = time(0);
    	newNode -> c_time = time(0);
    	newNode -> m_time = time(0);
	newNode -> dataBlock = NULL;
	return 0;
}

/* 
	userLevelMkdir() - Function to create new Directory , Steps followed are:
			1) Take the path and get the parent directory
			2) If no child node of that parent then add this as child node and update following
				Parent's child node
				Child's parent node
				child's left and right to NULL
				other info of child
			3) If parent has child then traverse those till current->rightFile is NULL
				current = newNode->leftFile
				current->rightFile = newNode
				newNode->rightFile = NULL		
*/
static int userLevelMkdir(const char *path, mode_t mode){
	char tmp_path[strlen(path)];
	strcpy(tmp_path, path);
	char *fileName = malloc(strlen(path) * sizeof(char));
	
	char *temp = strtok(tmp_path,"/");
	while(temp != NULL)
	{
		strcpy(fileName, temp);
		temp = strtok(NULL, "/");
	}
	strcpy(tmp_path, path);
	char *slashIndex = strrchr(tmp_path, '/');
	*slashIndex = 0;
	if(strlen(tmp_path) == 0)
	{
	    strcpy(tmp_path,"/");
	}
	fileInfo *parent = getNode(tmp_path);

	fileInfo *newNode  = (struct fileInfo*) malloc (sizeof(fileInfo));
	newNode->fileName = (char *)malloc(strlen(fileName)*sizeof(char));
	strcpy(newNode->fileName,fileName);
	newNode->fileType = 0;
	newNode->filePath = (char *)malloc(strlen(path)*sizeof(char));
	strcpy(newNode->filePath,path);
	
	if(parent->childFile == NULL)
	{
	  parent->childFile = newNode;
	  newNode->parentFile = parent;
	  newNode->rightFile = NULL;
	  newNode->leftFile = NULL;
	  newNode->childFile = NULL;
	}
	else
	{
	  current = parent->childFile;
	  while(current->rightFile != NULL)
	  {
	       current = current->rightFile;
	  }
	  newNode->leftFile = current;
	  current->rightFile = newNode;
	  newNode->rightFile = NULL;
	  newNode->childFile = NULL;
	  newNode->parentFile = NULL;
	}
	
	if (mode == -1) 
        {
           newNode -> mode = 0755 | S_IFDIR;
        } 
        else 
    	{
    	    newNode -> mode = mode;
     	}
    	newNode -> a_time = time(0);
    	newNode -> c_time = time(0);
    	newNode -> m_time = time(0);
	return 0;
}

/* 
	userLevelUnlink() - Function to deleting file , remove this file information:
				1) Find the specified Directory
				2) If not found error
				3) If found but is a directory then error
				4) If found and is a file then first update its next link
				5) then update parent's child node
				6) Delete the node		
*/
static int userLevelUnlink(const char *path){
        current = getNode(path);
	if (current == NULL) 
        {
	    return -ENOENT;
        }
	else
	{
	    if(current->fileType == 0)
	    {
	        return -EINVAL;
	    }
	    else
	    {
		current_size = current_size +  current->size;
		if(current->rightFile!=NULL)
		{
		    if(current->parentFile!=NULL)
		    {
			current->rightFile->parentFile = current->parentFile;
			current->parentFile->childFile = current->rightFile;
		    }
		    if(current->leftFile!=NULL)
		    {		
			current->leftFile->rightFile = current->rightFile;
		    }
		    else
	            {
			current->rightFile->leftFile = NULL;
		    }
		}
		else
		{
		    if(current->parentFile!=NULL)
		    {
			current->parentFile->childFile = NULL;
		    }
		    if(current->leftFile!=NULL)
		    {					
		        current->leftFile->rightFile = NULL;
		    }
		}
	    }
    	}
	return 0;	
}

/* 
	userLevelRmdir() - Function to deleting Directory , Steps involved are:
				1) Find the specified Directory
				2) If not found error
				3) If found but is a file then error
				4) If found and is a directory and has no children then first update its next link
				5) then update parent's child node
				6) Delete the node
				7) If found and is a directory and has children then error
*/
static int userLevelRmdir(const char *path){
	current = getNode(path);
	if (current == NULL) 
        {
	    return -ENOENT;
        }
	else
	{
	    if(current->fileType == 1)
	    {
	        return -EINVAL;
	    }
	    else
	    {
		if(current->childFile == NULL)
		{
			current_size = current_size +  current->size;
			if(current->rightFile!=NULL)
			{
			    if(current->parentFile!=NULL)
			    {
				current->rightFile->parentFile = current->parentFile;
				current->parentFile->childFile = current->rightFile;
			    }
		            if(current->leftFile!=NULL)
			    {		
			        current->leftFile->rightFile = current->rightFile;
			    }
			    else
			    {
				current->rightFile->leftFile = NULL;
			    }
			}
			else
			{
			    if(current->parentFile!=NULL)
			    {
				current->parentFile->childFile = NULL;
			    }
			    if(current->leftFile!=NULL)
			    {		
			        current->leftFile->rightFile = NULL;
			    }
			}
		}
		else
		{
			return -EINVAL;
		}
	    }
	}
	free(current);
	return 0;
}

/* 
	userLevelOpendir() - Function for opening directory , basically for cd command	
			     Steps followed are:
				1) Check whether given path is a valid file or directory.
				2) If file present return 0
*/
static int userLevelOpendir(const char *path) {
	current = getNode(path);
	if (current == NULL) 
        {
	    return -ENOENT;
        }
	else
	{
	    return 0;
	}
}


/* 
	userLevelTruncate() - Function for truncate file , basically for write operation	
			      Steps followed are:
				1) Check whether given path is a valid file or directory.
				2) If file present return 0
*/
static int userLevelTruncate(const char * path, off_t offset) {
	current = getNode(path);
	if (current == NULL) 
        {
	    return -ENOENT;
        }
	else
	{
	    return 0;
	}
}

/* 
	userLevelReaddir() - Function for reading directory , basically for ls command
			     Steps followed are:
				1) Check whether given path is a valid file or directory.
				2) If file present Filler function call for . , .. , and all subdirectories(if available)
*/
static int userLevelReaddir(const char *path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi){
    current = getNode(path);
    if (current == NULL) 
    {
	return -ENOENT;
    }
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    
    current = current->childFile;
    while (current != NULL)
    {
	filler(buf, current->fileName, NULL, 0);
        current = current->rightFile;
    }
    return 0;
}

// Implementation of all supported RAMDISK functions : End

// Mapping system call to user defined functions for all supported RAMDISK operations
static struct fuse_operations ramdisk_operations = {
   .getattr = userLevelGetAttr,
   .readdir = userLevelReaddir,
   .open = userLevelOpen,	
   .read = userLevelRead,
   .write = userLevelWrite,
   .create = userLevelCreate,
   .mkdir = userLevelMkdir,
   .rmdir = userLevelRmdir,
   .unlink = userLevelUnlink,
   .opendir = userLevelOpendir,
   .utimens = userLevelUtimens,
   .truncate = userLevelTruncate,
};

// Main driver call for RAMDISK functionality
int main(int argc, char *argv[])
{
  // If number of arguments less than 3 or more than 4 then tell usage
  if (argc < 3 || argc > 4) {
     printf("Usage: %s <Directory to mount> <Size of directory> [file name]\n",argv[0]);
     exit(1);
  }

  // If provided size is 0 then throw error
  if (atoi(argv[2]) == 0) { 
      printf("Size of the directory to be mounted cannot be 0\n");
      exit(1);
  }

  // Need to convert the size to MB
  total_size = atoi(argv[2])*1024*1024;
  current_size = 0;

  // Preparing arguments for fuse_main call
  int index;
  int reqArgc = argc - 1;
  char* reqArgv[reqArgc];
  for(index=0;index<reqArgc;index++){
     reqArgv[index] = argv[index];
  }

  // Preparing root information
  root = (struct fileInfo*) malloc (sizeof(fileInfo));
  root->fileName = malloc(10);
  strcpy(root->fileName,"root");
  root->fileType = 0;
  root->filePath = malloc(10);
  strcpy(root->filePath,"/"); 
  root->rightFile = NULL;
  root->leftFile = NULL;
  root->parentFile = NULL; 
  root->childFile = NULL;
  root->dataBlock = NULL;
  root->size = 0;
  root -> mode = 0755 | S_IFDIR;
  root -> a_time = time(0);
  root -> c_time = time(0);
  root -> m_time = time(0);
  
  // Calling fuse_main function
  return fuse_main(reqArgc, reqArgv, &ramdisk_operations, NULL);
}
