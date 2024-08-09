#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <unistd.h>

 struct thread_args{
	int* fd;
	int arg_c;
	char** arg_v;
};

void* handle_request(void* client_fd);

int main(int argc, char* argv[]) {
	setbuf(stdout, NULL);
 	setbuf(stderr, NULL);

	// You can use print statements as follows for debugging, they'll be visible when running tests.
	printf("Logs from your program will appear here!\n");

	// Uncomment this block to pass the first stage
	//
	int server_fd, client_addr_len;
	struct sockaddr_in client_addr;

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		printf("Socket creation failed: %s...\n", strerror(errno));
		return 1;
	}

	// Since the tester restarts your program quite often, setting SO_REUSEADDR
	// ensures that we don't run into 'Address already in use' errors
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		printf("SO_REUSEADDR failed: %s \n", strerror(errno));
		return 1;
	}

	struct sockaddr_in serv_addr = { .sin_family = AF_INET ,
		.sin_port = htons(4221),
		.sin_addr = { htonl(INADDR_ANY) },
	};

	if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		printf("Bind failed: %s \n", strerror(errno));
		return 1;
	}

	int connection_backlog = 5;
	if (listen(server_fd, connection_backlog) != 0) {
		printf("Listen failed: %s \n", strerror(errno));
		return 1;
	}

	printf("Waiting for a client to connect...\n");
	client_addr_len = sizeof(client_addr);

	//handle_req
	while(1){
		int id = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
		pthread_t thread;
		int* thread_args = &id;
		struct thread_args* Thread_args = (struct thread_args*)malloc(sizeof(struct thread_args));
		Thread_args->fd = &id;
		Thread_args->arg_c = argc;
		Thread_args->arg_v = argv;
		if(pthread_create(&thread, NULL, &handle_request, (void*)Thread_args) != 0)
			printf("thread cannot be created\n");
		pthread_detach(thread);
	}
	close(server_fd);

	return 0;
}

void* handle_request(void* args){
	printf("Client connected\n");
	struct thread_args* t_args = (struct thread_args*)args;

	int id = *((int*)t_args->fd);
	char request[1024];
	read(id, request, sizeof(request));

	char* path = (char*)malloc(sizeof(request));
	char* original_path = path;
	strcpy(path, request);
	path = strtok(path, " ");
	path = strtok(NULL, " ");
	char res[1024];

	if(strcmp(path, "/") == 0){
		sprintf(res, "HTTP/1.1 200 OK\r\n\r\n");
	}
	else{
		char* req = strtok(path, "/");
		if(strcmp(req, "echo") == 0){
			req = strtok(NULL, "/");
			int len = strlen(req);
			sprintf(res, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s", len, req);
		}
		else if(strcmp(req, "files") == 0){
			printf("enterting the else condition\n");
			if(t_args->arg_c > 1 && strcmp(t_args->arg_v[1], "--directory") == 0){
				char* directory = (char*)malloc(1024);
				strcpy(directory, t_args->arg_v[2]);
				printf("unalterd directory = %s\n",directory);
				char* file = strtok(NULL, "/");

				while(file != NULL){
					directory = strcat(directory, file);
					directory = strcat(directory, "/");
					file = strtok(NULL, "/");
				}

				if(strlen(directory) > 0)
					directory[strlen(directory) - 1] = '\0';
				printf("%s\n",directory);
				int file_fd = open(directory, O_RDONLY);
				if(file_fd == -1){
					char* not_found_message = "File not found\n";
					sprintf(res, "HTTP/1.1 404 Not Found\r\nContent-Type: application/octet-stream\r\nContent-Length: %d\r\n\r\n%s", strlen(not_found_message), not_found_message);
				}
				else{
					FILE* file_ptr = fdopen(file_fd, "r");
					struct stat *buff = (struct stat*)(malloc(sizeof(struct stat)));
					fstat(file_fd, buff);
					char buffer[1024];
					size_t read_size;
					char file_data[1024];
					file_data[0] = '\0';
					while(read_size = fread(buffer, 1, sizeof(buffer), file_ptr) > 0){
						strcat(file_data, buffer);
					}
					sprintf(res, "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: %d\r\n\r\n%s", (int)buff->st_size, file_data);
					fclose(file_ptr);
				}
				close(file_fd);
			}
		}
		else if(strcmp(req, "user-agent") == 0){
			char* user_agent = strtok(request, "\n");
			while(strncmp(user_agent, "User-Agent: ", 12) != 0){
				user_agent = strtok(NULL, "\n");
			}
			user_agent = strtok(user_agent, " ");
			user_agent = strtok(NULL, " ");
			int len = strlen(user_agent) - 1;
			sprintf(res, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s", len, user_agent);
		}
		else{
			sprintf(res, "HTTP/1.1 404 Not Found\r\n\r\n");
		}
	}
	send(id, res, strlen(res), 0);
	free(original_path);
	return NULL;
}
