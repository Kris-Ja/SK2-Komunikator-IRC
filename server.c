#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<string.h>
#include<netdb.h>
#include<stdlib.h>
#include<stdio.h>
#include<sys/wait.h>
#include<pthread.h>
#include<openssl/ssl.h>

#define MAX_THREADS 30
#define MAX_CHATS 30
#define MAX_CMD_SIZE 1000
#define MAX_USERNAME_SIZE 100
#define MAX_CHATNAME_SIZE 200

struct client_struct{
	int cfd;
	struct sockaddr_in caddr;
};

SSL_CTX* ctx;
int n_threads = 0;
int chat_creator[MAX_CHATS+1];
int username_length[MAX_THREADS+10];
char username[MAX_THREADS+10][MAX_USERNAME_SIZE];
SSL* ssl[MAX_THREADS+10];
char chat_name[MAX_CHATS+1][MAX_CHATNAME_SIZE];
int chat_name_length[MAX_CHATS+1];
fd_set chat_fd_set[MAX_CHATS+1];
pthread_mutex_t n_threads_mutex = PTHREAD_MUTEX_INITIALIZER,
				username_mutex = PTHREAD_MUTEX_INITIALIZER,
				cfd_write_mutex[MAX_THREADS+10],
				chat_fd_set_mutex[MAX_CHATS+1];

// safely check if fd is in chat
int fd_isset(int fd, int chat_id){
	pthread_mutex_lock(&chat_fd_set_mutex[chat_id]);
	int ret = FD_ISSET(fd, &chat_fd_set[chat_id]);
	pthread_mutex_unlock(&chat_fd_set_mutex[chat_id]);
	return ret;
}

// safely check if chat exists (actually check who is the creator)
int chat_exists(int chat_id){
	pthread_mutex_lock(&chat_fd_set_mutex[chat_id]);
	int ret = chat_creator[chat_id];
	pthread_mutex_unlock(&chat_fd_set_mutex[chat_id]);
	return ret;
}

int _write(int fd, char *buf, int len){
	int l = len;
	if(ssl[fd] == NULL) return 0;
	while (len > 0) {
		int i = SSL_write(ssl[fd], buf, len);
		if(i == 0) return 0;
		len -= i;
		buf += i;
	}
	return l;
}

int _read(int fd, char *buf, int bufsize){
	static __thread int last_size = 0;
	static __thread char last_buf[MAX_CMD_SIZE];
	
	// copy fragment from last _read
	memcpy(buf, last_buf, last_size);
	int l = last_size;
	bufsize -= last_size;
	
	// search for end of message
	for(int i=0; i<l; i++) if(buf[i]=='\0'){
		// remember remaining part for next _read execution
		memcpy(last_buf, buf+i+1, l-i-1);
		last_size = l-i-1;
		return i+1;
	}
	do {
		int i = SSL_read(ssl[fd], buf+l, bufsize);
		if(i <= 0) return 0;
		bufsize -= i;
		l += i;

		// search for end of message
		for(int j=l-i; j<l; j++) if(buf[j]=='\0'){
			// remember remaining part for next _read execution
			memcpy(last_buf, buf+j+1, l-j-1);
			last_size = l-j-1;
			return j+1;
		}
	} while (bufsize>0);
	return l;
}

// change small number to string
int int_to_text(int n, char*text){
	if(n == 0){
		text[0]='0';
		return 1;
	}
	int l = 10000;
	int i = 0;
	while(n/l == 0) l/=10;
	while(l>0){
		text[i++] = n/l + '0';
		l/=10;
	}
	return i;
}

// send info about users in chat
void send_users_in_chat(int fd, int chat_id){
	char chat_id_text[5];
	int l = int_to_text(chat_id, chat_id_text);
	
	// usernames should not change during that
	pthread_mutex_lock(&username_mutex);
	pthread_mutex_lock(&cfd_write_mutex[fd]);

	for(int i=0; i<MAX_THREADS+10; i++){
		// if user in chat
		if(i != fd && fd_isset(i, chat_id)){
			_write(fd, "U", 1);
			_write(fd, chat_id_text, l);
			_write(fd, " ", 1);
			_write(fd, username[i], username_length[i]);
		}
	}
	pthread_mutex_unlock(&cfd_write_mutex[fd]);
	pthread_mutex_unlock(&username_mutex);
}

