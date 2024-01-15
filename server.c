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
#define MAX_CHATNAME_SIZE 200

struct client_struct{
	int cfd;
	struct sockaddr_in caddr;
};

static volatile int n_threads = 0, chat_exists[MAX_CHATS+1];
static volatile char chat_name[MAX_CHATS+1][MAX_CHATNAME_SIZE];
static volatile int chat_name_length[MAX_CHATS+1];
static volatile fd_set chat_fd_set[MAX_CHATS+1];
static pthread_mutex_t n_threads_mutex = PTHREAD_MUTEX_INITIALIZER,
					   create_chat_mutex = PTHREAD_MUTEX_INITIALIZER,
					   cfd_write_mutex[MAX_THREADS+10],
					   chat_fd_set_mutex[MAX_CHATS+1];

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

void send_user_join(int chat_id, char* name, int name_length){
	for(int i=0; i<=MAX_THREADS+10; i++){
		if(FD_ISSET(i, &chat_fd_set[chat_id])){
			char chat_id_text[5];
			int l = int_to_text(chat_id, chat_id_text);
			
			pthread_mutex_lock(&cfd_write_mutex[i]);
			_write(i, "J", 1);
			_write(i, chat_id_text, l);
			_write(i, " ", 1);
			_write(i, name, name_length);
			pthread_mutex_unlock(&cfd_write_mutex[i]);
		}
	}
}
void send_user_leave(int chat_id, char* name, int name_length){
	for(int i=0; i<=MAX_THREADS+10; i++){
		if(FD_ISSET(i, &chat_fd_set[chat_id])){
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
void send_message(int chat_id, char* name, int name_length, char* buf, int bufsize){
	for(int i=0; i<=MAX_THREADS+10; i++){
		if(FD_ISSET(i, &chat_fd_set[chat_id])){
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
void list(int fd){
	pthread_mutex_lock(&create_chat_mutex);
	for(int i=0; i<MAX_CHATS; i++){
		if(chat_exists[i]){
			char buf[5];
			int l = int_to_text(i, buf);
			
			pthread_mutex_lock(&cfd_write_mutex[fd]);
			_write(fd, "L", 1);
			_write(fd, buf, l);
			_write(fd, " ", 1);
			_write(fd, (char*) chat_name[i], chat_name_length[i]);
			pthread_mutex_unlock(&cfd_write_mutex[fd]);
		}
	}
	pthread_mutex_unlock(&create_chat_mutex);
}

void join(int chat_id, int fd, char* name, int name_length){
	pthread_mutex_lock(&chat_fd_set_mutex[chat_id]);
	FD_SET(fd, &chat_fd_set[chat_id]);
	pthread_mutex_unlock(&chat_fd_set_mutex[chat_id]);
	send_user_join(chat_id, name, name_length);
}

void leave(int chat_id, int fd, char* name, int name_length){
	pthread_mutex_lock(&chat_fd_set_mutex[chat_id]);
	FD_CLR(fd, &chat_fd_set[chat_id]);
	pthread_mutex_unlock(&chat_fd_set_mutex[chat_id]);
	send_user_leave(chat_id, name, name_length);
}

int create(int fd, char* cname, int cname_length){
	int i;
	pthread_mutex_lock(&create_chat_mutex); 
	for(i=0; i<MAX_CHATS; i++){
		if(!chat_exists[i]){
			memcpy((char*) chat_name[i], cname, cname_length);
			chat_name_length[i] = cname_length;
			chat_exists[i] = 1;
			break;
		}
	}
	pthread_mutex_unlock(&create_chat_mutex);
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
		printf("disconnected from %s:%d\n", inet_ntoa((struct in_addr)caddr.sin_addr), ntohs(caddr.sin_port));
		cthread_close(client_info, name, name_length);
		return 0;
	}
	join(0, cfd, name, name_length);
	while(1){
		int n = _read(cfd, buf, sizeof(buf));
		if(n == 0){
			printf("disconnected from %s:%d\n", inet_ntoa((struct in_addr)caddr.sin_addr), ntohs(caddr.sin_port));
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
				if(FD_ISSET(cfd, &chat_fd_set[chat_id])) send_message(chat_id, name, name_length, buf+i+1, n-i-1);
				// Todo?: else send error message
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

	chat_name[0][0]='m';
	chat_name[0][1]='a';
	chat_name[0][2]='i';
	chat_name[0][3]='n';
	chat_name[0][4]='\0';
	chat_name_length[0] = 5;

	for(int i=0; i<MAX_CHATS+1; i++) pthread_mutex_init(&chat_fd_set_mutex[i], NULL);
	for(int i=0; i<MAX_THREADS+10; i++) pthread_mutex_init(&cfd_write_mutex[i], NULL);

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
