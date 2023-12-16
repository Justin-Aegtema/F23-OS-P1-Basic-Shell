//if you have any feedback on anything, I'd love to hear it! always trying to grow

//include headers for 
//standard input/output, memory allocation, strings, 
//system calls, sys call data types, wait, file control, and error codes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

//argc counts args, argv[1] stores file to read from
int main(int argc, char *argv[]) {

//allocate memory to pathArray, set initial values of pathArray to "path" and "/bin/"
//so, /bin/ is the initial path available to users
char** pathArray = (char** )malloc(4 * sizeof(char* ));
pathArray[0] = "path";//it probably doesn't matter, but by initializing this I'll always know pathArray[0] == "path"
pathArray[1] = "/bin/";
pathArray[2] = (char* )malloc(1 * sizeof(char));
pathArray[2][0] = '\0';//null character at end of valid pathArray entries

//set parameters for getline()
FILE* inputOption = NULL; //inputOption will point to either user input or file to read from
char* inputCommand = NULL;// input data is called "inputCommand" regardless of whether user input or file
size_t initialBufferSize = 0;
ssize_t didReadSucceed;//stores whether getline() successfull reads input
int anyInfoReadFromFile = 1;//always equals 1 for userInput; set to 0 for batch files until at least 1 non-null line is read(to catch bad batch files)

//batch mode
if (argc == 2) {
	inputOption = fopen(argv[1], "r");//set inputOption to specified file

	//call error if user specified file cannot be opened
    	if (inputOption == NULL) {
            char error_message[30] = "An error has occurred\n";
	    write(STDERR_FILENO, error_message, strlen(error_message));
	    exit(1);
	}

	anyInfoReadFromFile = 0; //set to 0 to recognize we are reading from batch and we have not read from batch successfully yet
}

if (argc >= 3) {//call error if ./dash main has too many input arguments
	char error_message[30] = "An error has occurred\n";
 	write(STDERR_FILENO, error_message, strlen(error_message));
	exit(1);
}

//WHILE LOOP - big while loop is core functionality of DASH shell
//loops through running specified input Commands
while (1){

//manual user input mode
if (argc <= 1) {
	printf("dash> ");//print "DASH> " perpetually
	fflush(stdout); //print "DASH> " immediately
	inputOption = stdin;//set inputOption to standard input (keyboard)
}

errno = 0;//reset errno so it reflects results of getline

//read input from user or from specified file 
didReadSucceed = getline(&inputCommand, &initialBufferSize, inputOption);

//check for input error and EOF
if (didReadSucceed == -1){
	if (errno) {//error message if getline() had an error while reading
		char error_message[30] = "An error has occurred\n";
		write(STDERR_FILENO, error_message, strlen(error_message));
		exit(1);
	}else if (anyInfoReadFromFile == 0) {//error message if getline only got -1 NULL + nothing has been read so far (bad batch file)
                char error_message[30] = "An error has occurred\n";
		write(STDERR_FILENO, error_message, strlen(error_message));
		exit(1);

	}else {//-1 NULL signals normal end of file as long as errno = 0 and info has been read from file
		exit(0);//End of File triggers exit(0)
	}
}
anyInfoReadFromFile = 1; //if we've made it here once, it means that at least one line was successfully read, so it is not a bad batch file

//remove newline character from inputCommand (it was  added by getline())
int inputCommandLength = strlen(inputCommand);
if (inputCommand[inputCommandLength - 1] == '\n'){
	inputCommand[inputCommandLength - 1] = '\0';
}

int inputBlankLine = 0;
if (inputCommand[0] == '\0') {//strlen(inputCommand) == 0){//checks if there is nothing in the inputCommand after removing newline
        inputBlankLine = 1;
}

//int hasRunBuiltIn = 0; //variable stores whether built in functions cd or path have been run (exit is handled differently)

//exit if inputCommand is "exit"
if (strcmp(inputCommand,"exit") == 0){		
	exit(0);
}

char* remainingCommands;//initialize character pointers to keep track of remaining commands from input
char* currentCommand;//initialize character pointer to keep track of current command from input
currentCommand = strtok_r(inputCommand, "&", &remainingCommands);//sets currentCommand to the first command (it is the only command if there are no "&" symbols)

//initialize process ID and childCount to keep track of parent vs child and help parent wait for all children
pid_t childPID = 1;
int childCount = 0;

if (currentCommand == NULL && inputBlankLine != 1) {//checks if the inputCommand had something before, but not after tokenizing based on "&" - returns error call if after removing '\n' only &'s were left
	char error_message[30] = "An error has occurred\n";
	write(STDERR_FILENO, error_message, strlen(error_message));
}

//BIG WHILE LOOP loop through all user commands
while (currentCommand != NULL) {
int hasRunBuiltIn = 0; //variable stores whether built in functions cd or path have been run (exit is handled differently)

//cd
if (currentCommand[0] == 'c' && currentCommand[1] == 'd' && (currentCommand[2] == ' '|| currentCommand[2] == '\0')) {//check currentCommand starts with "cd"
	char* currentArg;
	int argCount = 0;//track number of args in command

	//loop through cd tokens; 2nd token - 1st arg -  is the new directory
	currentArg = strtok(currentCommand, " \t");
	while(currentArg != NULL) {
		if (argCount == 1) {//change directory to the 2nd token ("currentArg" when argCount == 1)
			if (chdir(currentArg) == 0) {
			}
               		else { //error for chdir fail
				char error_message[30] = "An error has occurred\n";
				write(STDERR_FILENO, error_message, strlen(error_message));
			}
		}
		currentArg = strtok(NULL, " \t");//get next  currentCommand token
		argCount++;
	}
	
	if (argCount != 2) {//error for wrong argCount: if actual cd args are anything other than 1 count, argCount != 2 will trigger
               	char error_message[30] = "An error has occurred\n";
		write(STDERR_FILENO, error_message, strlen(error_message));
	}
	hasRunBuiltIn = 1;//record that the built in function cd has been run
}

//path
if (currentCommand[0] == 'p' && currentCommand[1] == 'a' && currentCommand[2] == 't' && currentCommand[3] == 'h' &&  (currentCommand[4] == ' '|| currentCommand[4] == '\0')) {//check currentCommand starts with "path"
	int pathIter = 0;

	//recall that pathArray stores the current path

	//erase previous  path
	while (pathArray[pathIter] != NULL){
		pathArray[pathIter] = '\0';
		pathIter++;
	}
	
	int pathArrayCapacity = 1;
	
	//iterate through currentCommand for new paths, counting and allocating memory as we go
	char* current_Path = strtok(currentCommand, " \t");//first token is the word "path"
	int pathCount = 1;
	while (current_Path != NULL) {
		//allocate new memory if pathCount > capacity
                if ((pathCount+1) >= pathArrayCapacity) {
			pathArrayCapacity = 2*pathArrayCapacity; //double capacity when more is needed
			pathArray = (char** )realloc(pathArray, pathArrayCapacity * sizeof(char* )); 
		}
		pathArray[pathCount-1] = (char* )malloc(1000);
 		strcpy(pathArray[pathCount-1], current_Path);//key: store current_Path in pathArray for future use
		strcat(pathArray[pathCount-1], "/");//add "/" so that path string concatenates correctly

		pathCount++;
		current_Path = strtok(NULL, " \t");//use strtok to move token to next path in currentCommand
	}
        pathArray[pathCount] = (char* )malloc(1 * sizeof(char));
	pathArray[pathCount][0] = '\0'; //signal the end of pathArray with null character

	hasRunBuiltIn = 1;//record that the built in function path has been run
}

//Formatting note: up until this point, indents had ignored that they were inside a while loop for currentCommand; now, they are indented for that

	//create a forked process, recording childPID
	//if built in function cd or path has been run, no forked process is created
	if (hasRunBuiltIn == 0) {
		childPID = fork();
		if (childPID > 0){ //add to the childCount every time the parent goes through this loop
			childCount++;
		}
	}

	//BEGIN CHILD CODE - execute command with arguments (only if it is a child process)
	if (childPID == 0) {
		int length = strlen(currentCommand);//check if command ends in ">" or " >" or " \t>"
		if ( (length >= 2 && currentCommand[length - 2] == ' ' && currentCommand[length - 1] == '>')  || (length >= 2 && currentCommand[length - 2] == '\t' && currentCommand[length - 1] == '>') || (length >= 1 && currentCommand[length - 1] == '>')){
			char error_message[30] = "An error has occurred\n";
			write(STDERR_FILENO, error_message, strlen(error_message));
                        exit(0);
		}

		//initialize variables to keep track of strtok_r() for redirect, ultimately in order to find the output file and set output to go there
		char* remainingRedirect;
		char* currentRedirect;
		int redirectIter = 0;
			
		currentRedirect = strtok_r(currentCommand, ">", &remainingRedirect);//currentRedirect becomes the command before ">"

		while (currentRedirect != NULL){
			
			//this code calls a bug on whitespace that should be ignored....
			//if (redirectIter == 0) {//error if there is nothing before ">" except whitespace
                        //      char* remainingExportFile;
			//	char* currentExportFile;
			//	currentExportFile = strtok_r(currentRedirect, " \t", &remainingExportFile);
			//	if (currentExportFile == NULL) {
			//		char error_message[30] = "An error has occurred\n";
	                //        	write(STDERR_FILENO, error_message, strlen(error_message));
			//		exit(0);
			//	}	
			//}
		
			//this while loop doesn't do much unless redirectIter == 1
			//when redirectIter == 1, we are on the 2nd token of input based on ">" delimeter
			//this token specifies the file name
			//this code allows output to be sent to the specified file
			if(redirectIter == 1) {
				//initialize variables to keep track of strtok_r() for Export File
				char* remainingExportFile;
				char* currentExportFile;
				int exportFileIter = 0;
				
				//uses strtok_r to remove whitespace from between ">" and file name 
				currentExportFile = strtok_r(currentRedirect, " \t", &remainingExportFile);
				
				if (currentExportFile == NULL) {//error if there is nothing after ">" except whitespace
                                        char error_message[30] = "An error has occurred\n";
					write(STDERR_FILENO, error_message, strlen(error_message));
					exit(0);
				}
				
				while (currentExportFile != NULL) {
					
					if(exportFileIter == 0) {
						//code to set up to write to file
						//open/create & obtain file number for desired output file number
						int fd = open(currentExportFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
							
						// Redirect stdout to the file
						dup2(fd, STDOUT_FILENO);
						close(fd);//close file
					} else if(exportFileIter >= 1) {//error called if whitespace creates more than 1 token after ">"
	                                       	char error_message[30] = "An error has occurred\n";
						write(STDERR_FILENO, error_message, strlen(error_message));
						exit(0);
					}
					exportFileIter++;//iterate
					currentExportFile = strtok_r(NULL, " \t", &remainingExportFile);//iterate to the next token after ">" that is separated by white space (if there are two more " \t" separated tokens after ">', it is an error
				}
			}else if(redirectIter > 1) {//error if there's more than one ">"
				char error_message[30] = "An error has occurred\n";
				write(STDERR_FILENO, error_message, strlen(error_message));
				exit(0);
			}
				
			redirectIter++;
                       	currentRedirect = strtok_r(NULL, ">", &remainingRedirect);//get next ">" token
		}

		//the above code creates/finds the file listed after ">" and opens it to send it standard output from execv()
			
		//this code also finds a token delimited by ">"
		//but does so in order to ignore everything after ">" so that it can focus on command and its args that come before ">"
		char* remainingRedirectV2;
		char* currentRedirectV2;
			
		//currentRedirectV2 - this string contains everything before the first ">"
		//or everything in the currentCommand, if there is no ">"
		currentRedirectV2 = strtok_r(currentCommand, ">", &remainingRedirectV2);
		
		int pathIter = 1;
		int whiteSpaceCommand = 0; //used check if the entire text before ">" is just ignorable whiteSpace
		while (pathArray[pathIter] != NULL) {//while loop to check if the currentCommand executes on any path within pathArray
			//initialize values to loop through currentCommand's args using strtok_r()
			char* remainingArgs; 
			char* currentArg;
			char* args[1000000];
			int firstArg = 0;//first arg keeps track of whether we're on the first "arg" -- the command
			int argCounter = 0;
			char currentPath[50000] = "";//initialize empty string currentPath
			//currentPath[0] = '\0';

			//sets currentArg to the first "argument" which is actually the command
			currentArg = strtok_r(currentRedirectV2, " \t", &remainingArgs);
			
			//check here to see if first "argument" the command is NULL - meaning that the space before ">" is entirely blank whitespace delimiters (" " and "\t")
			if(currentArg == NULL) {
				whiteSpaceCommand = 1;
			}

			//loop through the command and any arguments
			while (currentArg != NULL) {
				if (firstArg == 1) {
                      		args[argCounter] = currentArg;//add args after the command to agrs[]
				} else if (firstArg == 0) {
					//special code appends the command(the "first arg") to the path
					strcat(currentPath, pathArray[pathIter]);//currentPath store the current value from pathArray
					strcat(currentPath, currentArg);//append command to the path
					args[argCounter] = pathArray[pathIter];//stores command in args[]
					firstArg = 1;//records that command has been appended to the path
				}
				currentArg = strtok_r(NULL, " \t", &remainingArgs);//iterate to the next arg token to add it to args[]
				argCounter++;
			}
			args[argCounter] = NULL;//mark end of args[] array
			if (access(currentPath, X_OK) == 0){//check if execute access is allowed on currentPath
				execv(currentPath, args);//execute current command with args as specified by currentCommand
			}
			pathIter++;//get ready to try the next path
		}
		if (whiteSpaceCommand != 1) {//failed commands do not signal error if it's just whitespace
			char error_message[30] = "An error has occurred\n";//this error only prints if all the execv() calls on all specified paths fail
			write(STDERR_FILENO, error_message, strlen(error_message));
		}
		exit(0);//exit child process since execv() never did
	}
	//Now we're FINALLY BACK TO THE PARENT
	//iterate to the next currentCommand token among remainingCommands
	//since all children will be run before parent is asked to wait, all child commands will operate in parallel
	currentCommand = strtok_r(NULL, "&", &remainingCommands);

}//END OF CURRENT COMMAND WHILE LOOP

//parent counted each child in childCount
//this code calls wait(NULL) to wait for all children to end
if (childPID > 0) {
	int iter;
	for (iter = 0; iter < childCount; iter++){
		wait(NULL);
	}
}

}//END OF WHILE(1)
//this should never occur, so returns -1 to signal error
return -1;
}
