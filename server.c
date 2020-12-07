#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#define MAX 80 
#define PORT 8080 
#define SA struct sockaddr 

// Function designed for chat between client and server. 
void func(int sockfd) 
{ 
	char *buff = (char*)malloc(sizeof(char)*MAX);
	int n; 
	// infinite loop for chat 
	for (;;) { 
// knock knock
		bzero(buff, MAX); 
		strcpy(buff, "REG|13|Knock, knock.|");
		write(sockfd, buff, sizeof(buff));
		bzero(buff, MAX); 
		// read the message from client and copy it in buffer 
// this is whos there
		//reg
		int charsRead = 0;
		while(charsRead != 3){
			int temp = read(sockfd, buff, 3-charsRead);
			charsRead += temp;
		}
		if(!strcmp(buff, "REG")){
			printf("error pee"); 
		} 
		bzero(buff, MAX); 
		read(sockfd,buff,1);
		if(!buff[0] == "|"){
			printf("error pee");
		}
		// end reg
		// start number
		char tempString[1];
		int temp = 0
		while(1){
			read(sockfd, tempString, 1);
			if(isDigit(tempString)){
				buff[temp] = tempString;
				temp++;
			} else if(tempString[0] == "|") {
				break;
			} else {
				printf("shit error");
			}
		}
		// end number
		// start word
		int size = atoi(buff);
		bzero(buff, MAX); 
		charsRead = 0;
		while(charsRead != size){
			int temp = read(sockfd, buff, size-charsRead);
			charsRead += temp;
		}
		if(strcmp(buff, "Who's there?")!=0){
			printf("error fuckk");
		}
		bzero(buff, MAX); 
		read(sockfd,buff,1);
		if(buff[0] != "|"){
			printf("error pee");
		}
		bzero(buff, MAX);
		// send joe 
		char* joke = "REG|4|Joe."
		int jokelen = 4;

		strcpy(buff, joke);
		write(sockfd, buff, sizeof(buff));
		bzero(buff, MAX);
		
		// this is whos there
		//reg
		int charsRead = 0;
		while(charsRead != 3){
			int temp = read(sockfd, buff, 3-charsRead);
			charsRead += temp;
		}
		if(!strcmp(buff, "REG")){
			printf("error pee"); 
		} 
		bzero(buff, MAX); 
		read(sockfd,buff,1);
		if(!buff[0] == "|"){
			printf("error pee");
		}
		// end reg
		// start number
		char tempString[1];
		int temp = 0
		while(1){
			read(sockfd, tempString, 1);
			if(isDigit(tempString)){
				buff[temp] = tempString;
				temp++;
			} else if(tempString[0] == "|") {
				break;
			} else {
				printf("shit error");
			}
		}
		// end number
		if(atoi(buff) != 6+jokelen){
			printf("error shit")
		}
		bzero(buff, MAX);


		// start word
		int size = atoi(buff);
		bzero(buff, MAX); 
		charsRead = 0;
		while(charsRead != size){
			int temp = read(sockfd, buff, size-charsRead);
			charsRead += temp;
		}
		char who[] = ", who?";
		for(int i = 0; i<6+jokelen; i++){
			if(i<jokelen){
				if(buff[i] != joke[i]){
					printf("poopoo");
				}
			} else {
				if(buff[i] != who[i-jokelen]){
					printf("poopoo");
				}
			}
		}
		bzero(buff, MAX); 
		read(sockfd,buff,1);
		if(buff[0] != "|"){
			printf("error pee");
		}
		bzero(buff, MAX);
		//end word




		// print buffer which contains the client contents 
		printf("From client: %s\t To client : ", buff); 
		bzero(buff, MAX); 
		n = 0; 
		// copy server message in the buffer 
		while ((buff[n++] = getchar()) != '\n') 
			; 

		// and send that buffer to client 
		write(sockfd, buff, sizeof(buff)); 

		// if msg contains "Exit" then server exit and chat ended. 
		if (strncmp("exit", buff, 4) == 0) { 
			printf("Server Exit...\n"); 
			break; 
		} 
	} 
} 

// Driver function 
int main() 
{ 
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
	servaddr.sin_port = htons(PORT); 
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
		printf("server accept failed...\n"); 
		exit(0); 
	} 
	else
		printf("server accept the client...\n"); 

	// Function for chatting between client and server 
	func(connfd); 

	// After chatting close the socket 
	close(sockfd); 
} 