// send info to the creator of the chat (with chat_id)
void send_chat_create(int fd, int chat_id, char* chat_name, int chat_name_length){
	char chat_id_text[5];
	int l = int_to_text(chat_id, chat_id_text);

	pthread_mutex_lock(&cfd_write_mutex[fd]);
	_write(fd, "C", 1);
	_write(fd, chat_id_text, l);
	_write(fd, " ", 1);
	_write(fd, chat_name, chat_name_length);
	pthread_mutex_unlock(&cfd_write_mutex[fd]);
}

// send info that new chat has been created
void send_chat_new(int exclude_fd, int chat_id, char* chat_name, int chat_name_length){
	char chat_id_text[5];
	int l = int_to_text(chat_id, chat_id_text);

	for(int i=0; i<=MAX_THREADS+10; i++){
		// all clients except the creator
		if(i != exclude_fd && fd_isset(i, 0)){
			pthread_mutex_lock(&cfd_write_mutex[i]);
			_write(i, "L", 1);
			_write(i, chat_id_text, l);
			_write(i, " ", 1);
			_write(i, chat_name, chat_name_length);
			pthread_mutex_unlock(&cfd_write_mutex[i]);
		}
	}
}

// send info that chat has been deleted
void send_chat_destroy(int chat_id){
	char chat_id_text[5];
	int l = int_to_text(chat_id, chat_id_text);

	for(int i=0; i<=MAX_THREADS+10; i++){
		// to all clients in main chat
		if(fd_isset(i, 0)){
			pthread_mutex_lock(&cfd_write_mutex[i]);
			_write(i, "D", 1);
			_write(i, chat_id_text, l);
			_write(i, "\0", 1);
			pthread_mutex_unlock(&cfd_write_mutex[i]);
		}
	}
}

// send info about user joining chat
void send_user_join(int chat_id, char* name, int name_length){
	char chat_id_text[5];
	int l = int_to_text(chat_id, chat_id_text);

	for(int i=0; i<=MAX_THREADS+10; i++){
		// if client in chat
		if(fd_isset(i, chat_id)){
			pthread_mutex_lock(&cfd_write_mutex[i]);
			_write(i, "J", 1);
			_write(i, chat_id_text, l);
			_write(i, " ", 1);
			_write(i, name, name_length);
			pthread_mutex_unlock(&cfd_write_mutex[i]);
		}
	}
}

// send info about user leaving chat
void send_user_leave(int chat_id, char* name, int name_length){
	for(int i=0; i<=MAX_THREADS+10; i++){
		// if client in chat
		if(fd_isset(i, chat_id)){
			char chat_id_text[5];
			int l = int_to_text(chat_id, chat_id_text);

			pthread_mutex_lock(&cfd_write_mutex[i]);
			_write(i, "E", 1);
			_write(i, chat_id_text, l);
			_write(i, " ", 1);
			_write(i, name, name_length);
			pthread_mutex_unlock(&cfd_write_mutex[i]);
		}
	}
}

// send message to clients in chat
void send_message(int chat_id, char* name, int name_length, char* buf, int bufsize){
	for(int i=0; i<=MAX_THREADS+10; i++){
		// if client in chat
		if(fd_isset(i, chat_id)){
			char chat_id_text[5];
			int l = int_to_text(chat_id, chat_id_text);

			pthread_mutex_lock(&cfd_write_mutex[i]);
			_write(i, "S", 1);
			_write(i, chat_id_text, l);
			_write(i, " ", 1);
			_write(i, name, name_length-1);
			_write(i, "\n", 1);
			_write(i, buf, bufsize);
			pthread_mutex_unlock(&cfd_write_mutex[i]);
		}
	}
}

// send list of chats
void list(int fd){
	for(int i=1; i<MAX_CHATS+1; i++){
		pthread_mutex_lock(&chat_fd_set_mutex[i]);
		// if chat exists
		if(chat_creator[i]){
			char buf[5];
			int l = int_to_text(i, buf);

			pthread_mutex_lock(&cfd_write_mutex[fd]);
			_write(fd, "L", 1);
			_write(fd, buf, l);
			_write(fd, " ", 1);
			_write(fd, chat_name[i], chat_name_length[i]);
			pthread_mutex_unlock(&cfd_write_mutex[fd]);
		}
		pthread_mutex_unlock(&chat_fd_set_mutex[i]);
	}
}

