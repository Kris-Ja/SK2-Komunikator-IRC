#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<string.h>
#include<netdb.h>
#include<stdlib.h>
#include<stdio.h>
#include<sys/wait.h>
#include<pthread.h>

#define MAX_THREADS 30
#define MAX_CHATS 30
#define MAX_CMD_SIZE 1000
#define MAX_USERNAME_SIZE 100
#define MAX_CHATNAME_SIZE 200

struct client_struct{
	int cfd;
	struct sockaddr_in caddr;
};

int n_threads = 0;
int chat_creator[MAX_CHATS+1];
int username_length[MAX_THREADS+10];
char username[MAX_THREADS+10][MAX_USERNAME_SIZE];
char chat_name[MAX_CHATS+1][MAX_CHATNAME_SIZE];
int chat_name_length[MAX_CHATS+1];
fd_set chat_fd_set[MAX_CHATS+1];
pthread_mutex_t n_threads_mutex = PTHREAD_MUTEX_INITIALIZER,
					   username_mutex = PTHREAD_MUTEX_INITIALIZER,
					   cfd_write_mutex[MAX_THREADS+10],
					   chat_fd_set_mutex[MAX_CHATS+1];

int fd_isset(int fd, int chat_id){
	pthread_mutex_lock(&chat_fd_set_mutex[chat_id]);
	int ret = FD_ISSET(fd, &chat_fd_set[chat_id]);
	pthread_mutex_unlock(&chat_fd_set_mutex[chat_id]);
	return ret;
}

int chat_exists(int chat_id){
	pthread_mutex_lock(&chat_fd_set_mutex[chat_id]);
	int ret = chat_creator[chat_id];
	pthread_mutex_unlock(&chat_fd_set_mutex[chat_id]);
	return ret;
}

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

