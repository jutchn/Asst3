#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

int main(int argc, char **argv)
{
	struct addrinfo hints, *address_list, *addr;
	int error;
	int sock;
	int i;
	
	if (argc < 4) {
		printf("Usage: %s [host] [port] [message(s)...]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// we need to provide some additional information to getaddrinfo using hints
	// we don't know how big hints is, so we use memset to zero out all the fields
	memset(&hints, 0, sizeof(hints));
	
	// indicate that we want any kind of address
	// in practice, this means we are fine with IPv4 and IPv6 addresses
	hints.ai_family = AF_UNSPEC;
	
	// we want a socket with read/write streams, rather than datagrams
	hints.ai_socktype = SOCK_STREAM;

	// get a list of all possible ways to connect to the host
	// argv[1] - the remote host
	// argv[2] - the service (by name, or a number given as a decimal string)
	// hints   - our additional requirements
	// address_list - the list of results

	error = getaddrinfo(argv[1], argv[2], &hints, &address_list);
	if (error) {
		fprintf(stderr, "%s", gai_strerror(error));
		exit(EXIT_FAILURE);
	}

	
	// try each of the possible connection methods until we succeed
	for (addr = address_list; addr != NULL; addr = addr->ai_next) {
		// attempt to create the socket
		sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
		
		// if we somehow failed, try the next method
		if (sock < 0) continue;
		
		// try to connect to the remote host using the socket
		if (connect(sock, addr->ai_addr, addr->ai_addrlen) == 0) {
			// we succeeded, so break out of the loop
			break;
		}

		// we weren't able to connect; close the socket and try the next method		
		close(sock);
	}
	
	// if we exited the loop without opening a socket and connecting, halt
	if (addr == NULL) {
		fprintf(stderr, "Could not connect to %s:%s\n", argv[1], argv[2]);
		exit(EXIT_FAILURE);
	}
	
	// now that we have connected, we don't need the addressinfo list, so free it
	freeaddrinfo(address_list);

	// send the remaining messages to the remote host
	for (i = 3; i < argc; ++i) {
		write(sock, argv[i], strlen(argv[i]));
		usleep(50 * 1000);
			// wait 50ms between writes, just to make things interesting
	}
	
	// close the socket
	close(sock);

	return EXIT_SUCCESS;	
}


"echos.c"
---------

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>

#define BACKLOG 5

// the argument we will pass to the connection-handler threads
struct connection {
    struct sockaddr_storage addr;
    socklen_t addr_len;
    int fd;
};

int server(char *port);
void *echo(void *arg);

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: %s [port]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

    (void) server(argv[1]);
    return EXIT_SUCCESS;
}


int server(char *port)
{
    struct addrinfo hint, *address_list, *addr;
    struct connection *con;
    int error, sfd;
    pthread_t tid;

    // initialize hints
    memset(&hint, 0, sizeof(struct addrinfo));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_PASSIVE;
    	// setting AI_PASSIVE means that we want to create a listening socket

    // get socket and address info for listening port
    // - for a listening socket, give NULL as the host name (because the socket is on
    //   the local host)
    error = getaddrinfo(NULL, port, &hint, &address_list);
    if (error != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        return -1;
    }

    // attempt to create socket
    for (addr = address_list; addr != NULL; addr = addr->ai_next) {
        sfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        
        // if we couldn't create the socket, try the next method
        if (sfd == -1) {
            continue;
        }

        // if we were able to create the socket, try to set it up for
        // incoming connections;
        // 
        // note that this requires two steps:
        // - bind associates the socket with the specified port on the local host
        // - listen sets up a queue for incoming connections and allows us to use accept
        if ((bind(sfd, addr->ai_addr, addr->ai_addrlen) == 0) &&
            (listen(sfd, BACKLOG) == 0)) {
            break;
        }

        // unable to set it up, so try the next method
        close(sfd);
    }

    if (addr == NULL) {
        // we reached the end of result without successfuly binding a socket
        fprintf(stderr, "Could not bind\n");
        return -1;
    }

    freeaddrinfo(address_list);

    // at this point sfd is bound and listening
    printf("Waiting for connection\n");
    for (;;) {
    	// create argument struct for child thread
		con = malloc(sizeof(struct connection));
        con->addr_len = sizeof(struct sockaddr_storage);
        	// addr_len is a read/write parameter to accept
        	// we set the initial value, saying how much space is available
        	// after the call to accept, this field will contain the actual address length
        
        // wait for an incoming connection
        con->fd = accept(sfd, (struct sockaddr *) &con->addr, &con->addr_len);
        	// we provide
        	// sfd - the listening socket
        	// &con->addr - a location to write the address of the remote host
        	// &con->addr_len - a location to write the length of the address
        	//
        	// accept will block until a remote host tries to connect
        	// it returns a new socket that can be used to communicate with the remote
        	// host, and writes the address of the remote hist into the provided location
        
        // if we got back -1, it means something went wrong
        if (con->fd == -1) {
            perror("accept");
            continue;
        }

		// spin off a worker thread to handle the remote connection
        error = pthread_create(&tid, NULL, echo, con);

		// if we couldn't spin off the thread, clean up and wait for another connection
        if (error != 0) {
            fprintf(stderr, "Unable to create thread: %d\n", error);
            close(con->fd);
            free(con);
            continue;
        }

		// otherwise, detach the thread and wait for the next connection request
        pthread_detach(tid);
    }

    // never reach here
    return 0;
}

void *echo(void *arg)
{
    char host[100], port[10], buf[101];
    struct connection *c = (struct connection *) arg;
    int error, nread;

	// find out the name and port of the remote host
    error = getnameinfo((struct sockaddr *) &c->addr, c->addr_len, host, 100, port, 10, NI_NUMERICSERV);
    	// we provide:
    	// the address and its length
    	// a buffer to write the host name, and its length
    	// a buffer to write the port (as a string), and its length
    	// flags, in this case saying that we want the port as a number, not a service name
    if (error != 0) {
        fprintf(stderr, "getnameinfo: %s", gai_strerror(error));
        close(c->fd);
        return NULL;
    }

    printf("[%s:%s] connection\n", host, port);

    while ((nread = read(c->fd, buf, 100)) > 0) {
        buf[nread] = '\0';
        printf("[%s:%s] read %d bytes |%s|\n", host, port, nread, buf);
    }

    printf("[%s:%s] got EOF\n", host, port);

    close(c->fd);
    free(c);
    return NULL;
}