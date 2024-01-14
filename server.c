#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<string.h>
#include<netdb.h>
#include<stdlib.h>
#include<stdio.h>
#include<sys/wait.h>
#include<pthread.h>
#define MAX_THREADS 5
#define MAX_CHATS 2
#define MAX_CMD_SIZE 300
#define MAX_USERNAME_SIZE 30

struct client_struct{
	int cfd;
	struct sockaddr_in caddr;
};

static volatile int n_threads = 0, chat_exists[MAX_CHATS+1];
static volatile char* chat_name[MAX_CHATS+1];
static volatile int chat_name_length[MAX_CHATS+1];
static volatile fd_set chat_fd_set[MAX_THREADS+1];
static pthread_mutex_t n_threads_mutex = PTHREAD_MUTEX_INITIALIZER,
					   create_mutex = PTHREAD_MUTEX_INITIALIZER;

int _write(int fd, char *buf, int len){
	int l = len;
	while (len > 0) {
		int i = write(fd, buf, len);
		if(i == 0) return 0;
		len -= i;
		buf += i;
	}
	return l;
}

int _read(int fd, char *buf, int bufsize){
	static __thread int last_size = 0;
	static __thread char last_buf[MAX_CMD_SIZE];
	memcpy(buf, last_buf, last_size);
	int l = last_size;
	bufsize -= last_size;
	for(int i=0; i<l; i++) if(buf[i]=='\0'){
		memcpy(last_buf, buf+i+1, l-i-1);
		last_size = l-i-1;
		return i+1;
	}
	do {
		int i = read(fd, buf+l, bufsize);
		if(i == 0) return 0;
		bufsize -= i;
		l += i;
		for(int j=l-i; j<l; j++) if(buf[j]=='\0'){
			memcpy(last_buf, buf+j+1, l-j-1);
			last_size = l-j-1;
			return j+1;
		}
	} while (bufsize>0);
	return l;
}

void send_user_join(int chat_id, char* name, int name_length){
}
void send_user_leave(int chat_id, char* name, int name_length){
}
void send_message(int chat_id, char* name, int name_length, char* buf, int bufsize){
	for(int i=0; i<=MAX_THREADS; i++){
		if(FD_ISSET(i, &chat_fd_set[chat_id])){
			//_write(i, chat_id, 1); // Todo: change chat_id to string
			_write(i, ": ", 2);
			_write(i, name, name_length);
			_write(i, " wrote ", 7);
			_write(i, buf, bufsize);
			_write(i, "\n", 1);
		}
	}
}
void list(int fd){
	for(int i=0; i<MAX_CHATS; i++){
		if(chat_exists[i]);// send chat name and id to client
	}
}

void join(int chat_id, int fd, char* name, int name_length){
	FD_SET(fd, &chat_fd_set[chat_id]);
	send_user_join(chat_id, name, name_length);
}

void leave(int chat_id, int fd, char* name, int name_length){
	FD_CLR(fd, &chat_fd_set[chat_id]);
	send_user_leave(chat_id, name, name_length);
}

int create(int fd, char* cname, int cname_length){
	int i;
	pthread_mutex_lock(&create_mutex); 
	for(i=0; i<MAX_CHATS; i++){
		if(!chat_exists[i]){
			chat_name[i] = cname;
			chat_name_length[i] = cname_length;
			chat_exists[i] = 1;
			break;
		}
	}
	pthread_mutex_unlock(&create_mutex);
	return i;
}

void cthread_close(struct client_struct* client_info, char* name, int name_length){
	if(name_length>0)
		for(int i=0; i < MAX_CHATS; i++)
			if(FD_ISSET(client_info->cfd, &chat_fd_set[i])) leave(i, client_info->cfd, name, name_length);
	close(client_info->cfd);
	free(client_info);
	
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
	name_length = _read(cfd, name, sizeof(name));
	if(name_length == 0){
		//LOG
		cthread_close(client_info, name, name_length);
		return 0;
	}
	join(0, cfd, name, name_length);
	while(1){
		int n = _read(cfd, buf, sizeof(buf));
		if(n == 0){
			//LOG
			cthread_close(client_info, name, name_length);
			return 0;
		}
		int chat_id;
		switch(buf[0]){
			case 'L':
				list(cfd);
				break;
			case 'S':
				chat_id = buf[1]- '0';
				int i = 2;
				while(buf[i] != ' ') chat_id = chat_id*10 + buf[i++] - '0';
				send_message(chat_id, name, name_length, buf+i+1, n-i-1);
				break;
			case 'J':
				chat_id = buf[1] - '0';
				for(int i=2; i<n-1; i++) chat_id = chat_id*10 + buf[i] - '0';
				join(chat_id, cfd, name, name_length);
				break;
			case 'E':
				chat_id = buf[1] - '0';
				for(int i=2; i<n-1; i++) chat_id = chat_id*10 + buf[i] - '0';
				leave(chat_id, cfd, name, name_length);
				break;
			case 'C':
				chat_id = create(cfd, buf+1, n-1);
				break;
		}
	}
	cthread_close(client_info, name, name_length);
	return 0;
}

int main(int argc, char **argv){
	socklen_t size;
	int sfd, on=1;
	struct sockaddr_in saddr, caddr;

	chat_name[0] = "global";
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
		while(n_threads >= MAX_THREADS) sleep(1);
		client_info->cfd = accept(sfd, (struct sockaddr*)&client_info->caddr, &size);
		pthread_create(&tid, NULL, cthread, client_info);
		pthread_detach(tid);	
		pthread_mutex_lock(&n_threads_mutex);
		n_threads++;
		pthread_mutex_unlock(&n_threads_mutex);
	}
}