// join client to chat
void join(int chat_id, int fd, char* name, int name_length){
	pthread_mutex_lock(&chat_fd_set_mutex[chat_id]);
	FD_SET(fd, &chat_fd_set[chat_id]);
	pthread_mutex_unlock(&chat_fd_set_mutex[chat_id]);

	// send usernames of users in chat to client
	send_users_in_chat(fd, chat_id);
	//send info about new user in chat
	send_user_join(chat_id, name, name_length);
}

// delete chat
void destroy(int chat_id){
	pthread_mutex_lock(&chat_fd_set_mutex[chat_id]);
	chat_creator[chat_id] = 0;
	FD_ZERO(&chat_fd_set[chat_id]);
	pthread_mutex_unlock(&chat_fd_set_mutex[chat_id]);
	// send info about chat deletion
	send_chat_destroy(chat_id);
}

void leave(int chat_id, int fd, char* name, int name_length){
	// remove client from chat
	pthread_mutex_lock(&chat_fd_set_mutex[chat_id]);
	FD_CLR(fd, &chat_fd_set[chat_id]);
	pthread_mutex_unlock(&chat_fd_set_mutex[chat_id]);

	// send goodbye message from client
	send_user_leave(chat_id, name, name_length);

	// destroy chat when creator leaves
	if(chat_exists(chat_id) == fd) destroy(chat_id);
}

int create(int fd, char* cname, int cname_length){
	int chat_id = -1;
	// find not used chat_id
	for(int i=0; i<MAX_CHATS+1; i++){
		pthread_mutex_lock(&chat_fd_set_mutex[i]);
		// if chat_id not used
		if(!chat_creator[i]){
			chat_id = i;
			// set chat name and creator
			memcpy(chat_name[i], cname, cname_length);
			chat_name_length[i] = cname_length;
			chat_creator[i] = fd;
			pthread_mutex_unlock(&chat_fd_set_mutex[i]);
			break;
		}
		pthread_mutex_unlock(&chat_fd_set_mutex[i]);
	}
	if(chat_id != -1){
		send_chat_create(fd, chat_id, cname, cname_length); // send C to fd
		send_chat_new(fd, chat_id, cname, cname_length); // send L to all except fd
	}
	return chat_id;
}

void cthread_close(struct client_struct* client_info, char* name, int name_length){
	// leave chats if normal disconnect (not because of invalid username)
	if(name_length > 0)
		for(int i=0; i < MAX_CHATS+1; i++)
			if(fd_isset(client_info->cfd, i)) leave(i, client_info->cfd, name, name_length);
	
	// other threads cannot write to freed ssl
	pthread_mutex_lock(&cfd_write_mutex[client_info->cfd]);
	
	SSL_free(ssl[client_info->cfd]);
	// mark ssl as freed (other threads should not write to it)
    ssl[client_info->cfd] = NULL;
	pthread_mutex_unlock(&cfd_write_mutex[client_info->cfd]);

	close(client_info->cfd);
	free(client_info);

	// change threads counter
	pthread_mutex_lock(&n_threads_mutex);
	n_threads--;
	pthread_mutex_unlock(&n_threads_mutex);
}

