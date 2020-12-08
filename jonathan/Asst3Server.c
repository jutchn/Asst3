#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>

#define MAXBUFFER 20 // Initial Max Size for string buffers when reading
#define MAXINTSIZE 10 // Max num digits for signed integer (2^31) when converting to char  
#define MAXERRSIZE 10 // Max size for error msg (ERR|4 char code|)
#define MAXERRCODE 4 // Max size for error code (e.g. M1CT)
#define NULLTERMINATORSIZE 1 // Size of '\0' to avoid random +1s
#define PIPESIZE 1 // Size of '|' to avoid random +1s
#define TYPELENGTH 3 // Size for message typing (e.g. ERR or REG)

// Struct for linked list of jokes
typedef struct JokeNode{
    char* setup;
    char* punch;
    struct JokeNode* next;
} JokeNode;

// Function to add joke to head of a list
void addJoke(JokeNode** head, char* setup, char* punch){
    JokeNode* joke = (JokeNode*)malloc(sizeof(JokeNode));
    joke->setup = (char*)malloc(sizeof(char)*(strlen(setup)+NULLTERMINATORSIZE));
    strcpy(joke->setup, setup);
    joke->punch = (char*)malloc(sizeof(char)*(strlen(punch)+NULLTERMINATORSIZE));
    strcpy(joke->punch, punch);
    joke->next = (*head);
    
    (*head) = joke;
}

// Function to get a random joke from a list
JokeNode* getRandomJoke(JokeNode* head){
    // Empty list case, return NULL pointer
    if(head == NULL) return NULL;

    // List of only one case, return the one to avoid extra calculations
    if(head->next == NULL) return head;
    
    // Get the size of the list
    int count = 0;
    JokeNode* temp = head;
    while(temp != NULL){
        count++;
        temp = temp->next;
    }

    // Initialize the random seed based on the current time
    srand((unsigned) time(0));

    // Iterate to random position in list and return the joke
    temp = head;
    for(int i = rand() % count; i > 0; i--){
        temp = temp->next;
    }
    return temp;
}

// Function to read in a joke file 
// Returns 0 if list is empty or can't be read, 1 if read is successful
int initializeJokeFile(JokeNode** head, char* path){
    FILE* input_file = fopen(path, "r");
    if(input_file == NULL){
	    printf( "Error: File can't be read at %s\n" , path);
	    return 0;
    }
    
    // Look through file character by character
    char c = (char)fgetc(input_file);
    while(c != EOF){
        int jokeSize = MAXBUFFER;
        char* setup = (char*)malloc(sizeof(char)*jokeSize);
        char* punch = (char*)malloc(sizeof(char)*jokeSize);
        int numChars;

        // Pass all newlines
        while(c=='\n'){
            c = (char)fgetc(input_file);
        }

        // Read in the setup line
        numChars = 0;
        while(c != '\n' && c != EOF){
            // Reallocate memory if setup is too long
            if(numChars == jokeSize){
                jokeSize *= 2;
                setup = (char*)realloc(setup,jokeSize*sizeof(char));
            }
            setup[numChars++] = c;
            c = (char)fgetc(input_file);
        }
        setup[numChars] = '\0';

        // If file ends without punchline, don't add joke to list
        if(c == EOF){
            break;
        }

        // Pass all newlines
        while(c=='\n'){
            c = (char)fgetc(input_file);
        }

        // Read in the punchline
        numChars = 0;
        jokeSize = MAXBUFFER;
        while(c != '\n' && c != EOF){
            // Reallocate memory if setup is too long
            if(numChars == jokeSize){
                jokeSize *= 2;
                punch = (char*)realloc(punch,jokeSize*sizeof(char));
            }
            punch[numChars++] = c;
            c = (char)fgetc(input_file);
        }
        punch[numChars] = '\0';
        
        // Add joke to the list
        addJoke(head, setup, punch);

        if(c != EOF){
            c = (char)fgetc(input_file);
        }
    }
    fclose(input_file);

    // If list still empty return 0, otherwise 1
    if(*head == NULL){
        return 0;
    } else {
        return 1;
    }
}

// Function to free a list
void freeList(JokeNode* head){
    JokeNode* temp;
    while(head!=NULL){
        temp = head;
        head = head->next;
        free(temp);
    }
    return;
}

// Function to convert a string to KKJ format
char* toKKJ(char* str){
    //Turn length of string into a string
    char size[MAXINTSIZE];
    sprintf(size, "%d", (int)strlen(str));

    // Concatenate the string into KKJ format
    char* newString = (char*)malloc(sizeof(char)*
        (TYPELENGTH+PIPESIZE+strlen(size)+PIPESIZE+
        strlen(str)+PIPESIZE+NULLTERMINATORSIZE));
    strcpy(newString, "REG|");
    strcat(newString, size);
    strcat(newString, "|");
    strcat(newString, str);
    strcat(newString, "|");
    return newString;
}

