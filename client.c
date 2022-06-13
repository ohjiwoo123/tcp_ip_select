#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 1024
#define NAME_SIZE 20
#define PACKET_SIZE 1082

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
}Packet;
#pragma pack(pop)

int main(int argc, char *argv[])
{
	//printf("size of Packet : %d\n",sizeof(Packet));
	int sock;
	struct sockaddr_in serv_addr;
	pthread_t snd_thread, rcv_thread;
	void * thread_return;
	if(argc!=4)
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
		Packet *send_packet = malloc(sizeof(send_packet)+PACKET_SIZE);
		if (send_packet == NULL)
		{
			error_handling("send_packet Pointer is NULL\n");
			return (void*)-1;
		}
		memset(send_packet,0,sizeof(send_packet)+PACKET_SIZE);

		fgets(name_msg, BUF_SIZE, stdin);
		if(!strcmp(send_packet->buf,"q\n")||!strcmp(send_packet->buf,"Q\n"))
		{
			close(sock);
			exit(0);
		}
		//sprintf(name_msg,"%s %s", name, msg);

		//strcpy(send_packet->IP_Address,);
		//send_packet->Port = ;
		strcpy(send_packet->buf,name_msg);
		strcpy(send_packet->cmdOrResult,"Command");
		strcpy(send_packet->Name,name);
		//write(sock, name_msg, strlen(name_msg));
		printf("sizeof packet : %d before free\n",sizeof(send_packet));
		printf("buf : %s, cmdorresult : %s, name : %s\n",send_packet->buf,send_packet->cmdOrResult,send_packet->Name);
		write(sock, (char*)send_packet,sizeof(send_packet)+PACKET_SIZE);

		free(send_packet);
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
		printf("readstart!!\n");
		Packet *recv_packet = malloc(sizeof(recv_packet)+PACKET_SIZE);
		memset(recv_packet,0,sizeof(recv_packet)+PACKET_SIZE);
		// 구조체 정보 먼저 받기, 커맨드인지 출력물인지 
		if(read(sock,(char*)recv_packet,sizeof(recv_packet)+PACKET_SIZE)<=0)
		{
			error_handling("read struct Failed!\n");
                        return (void*)-1;
		}
		printf("readsuccess!!\n");
		if(strcmp(recv_packet->cmdOrResult,"Command")==0)
		{
			printf("inside Command\n");
			printf("Command is %s\n",recv_packet->buf);
			//str_len=read(sock, name_msg, BUF_SIZE-1);
	                //if(str_len==-1)
        	        //        return (void*)-1;
		        //name_msg[str_len]=0;
			FILE *fp;
			fp = popen(recv_packet->buf,"r");
			if(fp == NULL)
			{
				perror("popen()실패 또는 없는 리눅스 명령어를 입력하였음.\n");
				return (void*)-1;
			}
			while(fgets(recv_packet->buf,BUF_SIZE,fp))
			{
				printf("insideof fgets func\n");
				Packet *send_packet2 = malloc(sizeof(send_packet2)+PACKET_SIZE);
				memset(send_packet2,0,sizeof(send_packet2)+PACKET_SIZE);
				strcpy(send_packet2->cmdOrResult,"Print_Result");
				strcpy(send_packet2->buf,recv_packet->buf);
				if(write(sock,(char*)send_packet2,sizeof(send_packet2)+PACKET_SIZE) <= 0)
				{
					close(sock);
					break;
				}
				free(send_packet2);
			}
			pclose(fp);
		}

		else if (strcmp(recv_packet->cmdOrResult,"Print_Result")==0)
		{
			//str_len=read(sock, name_msg, NAME_SIZE+BUF_SIZE-1);
			//if(str_len==-1)
			//	return (void*)-1;
			//name_msg[str_len]=0;
			//fputs(name_msg, stdout);
			fputs(recv_packet->buf,stdout);
			printf("%s\n",recv_packet->Name);
		}
		free(recv_packet);
	}
	return NULL;
}

void error_handling(char *msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
