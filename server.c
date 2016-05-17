#include "server.h"

int main(int argc, char* argv[]) {
	struct sockaddr_in clientaddr;
    	socklen_t addrlen;
	char c, PORT[6] = "8000";
	int i, connfd;
    	
	// Default Port = 8000 / path = cwd
	path = (char*)malloc(sizeof(char) * MAX_PATH);
	webroot();
	
    	//Parsing the command line arguments
    	while((c = getopt(argc, argv, "p:r:")) != -1) {
		switch(c) {
	    		case 'r':
				strcpy(path,optarg);
				break;
	    		case 'p':
				strcpy(PORT,optarg);
				break;
	    		case '?':
				fprintf(stderr,"Wrong arguments given!!!\n");
				exit(1);
	    		default:
				exit(1);
		}
	}

	printf("HTTP Server Start\nPort No. - %s%s%s\nRoot directory - %s%s%s\n",
	    		"\033[92m",
    			PORT,
    			"\033[0m",
    			"\033[92m",
    			path,
    			"\033[0m"
	      );

	server_init(PORT);

    	// ACCEPT connections
    	while(1) {
		p.ready_set = p.read_set;
		p.nready = select(p.maxfd + 1, &p.ready_set, NULL, NULL, NULL);

		addrlen = sizeof(clientaddr);		
    		
		if(FD_ISSET(listenfd, &p.ready_set)) {
			connfd = accept(listenfd, (struct sockaddr *) &clientaddr, &addrlen);
    			if(connfd < 0)
				perror("ACCEPT ERROR");
			
			FD_SET(connfd, &p.read_set);
			if(connfd > p.maxfd)
				p.maxfd = connfd;
			p.nready--;
		}
		
		check_clients();
	}
    	return 0;
}

//start server
void server_init(char *port) {
    	struct addrinfo hints, *res, *paddr;
	
    	// getaddrinfo for host
	bzero(&hints, sizeof(hints));
    	hints.ai_family = AF_INET;
    	hints.ai_socktype = SOCK_STREAM;
    	hints.ai_flags = AI_PASSIVE;
    	if(getaddrinfo(NULL, port, &hints, &res) != 0) {
		perror("GETADDRINFO ERROR");
		exit(1);
    	}

    	// socket and bind
    	for(paddr = res; paddr != NULL; paddr = paddr->ai_next) {
		listenfd = socket(paddr->ai_family, paddr->ai_socktype, 0);
		if(listenfd == -1)
			continue;
		if(bind(listenfd, paddr->ai_addr, paddr->ai_addrlen) == 0)
			break;
    	}
    	if(paddr == NULL) { // socket / bind error control
		perror("SOCKET / BIND ERROR");
		exit(1);
    	}
	
    	freeaddrinfo(res);
	
    	// listen for incoming connections
    	if(listen(listenfd, 1000) != 0) {
		perror("LISTEN ERROR");
		exit(1);
    	}

	// initialize pool
	p.maxfd = listenfd;
	FD_ZERO(&p.read_set);
	FD_SET(listenfd, &p.read_set);
}

//client connection
void parser(int cfd) {
    	char mesg[MAX_MSG], *reqline[3], data_to_send[BYTES], *s;
    	int rcvd, fd, bytes_read;
	int hflag = -1;	

    	memset((void*)mesg, (int)'\0', MAX_MSG);
	
    	rcvd = recv(cfd, mesg, MAX_MSG, 0);
	
    	if(rcvd < 0)
		fprintf(stderr, "RECEIVE ERROR\n");
    	else if(rcvd == 0) {
		fprintf(stderr, "Unexpected Disconnection.\n");
	}
    	else {   // message received
    		printf("%s", mesg);
		reqline[0] = strtok(mesg, " \t\n");
		if(strncmp(reqline[0], "GET\0", 4) == 0) {
	    		reqline[1] = strtok(NULL, " \t");
	    		reqline[2] = strtok(NULL, " \t\n");
			if(strncmp(reqline[2], "HTTP/1.0", 8) == 0)
				hflag = 0;
			else if(strncmp(reqline[2], "HTTP/1.1", 8) == 0)
				hflag = 1;
			else
				write(cfd, "HTTP 400 Bad Request\n", 25);
	    		
			if (strncmp(reqline[1], "/\0", 2) == 0)
		    		reqline[1] = "/index.html";        
			// index.html will be opened by default
			
			webroot();
			strcpy(&path[strlen(path)], reqline[1]);
			printf("file: %s\n\n", path);
			
			s = strchr(path, '.');
			if(strcmp(s + 1, "php") == 0) {
				php_cgi(cfd, reqline[1]);
			}
			else if((fd = open(path, O_RDONLY)) != -1) {   //FILE FOUND
				if(hflag == 0)
					send(cfd, "HTTP/1.0 200 OK\n\n", 17, 0);
				else if(hflag == 1)
					send(cfd, "HTTP/1.1 200 OK\n\n", 17, 0);

				while((bytes_read = read(fd, data_to_send, BYTES)) > 0)
					write(cfd, data_to_send, bytes_read);
			}
			else {	// FILE NOT FOUND
				if(hflag == 0)
					write(cfd, "HTTP/1.0 404 Not Found\n", 23); 
				else if(hflag == 1)
					write(cfd, "HTTP/1.1 404 Not Found\n", 23);
			}
		}
    	}
	
	// Somewhere makes /Sample.html duplicate -> need to fix
	// Guess reqline & path of global var setting problem
    	// SOCKET Close back in check_clients function
}

void webroot() {
	FILE *fp;
	
	if(fp = fopen("conf", "rt")) {
		fgets(path, MAX_PATH, fp);
		fclose(fp);
	}
	else
		strcpy(path, getenv("PWD"));
}

void check_clients() {
	int s;
	
	for(s = 0; (s < p.maxfd + 1) && (p.nready > 0); s++) {
		if(s == listenfd)
			continue;
		
		if(FD_ISSET(s, &p.read_set) && FD_ISSET(s, &p.ready_set)) {
			p.nready--;
			parser(s);
			
		  	//Further sent and received operations are DISABLED
			shutdown(s, SHUT_RDWR);
			close(s);
			FD_CLR(s, &p.read_set);
			if(s == p.maxfd) {
				p.maxfd--;
				while(!FD_ISSET(p.maxfd, &p.read_set))
					p.maxfd--;
			}
		}
	}
}

void php_cgi(int fd, char *s) {
        dup2(fd, STDOUT_FILENO);
        char script[500];
	strcpy(script, "SCRIPT_FILENAME=");
	strcat(script, path);
	putenv(script);
	//strcpy(script, "SCRIPT_NAME=");
	//strcat(script, s);
	//putenv(script);
	putenv("GATEWAY_INTERFACE=CGI/1.1");
	//putenv("QUERY_STRING=");
	//putenv("PATH_INFO=/");
	putenv("REQUEST_METHOD=GET");
	putenv("REDIRECT_STATUS=true");
	putenv("SERVER_PROTOCOL=HTTP/1.1");
	//strcpy(script, "REQUEST_URL=");
	//strcat(script, s);
	//putenv(script);
	//putenv("HTTP_HOST=localhost");
	//putenv("REMOTE_HOST=127.0.0.1");
	execl("/usr/bin/php-cgi", "php-cgi", NULL);
}