// Function to send an error to client
// Errors follow the format ERR|M(msg number)(2 letter error type)|
void sendError(int fd, int msgNum, char* errType){
    char err[MAXERRSIZE];
    strcpy(err, "ERR|M");
    char num[MAXINTSIZE];
    sprintf(num, "%d", msgNum);
    strcat(err, num);
    strcat(err, errType);
    strcat(err, "|");

    write(fd, err, strlen(err)+NULLTERMINATORSIZE);
    return;
}

// Function to receive KKJ message from client
// KKJ messages must follow the form:
// REG|<message length>|<message>| or ERR|<error code>|
// returns 0 if there is an error, otherwise 1
int receiveKKJ(int fd, int msgNum, char* msgMatch){
    int nread = -1; // counter to track how many bytes read from client
    int received = 0; // counter to track assigned position in char array
    int buffSize = MAXBUFFER; // int of size of memory to allocate/reallocate
	char* buff = (char*)malloc(sizeof(char)*buffSize); // Buffer to store reads from client
    char* message; // Buffer to store message from client

    //Loop through until first '|' or until the type is greater than size 3
    while(nread != 0){
        nread = read(fd, &buff[received], 1);
        if(buff[received] == '|'){
            break;
        }
        // Send an error if the type surpases the allowed length
        if(received >= TYPELENGTH){
            printf( "Error type is too long\n" );
            sendError(fd, msgNum, "FT");
            return 0;
        }
        if(buff[received] != '\0' && buff[received] != '\n') {
            received += nread;
        }
    }
    // Check if client disconnects
    if(nread == 0){
        printf( "Client Connection Closed\n" );
        free(buff);
        return 0;
    }
    buff[received] = '\0';

    // Send an error if the type is less than the allowed length
    if(received != TYPELENGTH){
        printf( "Error type is too short\n" );
        sendError(fd, msgNum, "FT");
        return 0;
    }
    
    
    if(strcmp(buff, "REG") == 0){ // Case for when type is REG

        // Read in size until the next '|'
        received = 0;
        while(nread != 0){
            nread = read(fd, &buff[received], 1);
            // Reallocate if the size number is too large (unlikely)
            if(received == MAXBUFFER){
                buffSize *= 2;
                buff = (char*)realloc(buff,sizeof(char)*buffSize);
            }

            if(buff[received] == '|') {
                break;
            }
            // Send an error if the size isn't strictly numbers
            if(!isdigit(buff[received])){
                printf( "Error length should only be digits\n" );
                sendError(fd, msgNum, "FT");
                free(buff);
                return 0;
            }

            if(buff[received] != '\0' && buff[received] != '\n') {
                received += nread;
            }
        }
        // Check if client disconnects
        if(nread == 0){
            printf( "Client Connection Closed\n" );
            free(buff);
            return 0;
        }
        buff[received] = '\0';

        // Allocate memory for message based on received size
        long msgLen = atol(buff);
        message = (char*)malloc(sizeof(char)*(msgLen+NULLTERMINATORSIZE));
        
        // Read in message until the next '|' or until message passes allowed size
        received = 0;
        while(nread != 0){
            nread = read(fd, &message[received], 1);
            if(message[received] == '|'){
                break;
            }
            // Send an error if received string is too long
            if(received >= msgLen){
                printf( "Error received string too long\n" );
                sendError(fd, msgNum, "LN");
                free(buff);
                return 0;
            }
            if(message[received] != '\0') {
                received += nread;
            }
        }
        // Check if client disconnects
        if(nread == 0){
            printf( "Client Connection Closed\n" );
            free(buff);
            return 0;
        }
        // Send an error if received string is too short
        if(received != msgLen){
            printf( "Error received string too short\n" );
            sendError(fd, msgNum, "LN");
            free(buff);
            free(message);
            return 0;
        }    
        message[received] = '\0';

        // Send an error if received string doesn't match the desired string
        if(strcmp(message, msgMatch) != 0){
            printf( "Error strings dont match\n" );
            sendError(fd, msgNum, "CT");
            free(buff);
            free(message);
            return 0;
        }

        // Successfully receive message
        printf("Message %d Received Successfully: ", msgNum);
        printf(  "%s\n" , message);
        free(message);
        free(buff);
        return 1;
    } else if(strcmp(buff, "ERR") == 0){ // Case for when type is ERR
        // Read in error code until the '|' or until code passes allowed length
        received = 0;
        while(nread != 0){
            nread = read(fd, &buff[received], 1);
            if(buff[received] == '|'){
                break;
            }
            // Print error if the error code is too large
            if(received >= MAXERRCODE){
                printf( "Error in error syntax\n" );
                free(buff);
                return 0;
            }
            if(buff[received] != '\0' && buff[received] != '\n') {
                received += nread;
            }
        }
        // Check if client disconnects
        if(nread == 0){
            printf( "Error Client Connection Closed\n" );
            free(buff);
            return 0;
        }
        buff[received] = '\0';
        // Print error if the error code is too small
        if(received != MAXERRCODE){
            printf( "Error in error syntax\n" );
            free(buff);
            return 0;
        }

        // Print error code
        printf( "Error Code: %s\n" , buff);
        free(buff);
        return 0;
    } else { // Send an error if type isn't REG or ERR
        printf("Error in Message Typing\n");
        sendError(fd, msgNum, "FT");
        free(buff);
        return 0;
    }
}

