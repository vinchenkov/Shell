/****************************************************************
 * Name        :                                                *
 * Class       : CSC 415                                        *
 * Date        :                                                *
 * Description :  Writting a simple bash shell program          *
 *                that will execute simple commands. The main   *
 *                goal of the assignment is working with        *
 *                fork, pipes and exec system calls.            *
 ****************************************************************/

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <sys/wait.h>
#include <fcntl.h>

#define BUFFERSIZE 256
#define PROMPT "myShell >> "
#define PROMPTSIZE sizeof(PROMPT) 

struct inputComponents {   
	char** myargs[6];
	char* inFile; 
	char* outFile;  
	
	int isABackground;
	int hasOut; 
	int hasIn;  
	int hasBuiltin;
	int cmdCount;  
	int myargc; 
}; 

void getInput(char* input, char* dir) {  
	size_t bufsize = BUFFERSIZE;
	printf("myShell ");
	getwd(dir); 
	printf("%s >> ", dir); 
	getline(&input, &bufsize, stdin);  
	int length = strlen(input); 
	if(length > 1 ) {
		if (input[length - 1] == '\n') {
			input[length - 1] = '\0'; 
		}     
	}
}  

void parseInput(struct inputComponents * cmp, char * input) {  
	
	char * token = malloc(256);  
	
	//when a redirection symbol is found, an I/O file is coming next
	int incomingInFile = 0; 
	int incomingOutFile = 0;  
	
	cmp->isABackground = 0;
	cmp->myargc = 0; 
	cmp->cmdCount = 1;  
	cmp->hasIn = 0; 
	cmp->hasOut = 0; 
	
	token = strtok(input, " ");     
	while( token != NULL ) {   
		if( (strcmp( token, "|")) == 0 ){ 
			cmp->cmdCount++;  
			cmp->myargc = 0;    
		} 
		else if(incomingInFile) { 
			cmp->inFile = token;  
			cmp->hasIn = 1; 
			incomingInFile = 0;
		} 
		else if(incomingOutFile) { 
			cmp->outFile = token; 
			cmp->hasOut = 1; 
			incomingOutFile = 0;			
		} 
		else if ( (strcmp( token, ">")) == 0 ) { 
			incomingOutFile = 1; 
			cmp->hasOut = 1;
		}  
		else if ( (strcmp( token, ">>")) == 0 ) { 
			incomingOutFile = 1; 
			cmp->hasOut = 2;
		} 
		else if ( (strcmp( token, "<")) == 0 ) { 
			incomingInFile = 1;  
		}  
		else if( (strcmp( token, "&")) == 0 ) {  
			cmp->isABackground = 1;  
			write(1, "hey", 3);
		}
		else{ 
			(cmp->myargs[cmp->cmdCount - 1])[cmp->myargc] = token;
			cmp->myargc++;  
		}		
		token = strtok( NULL, " ");    
	}  
} 

int executeBuiltin(char** builtin) { 
	if( strcmp(builtin[0], "cd") == 0) { 
		chdir(builtin[1]); 
		return 1;
	} 
	else if( strcmp(builtin[0], "help") == 0 ){ 
		write(1, "USE SHELL AS YOU WOULD IN A LINUX SHELL\n", 40);  
		return 1;
	}  
	else{ 
		return 0;
	}
}

int executeComponents(struct inputComponents * cmp) { 
	int stdrdin = dup(0); 
	int stdrdout = dup(1); 
	int in; 
	int out;  
	int status;
	pid_t pid;   
	
	if(cmp->hasIn) {  
		in = open(cmp->inFile, O_RDONLY);  
	} 
	else{  
		in = dup(stdrdin); 
	}    
	
	//check for builtins
	if( strcmp((cmp->myargs[0])[0], "cd") == 0 || strcmp((cmp->myargs[0])[0], "help") == 0 || strcmp((cmp->myargs[0])[0], "exit") == 0) {
		status = executeBuiltin(cmp->myargs[0]); 	
	}
	else{
		for(int i = 0; i < cmp->cmdCount; i++) {  
			dup2(in, 0); 
			close(in);    
			
			//if last command set out to stdout or outfile
			if( i == cmp->cmdCount -1 ) { 
				if(cmp->hasOut) {  
					if(cmp->hasOut == 1){
						out = open( cmp->outFile, O_WRONLY);  
					} 
					else{ 
						out = open( cmp->outFile, O_APPEND);  
					}
				} 
				else { 
					out = dup(stdrdout); 
				} 
			}  
			//if not last command
			else {  
				int mypipe[2]; 
				pipe(mypipe); 
				in=mypipe[0]; 
				out=mypipe[1]; 
			} 
			
			dup2(out, 1); 
			close(out); 
			pid = fork(); 
			if( pid == 0 ) {    
				execvp((cmp->myargs[i])[0], cmp->myargs[i]); 
				exit(0); 
			}  
		} 
	}
	
	dup2(stdrdin, 0); 
	dup2(stdrdout, 1); 
	close(stdrdout); 
	close(stdrdin);    
	
	if( !cmp->isABackground ) { 
		waitpid(pid, 0, 0); 
	}		 
	return status; 
}   

int main(int* argc, char** argv){ 
	 
	int status = 1;    
	size_t bufsize = BUFFERSIZE;  
	
	while(status) {  	 
		
		// allocation | reallocation
		struct inputComponents * components = malloc(sizeof(*components));
		char * input = malloc(bufsize * sizeof(char*));  
		char * dir = malloc(bufsize * (sizeof(char*)));   
		components->outFile= malloc(bufsize * sizeof(char*));
		components->inFile= malloc(bufsize * sizeof(char*)); 
		for(int i = 0; i < 6; i++) {  
			components->myargs[i] = malloc(bufsize * sizeof(char*));
		}
		
		getInput(input, dir);     
		if(input != NULL){ 
			parseInput(components, input); 
			status = executeComponents(components);    
			free(input);  
			free(dir); 
			free(components);
		}    
	}    
return 0;
}  


