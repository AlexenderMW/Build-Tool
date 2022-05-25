/*
Programmer Name: Alexender Meltzer-Werner
Date Submitted: 10/4/2020
File Purpose: Create a basic framwork for the game connect four where the player wins if 'a' is selected, quits when 'q' is selected, and continues otherwise if any other choise is selected.
Date Updated: 10/4/2020
Purpose for Update: adding documentation and error handlling
Global Variable List: 

//used to iterate through tid[]
int threadCount = 0;
//thread ids created so that threads can be created inside a loop, breaks if more than 100 threads are needed 
pthread_t tid[100];
//stores the base directory this program is in
char basePath[1000];
semaphore used for signaling threads
sem_t semaphore;
//value used to create different library.a directories
int libraryNum = 0;

Value Semantics declaration:
	there are no value semantics

Dynamic Memory Usage declaration:
	pathAndFile - use to create strings of variable length containing the path and file name

Static variable list:
	no static variables declared

Functions:
//void *handleFile(void *path);
//precondition: Must be handed the path to a .c file, must be called by a thread
//postcondition: calls system to compile .c file and creates a .o file inside the current directory.  When thread finishes semaphore incremented and thread exits with NULL
//void dirTraversal(char *path);
//precondition:Path to base directory that program is given as input
//postccondition:
//Recursively explores all files and directories inside base directory
//When a .c file is found and its not in the base directory, nor is there more than 4 threads are in current use, creates a thread that calls handleFile passing the file and path
//If in a directory that is not the base directory calls system to create static library located in base directory with all .o files 
*/\
#include <stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <semaphore.h>
#include <pthread.h>
int threadCount = 0; //used to iterate through tid[]
pthread_t tid[100];//thread ids created so that threads can be created inside a loop, breaks if more than 100 threads are needed 
char basePath[1000];//stores the base directory this program is in
sem_t semaphore;//value used to create different library.a directories
int libraryNum = 0;//value used to create different library.a directories
void *handleFile(void *path);
void dirTraversal(char *path);
int main(void)
{
	//initializing variables
	sem_init(&semaphore, 0, 4);
	char path[PATH_MAX];
	char sysCommand[10000];
	//getting base path and saving it
	getcwd(path, sizeof(path));
	if(path == NULL)
	{
		printf("error, cannot open current directory");
	}
	strcpy(basePath, path);
	//look through directories recursively
	dirTraversal(path);
	strcpy(sysCommand, "gcc ");
	DIR *directory = opendir(path);
	struct dirent *current;
	//creating system command to link all .a and .o files in base directory
	while((current = readdir(directory)) != NULL)
	{
		int length = strlen(current->d_name);
		const char *fileType = &current->d_name[length - 2];
		if((strcmp(fileType, ".a") == 0) || (strcmp(fileType, ".o") == 0))
		{
			char *pathAndFile = malloc(strlen(path) + strlen(current->d_name) + 2);
			strcpy(pathAndFile, path);
			strcat(pathAndFile, "/");
			strcat(pathAndFile, current->d_name);
			strcat(sysCommand, " ");
			strcat(sysCommand, pathAndFile);
			free(pathAndFile);
		}
	}
	strcat(sysCommand, " -o myProgram");
	printf("--------------->here we go %s\n", sysCommand);
	//if the files do not complie exit with error
	if(system(sysCommand) != 0)
	{
		printf("failed to compile and link!\n");
		exit(1);
	}
	sem_destroy(&semaphore);
	return 0;
}
//precondition:Path to base directory that program is given as input
//postccondition:
//Recursively explores all files and directories inside base directory
//When a .c file is found and there is more than 4 threads are in current use, creates a thread that calls handleFile passing the file and path
//If in a directory that is not the base directory calls system to create static library located in base directory with all .o files 
void dirTraversal(char *path)
{
	int compiled = 0;
	char updatePath[1000];
	DIR *directory = opendir(path);
	struct dirent *current;
	//iterating through directory
	while((current = readdir(directory)) != NULL)
	{
		//if file recurse into file
	 	if(current->d_type == DT_DIR)
		{
			if(strcmp(current->d_name, ".") != 0 && strcmp(current->d_name, "..") != 0)
			{
				strcpy(updatePath, path);
				strcat(updatePath, "/");
				strcat(updatePath, current->d_name);
				dirTraversal(updatePath);
			}
		}
		//if file is a .c crates thread and calls handleFile
		else
		{
			int length = strlen(current->d_name);
			const char *fileType = &current->d_name[length - 2];
			if(strcmp(fileType, ".c") == 0)
			{
				//building path and file locaiton
				char *pathAndFile = malloc(strlen(path) + strlen(current->d_name) + 2);
				strcpy(pathAndFile, path);
				strcat(pathAndFile, "/");
				strcat(pathAndFile, current->d_name);
				//waiting for less than 4 threads to be active
				sem_wait(&semaphore);
				pthread_create(&tid[threadCount], NULL, &handleFile, (void *)pathAndFile);
				pthread_join(tid[threadCount], NULL);
				free(pathAndFile);	
				++threadCount;
				++compiled;	
			}
		}
	}
	//if a .c file was compiled and was not located in the base path creates a static .a library with all .o files inside the base path
	if(compiled && (strcmp(path, basePath) != 0))
	{
		libraryNum++;
		char temp[10];
		//building library directory name
		sprintf(temp, "%d", libraryNum);
		char library[50];
		strcpy(library, "library");
		strcat(library, temp);
		strcat(library, ".a");
		char sysCommand[10000];
		strcpy(sysCommand, "ar rcs ");
		strcat(sysCommand, library);
		directory = opendir(path);
		//builiding system command
		while((current = readdir(directory)) != NULL)
		{
			int length = strlen(current->d_name);
			const char *fileType = &current->d_name[length - 2];
			if(strcmp(fileType, ".o") == 0)
			{
				char *pathAndFile = malloc(strlen(path) + strlen(current->d_name) + 2);
				strcpy(pathAndFile, path);
				strcat(pathAndFile, "/");
				strcat(pathAndFile, current->d_name);
				strcat(sysCommand, " ");
				strcat(sysCommand, pathAndFile);
				free(pathAndFile);
			}
		}
		//calling system with command
		printf("\n\ncall: %s\n\n", sysCommand);
		system(sysCommand);
	}
	closedir(directory);
}
//precondition: Must be handed the path to a .c file, must be called by a thread
//postcondition: calls system to compile .c file and creates a .o file inside the current directory.  When thread finishes semaphore incremented and thread exits with NULL
void *handleFile(void *path)
{
	//building system command to compiile .c file
	char *pathAndFile = (char *)path;
	printf("%s\n", pathAndFile);
	char sysCommand[1000]; 
	strcpy(sysCommand, "gcc -c -o ");
	pathAndFile[strlen(pathAndFile)-1] = 0;
	strcat(pathAndFile, "o");
	strcat(sysCommand, pathAndFile);
	pathAndFile[strlen(pathAndFile)-1] = 0;
	strcat(pathAndFile, "c");
	strcat(sysCommand, " ");
	strcat(sysCommand, pathAndFile);
	//if .c file fails to complie exit(1)
	if(system(sysCommand) != 0)
	{
		printf("failed to compile exiting\n");
		exit(1);
	}
	//incrementing semaphore to allow another thread
	sem_post(&semaphore);
	pthread_exit(NULL);
}