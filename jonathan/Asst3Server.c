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

#define RESETCOLOR "\033[0m"
#define ERRORCOLOR "\033[31m"
#define NULLTERMINATORSIZE 1
#define MAXSTRINGLENGTH 20
#define MAXKKJ 1000
#define SA struct sockaddr 

typedef struct JokeNode{
    char* setup;
    char* punch;
    struct JokeNode* next;
} JokeNode;

void addJoke(JokeNode** head, char* setup, char* punch){
    JokeNode* joke = (JokeNode*)malloc(sizeof(JokeNode));
    joke->setup = (char*)malloc(sizeof(char)*strlen(setup) + NULLTERMINATORSIZE);
    strcpy(joke->setup, setup);
    joke->punch = (char*)malloc(sizeof(char)*strlen(punch) + NULLTERMINATORSIZE);
    strcpy(joke->punch, punch);
    joke->next = (*head);
    
    (*head) = joke;
}

JokeNode* getRandomJoke(JokeNode* head){
    if(head == NULL) return NULL;
    if(head->next == NULL) return head;
    
    int count = 0;
    JokeNode* temp = head;
    while(temp != NULL){
        count++;
        temp = temp->next;
    }

    srand((unsigned) time(0));
    temp = head;
    for(int i = rand() % count; i > 0; i--){
        temp = temp->next;
    }
    return temp;

}

void initializeJokeFile(JokeNode** head, char* path){
    FILE* input_file = fopen(path, "r");
    if(input_file == NULL){
	    printf(ERRORCOLOR "Error: File can't be read at %s\n" RESETCOLOR, path);
	    return;
    }
    
    char c = (char)fgetc(input_file);
    while(c != EOF){
        int jokeSize = MAXSTRINGLENGTH;
        char* setup = (char*)malloc(sizeof(char)*jokeSize);
        char* punch = (char*)malloc(sizeof(char)*jokeSize);
        int numChars;
        while(c=='\n'){
            c = (char)fgetc(input_file);
        }
        numChars = 0;
        while(c != '\n' && c != EOF){
            if(numChars == jokeSize){
                jokeSize *= 2;
                setup = (char*)realloc(setup,jokeSize*sizeof(char));
            }
            setup[numChars++] = c;
            c = (char)fgetc(input_file);
        }
        setup[numChars] = '\0';

        while(c=='\n'){
            c = (char)fgetc(input_file);
        }
        numChars = 0;
        jokeSize = MAXSTRINGLENGTH;
        while(c != '\n' && c != EOF){
            if(numChars == jokeSize){
                jokeSize *= 2;
                punch = (char*)realloc(punch,jokeSize*sizeof(char));
            }
            punch[numChars++] = c;
            c = (char)fgetc(input_file);
        }
        punch[numChars] = '\0';
        
        addJoke(head, setup, punch);

        if(c != EOF){
            c = (char)fgetc(input_file);
        }
    }
    fclose(input_file);
}
void sendJoke(int sockfd, JokeNode* joke) 
{ 
	char message[MAXKKJ]; 
	int n; 

	bzero(message, MAXKKJ); 
	// copy server message in the buffer 
	strcpy(message, "REG|13|Knock, Knock.|");
	// and send that buffer to client 
	write(sockfd, message, sizeof(message));
	// read the message from client and copy it in buffer 
    bzero(message, MAXKKJ); 
	int temp = read(sockfd, message, sizeof(message));
    printf("%d\n", temp);
    printf("%s\n", message);
    //printf("%s\n", message);
    //printf("%d\n", strcmp(message,"REG|12|Who's there?|"));
    //while(strcmp(message,"REG|12|Who's there?|") != 0){
    //    read(sockfd, message, sizeof(message));
    //}
	bzero(message, MAXKKJ); 
	n = 0; 
    // copy server message in the buffer 
	strcpy(message, joke->setup);
	// and send that buffer to client 
	write(sockfd, joke->setup, sizeof(joke->setup)); 
	// read the message from client and copy it in buffer 
    bzero(message, MAXKKJ); 
    temp = read(sockfd, message, sizeof(message));
    printf("%d\n", temp);
    printf("%s\n", message);
    //printf("%s\n", message);
    //printf("%d\n", strcmp(message,"REG|12|Who's there?|"));
    //while(strcmp(message,"REG|12|Who's there?|") != 0){
    //    read(sockfd, message, sizeof(message));
    //}

    bzero(message, MAXKKJ); 
	n = 0; 
    // copy server message in the buffer 
	strcpy(message, joke->punch);
    printf("%s\n", joke->punch);
    printf("%s\n", message);
	// and send that buffer to client 
	write(sockfd, joke->punch, sizeof(joke->punch)); 
	// read the message from client and copy it in buffer 
    bzero(message, MAXKKJ); 
	read(sockfd, message, sizeof(message));
    //printf("%s\n", message);
    //printf("%d\n", strcmp(message,"REG|12|Who's there?|"));
    //while(strcmp(message,"REG|12|Who's there?|") != 0){
    //    read(sockfd, message, sizeof(message));
    //}
    printf("done\n");
} 
int main(int argc, char **argv)
{
    // Check for correct amount of parameters
    if (argc < 2 || argc > 3) {
		printf(ERRORCOLOR "INVALID PARAMETERS\nPlease follow the form: %s <port number> <joke file (optional)>\n" RESETCOLOR, argv[0]);
		exit(EXIT_FAILURE);
	}

    //Initialize List of Jokes
    JokeNode** head = (JokeNode**)malloc(sizeof(JokeNode*));

    //If there is a joke file, read in the file and add the jokes to the list 
    if(argc == 3){
        initializeJokeFile(head, argv[2]);
    } else {
        //Otherwise initialize the list with a default joke
        addJoke(head, "Who", "I didn't know you were an owl!");
    }
    
    JokeNode* randomJoke = getRandomJoke(*head);
    printf("Knock Knock\nWho's there?\n%s\n%s, who?\n%s\nUgh\n",randomJoke->setup, randomJoke->setup, randomJoke->punch);
	
    int sockfd, connfd, len; 
	struct sockaddr_in servaddr, cli; 

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		printf("socket creation failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully created..\n"); 
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	servaddr.sin_port = htons(atoi(argv[1])); 
	//htonl(uint32_t hostlong): converts the normal unsigned integer hostlong from host byte order to network byte order.
	//htons()
	//ntohl()

	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
		printf("socket bind failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully binded..\n"); 

	// Now server is ready to listen and verification 
	if ((listen(sockfd, 5)) != 0) { 
		printf("Listen failed...\n"); 
		exit(0); 
	} 
	else
		printf("Server listening..\n"); 
	len = sizeof(cli); 

	// Accept the data packet from client and verification 
	connfd = accept(sockfd, (SA*)&cli, &len); 
	if (connfd < 0) { 
		printf("server acccept failed...\n"); 
		exit(0); 
	} 
	else
		printf("server acccept the client...\n"); 

	// Function for chatting between client and server 
	sendJoke(connfd, randomJoke); 

	// After chatting close the socket 
	close(sockfd); 
    return EXIT_SUCCESS;	
}