// Function to handle the server protocol
// Server sends 3 messages and receives 3 messages
void kkjProtocol(int fd, JokeNode* joke) 
{
    // Send initial to client
	char* kkjStart = toKKJ("Knock, knock.");
	write(fd, kkjStart, strlen(kkjStart)+1);
    printf("Message 0 Sent Successfully: Knock, knock.\n");
    free(kkjStart);

    // Receive response from client, check for errors
    // Must follow form "Who's there?"
    int msg1correct = receiveKKJ(fd, 1, "Who's there?");
    if(!msg1correct) {
        printf("Exited with Error in Message 1. Waiting for next client...\n");
        return;
    }

    // Send joke setup to client
    char* kkjSetup = toKKJ(joke->setup);
	write(fd, kkjSetup, strlen(kkjSetup)+NULLTERMINATORSIZE); 
    printf("Message 2 Sent Successfully: %s\n", joke->setup);
    free(kkjSetup);

    // Receive response from client, check for errors
    // Must follow form "<setup line>, who?"
    char* expected = (char*)malloc(sizeof(char)*(strlen(joke->setup)+
        strlen(", who?")+NULLTERMINATORSIZE));
    strcpy(expected, joke->setup);
    strcat(expected, ", who?");
    int msg3correct = receiveKKJ(fd, 3, expected);
    free(expected);
    if(!msg3correct) {
        printf( "Exited with Error in Message 3. Waiting for next client...\n" );
        return;
    }

    // Send joke punchline to client
    char* kkjPunch = toKKJ(joke->punch);
	write(fd, kkjPunch, strlen(kkjPunch)+NULLTERMINATORSIZE); 
    printf("Message 4 Sent Successfully: ");
    printf( "%s\n" , joke->punch);
    free(kkjPunch);

    // Receive expression of A/D/S from client, check for errors
    // Must follow form "Ugh."
    int msg5correct = receiveKKJ(fd, 1, "Ugh.");
    if(!msg5correct) {
        printf( "Exited with Error in Message 5. Waiting for next client...\n" );
        return;
    } 

    // Print when protocol over
    printf("Knock Knock Joke Protocol Complete. Waiting for next client...\n");
} 

int main(int argc, char **argv)
{
    // Check for correct amount of parameters
    if (argc < 2 || argc > 3) {
		printf( "INVALID PARAMETERS\nPlease follow the form: " );
        printf( "%s <port number> <joke file (optional)>\n" , argv[0]);
		exit(EXIT_FAILURE);
	}

    //Initialize Socket Params
    int socketFD;
    int clientFD;
    struct sockaddr_in serverAddress;
    struct sockaddr_in client;
    socklen_t clientSize = sizeof(client);

    //Initialize List of Jokes
    JokeNode** head = (JokeNode**)malloc(sizeof(JokeNode*));

    //If there is a joke file, read in the file and add the jokes to the list 
    if(argc == 3){
        int hasJokes = initializeJokeFile(head, argv[2]);
        //If the file can't be opened or has no jokes, initialize default joke
        if(!hasJokes){
            addJoke(head, "Who", "I didn't know you were an owl!");
        }
    } else {
        //Otherwise initialize the list with a default joke
        addJoke(head, "Who", "I didn't know you were an owl!");
    }

	// Create the socket 
    socketFD = socket(AF_INET, SOCK_STREAM, 0); 
    if (socketFD == -1) { 
        printf("Error: Failed to create socket\n"); 
        exit(EXIT_FAILURE); 
    } 

    // Assign IP and Port number
    serverAddress.sin_family = AF_INET; 
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY); 
    serverAddress.sin_port = htons(atoi(argv[1])); 

    // Bind the socket to the server address and listen
    if ((bind(socketFD, (struct sockaddr*) &serverAddress, sizeof(serverAddress))) != 0) { 
        printf("Error: Failed to bind socket\n"); 
        exit(EXIT_FAILURE); 
    } 
    if ((listen(socketFD, 5)) != 0) { 
        printf("Error: Socket failed to listen\n"); 
        exit(EXIT_FAILURE); 
    } 

    // Loop forever, looking for clients
    while(1){
        // Get a random joke from the list to be sent to the client
        JokeNode* randomJoke = getRandomJoke(*head);

        // Try to accept client 
        clientFD = accept(socketFD, (struct sockaddr*)&client, &clientSize); 

        // If no client, keep looping until finding one
        if (clientFD < 0) { 
            continue;
        }

        // When client found, start the knock knock joke protocol
        printf("Connected to Client...\n");
        kkjProtocol(clientFD, randomJoke); 

        // After protocol, close client 
        close(clientFD);
    }

    // Free the joke list and close the server socket
    // Since the loop goes forever, we never actually reach this
    // But ideally the loop should close and free everything
    freeList(*head);
    free(head);
    close(socketFD);
    return EXIT_SUCCESS;	
}