void* cthread(void* arg){
	char buf[MAX_CMD_SIZE], name[MAX_USERNAME_SIZE];
	int name_length;
	
	struct client_struct* client_info = (struct client_struct*)arg;
	int cfd = client_info->cfd;
	struct sockaddr_in caddr = client_info->caddr;

	printf("new connection from %s:%d\n", inet_ntoa((struct in_addr)caddr.sin_addr), ntohs(caddr.sin_port));

	// initialize SSL connection
    pthread_mutex_lock(&cfd_write_mutex[cfd]);
	ssl[cfd] = SSL_new(ctx);
	SSL_set_fd(ssl[cfd], cfd);
	SSL_accept(ssl[cfd]);
    pthread_mutex_unlock(&cfd_write_mutex[cfd]);

	// read client username
	name_length = _read(cfd, name, sizeof(name));

	// disconnect when invalid username
	if(name_length < 2){
		printf("disconnected from %s:%d\n", inet_ntoa((struct in_addr)caddr.sin_addr), ntohs(caddr.sin_port));
		cthread_close(client_info, name, name_length);
		return 0;
	}

	// insert username into variables accesible from other threads
	pthread_mutex_lock(&username_mutex);
	memcpy(username[cfd], name, name_length);
	username_length[cfd] = name_length;
	pthread_mutex_unlock(&username_mutex);
	
	// join client to main chat
	join(0, cfd, name, name_length);

	// send list of chats to client
	list(cfd);

	while(1){
		// read message from client
		int n = _read(cfd, buf, sizeof(buf));
		if(n == 0){ // disconnected
			printf("disconnected from %s:%d\n", inet_ntoa((struct in_addr)caddr.sin_addr), ntohs(caddr.sin_port));
			cthread_close(client_info, name, name_length);
			return 0;
		}
		if(n == 1) continue; // wrong message

		int chat_id;
		switch(buf[0]){

			case 'S': // send
				// calculate chat_id (number from string)
				chat_id = buf[1]- '0';
				int i = 2;
				while(buf[i] != ' ') chat_id = chat_id*10 + buf[i++] - '0';
				if(chat_id > MAX_CHATS || chat_id < 0) continue;

				// send message if client in chat
				if(fd_isset(cfd, chat_id)) send_message(chat_id, name, name_length, buf+i+1, n-i-1);
				break;

			case 'J': // join
				// calculate chat_id (number from string)
				chat_id = buf[1] - '0';
				for(int i=2; i<n-1; i++) chat_id = chat_id*10 + buf[i] - '0';
				if(chat_id > MAX_CHATS || chat_id < 0) continue;
				
				// join chat if chat exists and client not in chat
				if(!fd_isset(cfd, chat_id) && chat_exists(chat_id)) join(chat_id, cfd, name, name_length);
				break;

			case 'E': // leave
				// calculate chat_id (number from string)
				chat_id = buf[1] - '0';
				for(int i=2; i<n-1; i++) chat_id = chat_id*10 + buf[i] - '0';
				if(chat_id > MAX_CHATS || chat_id <= 0) continue;
				
				// leave chat if client in chat
				if(fd_isset(cfd, chat_id)) leave(chat_id, cfd, name, name_length);
				break;

			case 'C': // create
				chat_id = create(cfd, buf+1, n-1);
				join(chat_id, cfd, name, name_length);
				break;
		}
	}
}

int main(int argc, char **argv){
	socklen_t size;
	int sfd, on=1;
	struct sockaddr_in saddr, caddr;

	// initialize SSL library
    if(!SSL_load_error_strings()){
        printf("error: initializing SSL library\n");
        return 0;
    }
    if(!SSL_library_init()){
        printf("error: initializing SSL library\n");
        return 0;
    }
    ctx = SSL_CTX_new(TLS_server_method());
    if(!SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM)){
        printf("error: initializing SSL library, please generate certificate file (no server.crt file found)\n");
        return 0;
    }
    if(!SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM)){
        printf("error: initializing SSL library, please generate certificate file (no server.key file found)\n");
        return 0;
    }

	chat_name[0][0] = 'm';
	chat_name[0][1] = 'a';
	chat_name[0][2] = 'i';
	chat_name[0][3] = 'n';
	chat_name[0][4] = '\0';
	chat_name_length[0] = 5;
	chat_creator[0] = 1;

	// initialize mutexes and fd_sets
	for(int i=0; i<MAX_CHATS+1; i++) pthread_mutex_init(&chat_fd_set_mutex[i], NULL);
	for(int i=0; i<MAX_THREADS+10; i++) pthread_mutex_init(&cfd_write_mutex[i], NULL);
	for(int i=0; i<MAX_CHATS+1; i++) FD_ZERO(&chat_fd_set[i]);

	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = INADDR_ANY;
	saddr.sin_port = htons(1234);
	sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
	bind(sfd, (struct sockaddr*)&saddr, sizeof(saddr));
	listen(sfd, 10);
	size = sizeof(caddr);

	while(1){
		pthread_t tid;
		struct client_struct* client_info = malloc(sizeof(struct client_struct));
		
		// wait if too many threads already running
		while(n_threads >= MAX_THREADS) sleep(1);

		// accept connection from client
		client_info->cfd = accept(sfd, (struct sockaddr*)&client_info->caddr, &size);
		
		// create thread for client handling
		pthread_create(&tid, NULL, cthread, client_info);
		pthread_detach(tid);
		
		//change threads counter
		pthread_mutex_lock(&n_threads_mutex);
		n_threads++;
		pthread_mutex_unlock(&n_threads_mutex);
	}
}