void send_users_in_chat(int fd, int chat_id){
	char chat_id_text[5];
	int l = int_to_text(chat_id, chat_id_text);
	
	pthread_mutex_lock(&username_mutex);
	pthread_mutex_lock(&cfd_write_mutex[fd]);

	for(int i=0; i<MAX_THREADS+10; i++){
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

void send_chat_new(int exclude_fd, int chat_id, char* chat_name, int chat_name_length){
	char chat_id_text[5];
	int l = int_to_text(chat_id, chat_id_text);

	for(int i=0; i<=MAX_THREADS+10; i++){
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

void send_chat_destroy(int chat_id){
	char chat_id_text[5];
	int l = int_to_text(chat_id, chat_id_text);

	for(int i=0; i<=MAX_THREADS+10; i++){
		if(fd_isset(i, 0)){
			pthread_mutex_lock(&cfd_write_mutex[i]);
			_write(i, "D", 1);
			_write(i, chat_id_text, l);
			_write(i, "\0", 1);
			pthread_mutex_unlock(&cfd_write_mutex[i]);
		}
	}
}

void send_user_join(int chat_id, char* name, int name_length){
	char chat_id_text[5];
	int l = int_to_text(chat_id, chat_id_text);
			
	for(int i=0; i<=MAX_THREADS+10; i++){
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

void send_user_leave(int chat_id, char* name, int name_length){
	for(int i=0; i<=MAX_THREADS+10; i++){
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

void send_message(int chat_id, char* name, int name_length, char* buf, int bufsize){
	for(int i=0; i<=MAX_THREADS+10; i++){
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

void list(int fd){
	for(int i=0; i<MAX_CHATS+1; i++){
		pthread_mutex_lock(&chat_fd_set_mutex[i]);
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

void join(int chat_id, int fd, char* name, int name_length){
	pthread_mutex_lock(&chat_fd_set_mutex[chat_id]);
	FD_SET(fd, &chat_fd_set[chat_id]);
	pthread_mutex_unlock(&chat_fd_set_mutex[chat_id]);
	
	send_users_in_chat(fd, chat_id);
	send_user_join(chat_id, name, name_length);
}

void destroy(int chat_id){
	pthread_mutex_lock(&chat_fd_set_mutex[chat_id]);
	chat_creator[chat_id] = 0;
	FD_ZERO(&chat_fd_set[chat_id]);
	pthread_mutex_unlock(&chat_fd_set_mutex[chat_id]);
	send_chat_destroy(chat_id);	
}

void leave(int chat_id, int fd, char* name, int name_length){
	pthread_mutex_lock(&chat_fd_set_mutex[chat_id]);
	FD_CLR(fd, &chat_fd_set[chat_id]);
	pthread_mutex_unlock(&chat_fd_set_mutex[chat_id]);

	send_user_leave(chat_id, name, name_length);

	// destroy chat when creator leaves
	if(chat_exists(chat_id) == fd) destroy(chat_id); 
}

int create(int fd, char* cname, int cname_length){
	int n = -1;
	for(int i=0; i<MAX_CHATS+1; i++){
		pthread_mutex_lock(&chat_fd_set_mutex[i]);
		if(!chat_creator[i]){
			n = i;
			memcpy(chat_name[i], cname, cname_length);
			chat_name_length[i] = cname_length;
			chat_creator[i] = fd;
			pthread_mutex_unlock(&chat_fd_set_mutex[i]);
			break;
		}
		pthread_mutex_unlock(&chat_fd_set_mutex[i]);
	}
	if(n != -1){
		send_chat_create(fd, n, cname, cname_length); // send C to fd
		send_chat_new(fd, n, cname, cname_length); // send L to all except fd
	}
	return n;
}

void cthread_close(struct client_struct* client_info, char* name, int name_length){
	if(name_length>0)
		for(int i=0; i < MAX_CHATS+1; i++)
			if(fd_isset(client_info->cfd, i)) leave(i, client_info->cfd, name, name_length);
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
	if(name_length < 2){
		printf("disconnected from %s:%d\n", inet_ntoa((struct in_addr)caddr.sin_addr), ntohs(caddr.sin_port));
		cthread_close(client_info, name, name_length);
		return 0;
	}

	pthread_mutex_lock(&username_mutex);
	memcpy(username[cfd], name, name_length);
	username_length[cfd] = name_length;
	pthread_mutex_unlock(&username_mutex);

	join(0, cfd, name, name_length);
	while(1){
		int n = _read(cfd, buf, sizeof(buf));
		if(n == 0){
			printf("disconnected from %s:%d\n", inet_ntoa((struct in_addr)caddr.sin_addr), ntohs(caddr.sin_port));
			cthread_close(client_info, name, name_length);
			return 0;
		}
		if(n == 1) continue;
		int chat_id;
		switch(buf[0]){
			case 'L':
				list(cfd);
				break;
			case 'S':
				chat_id = buf[1]- '0';
				int i = 2;
				while(buf[i] != ' ') chat_id = chat_id*10 + buf[i++] - '0';
				if(chat_id > MAX_CHATS) continue;
				if(fd_isset(cfd, chat_id)) send_message(chat_id, name, name_length, buf+i+1, n-i-1);
				break;
			case 'J':
				chat_id = buf[1] - '0';
				for(int i=2; i<n-1; i++) chat_id = chat_id*10 + buf[i] - '0';
				if(chat_id > MAX_CHATS) continue;
				if(!fd_isset(cfd, chat_id) && chat_exists(chat_id)) join(chat_id, cfd, name, name_length);
				break;
			case 'E':
				chat_id = buf[1] - '0';
				for(int i=2; i<n-1; i++) chat_id = chat_id*10 + buf[i] - '0';
				if(chat_id > MAX_CHATS) continue;
				if(fd_isset(cfd, chat_id)) leave(chat_id, cfd, name, name_length);
				break;
			case 'C':
				chat_id = create(cfd, buf+1, n-1);
				join(chat_id, cfd, name, name_length);
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

	chat_name[0][0] = 'm';
	chat_name[0][1] = 'a';
	chat_name[0][2] = 'i';
	chat_name[0][3] = 'n';
	chat_name[0][4] = '\0';
	chat_name_length[0] = 5;
	chat_creator[0] = 1;

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
		while(n_threads >= MAX_THREADS) sleep(1);
		client_info->cfd = accept(sfd, (struct sockaddr*)&client_info->caddr, &size);
		pthread_create(&tid, NULL, cthread, client_info);
		pthread_detach(tid);	
		pthread_mutex_lock(&n_threads_mutex);
		n_threads++;
		pthread_mutex_unlock(&n_threads_mutex);
	}
}
