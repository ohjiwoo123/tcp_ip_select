#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 1024
#define NAME_SIZE 20

void * send_msg(void * arg);
void * recv_msg(void * arg);
void error_handling(char * msg);

char name[NAME_SIZE]="[DEFAULT]";
char msg[BUF_SIZE];

#pragma pack(push, 1)    // 1바이트 크기로 정렬
typedef struct
{
	char IP_Address[14];
	int Port;
	char Name[20];
	char cmdOrResult[20];
        char buf[BUF_SIZE];
}
Packet;
#pragma pack(pop)

int main(int argc, char *argv[])
{
	//Declare and initialize a client socket address structure
	struct sockaddr_in client_addr;
	bzero(&client_addr, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = htons(INADDR_ANY);
	client_addr.sin_port = htons(0);

	//Creates a socket and returns the socket descriptor if successful
	int client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(client_socket_fd < 0)
	{
	  perror("Create Socket Failed:");
	  exit(1);
	}

	//Binding the client socket and the client socket address structure is not required
	if(-1 == (bind(client_socket_fd, (struct sockaddr*)&client_addr, sizeof(client_addr))))
	{
	  perror("Client Bind Failed:");
	  exit(1);
	}

	int sock;
	struct sockaddr_in serv_addr;
	pthread_t snd_thread, rcv_thread;
	void * thread_return;
	if(argc!=3)
	{
		printf("Usage : %s <IP> <port> <name>\n", argv[0]);
		exit(1);
	}

	sprintf(name, "[%s]", argv[3]);
	sock=socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_addr.sin_port=htons(atoi(argv[2]));

	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
		error_handling("connect() error");

	pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);
	pthread_join(snd_thread, &thread_return);
	pthread_join(rcv_thread, &thread_return);
	close(sock);
	return 0;
}

void * send_msg(void * arg)   // send thread main
{
	int sock=*((int*)arg);
	char name_msg[BUF_SIZE];
	while(1)
	{
		fgets(name_msg, BUF_SIZE, stdin);
		if(!strcmp(name_msg,"q\n")||!strcmp(name_msg,"Q\n"))
		{
			close(sock);
			exit(0);
		}
		//sprintf(name_msg,"%s %s", name, msg);

		Packet *send_packet = malloc(sizeof(send_packet));
		if (send_packet == NULL)
		{
			printf("send_packet Pointer is NULL\n");
			return (void*)-1;
		}
		strcpy(send_packet->IP_Address,);
		send_packet->Port = ;
		strcpy(send_packet->cmdOrResult,"Command");
		strcpy(send_packet->Name,name);
		//write(sock, name_msg, strlen(name_msg));
		write(sock, (char*)send_packet,sizeof(send_packet));
		free(packet);
	}
	return NULL;
}

void * recv_msg(void * arg)   // read thread main
{
	int sock=*((int*)arg);
	char name_msg[BUF_SIZE];
	int str_len;
	while(1)
	{
		packet *recv_packet = malloc(sizeof(cmdOrResult));
		// 구조체 정보 먼저 받기, 커맨드인지 출력물인지 
		if(read(sock,(char*)st2,sizeof(st2))<=0)
		{
			printf("read struct Failed!\n");
                        return (void*)-1;
		}
		if(strcmp(st2->buf,"Command")==0)
		{
			str_len=read(sock, name_msg, BUF_SIZE-1);
	                if(str_len==-1)
        	                return (void*)-1;
		        name_msg[str_len]=0;
			FILE *fp;
			fp = popen(name_msg,"r");
			if(fp == NULL)
			{
				perror("popen()실패 또는 없는 리눅스 명령어를 입력하였음.\n");
				return (void*)-1;
			}
			while(fgets(name_msg,BUF_SIZE,fp))
			{
				cmdOrResult *st3 = malloc(sizeof(cmdOrResult));
				strcpy(st3->buf,"Print_Result");
				//printf("%s\n",buf);
				if(write(sock,name_msg,sizeof(name_msg)) <= 0)
				{
					close(sock);
					break;
				}
				if(write(sock,(char*)st3,sizeof(st3)) <= 0)
				{
					close(sock);
					break;
				}
				free(st3);
			}
			pclose(fp);
		}

		else if (strcmp(st2->buf,"Print_Result")==0)
		{
			str_len=read(sock, name_msg, NAME_SIZE+BUF_SIZE-1);
			if(str_len==-1)
				return (void*)-1;
			name_msg[str_len]=0;
			fputs(name_msg, stdout);
		}
		free(st2);
	}
	return NULL;
}

void error_handling(char *msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
