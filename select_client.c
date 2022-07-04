#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 1024
#define NAME_SIZE 20
#define PACKET_SIZE 1102
#define UserManageMent_SIZE 38

int PrintUI();
void error_handling(char *message);
void clearInputBuffer();
void getList(int sock);
void disConnect(int sock);
void send_message(int sock);
void send_cmd(int sock);
void getHistory();
void *handle_connection(int sock_num, fd_set *reads);

void *t_PrintUI(void *data);
//void *send_msg(void *arg);
void *recv_msg(void *arg);

char name[NAME_SIZE]="[DEFAULT]";
char msg[BUF_SIZE];
pthread_mutex_t mutx;

int history_count = 0;
char *history_arr[256];

#pragma pack(push,1)
typedef struct
{
	char IP_Address[14];
	char NickName[20];
	int Port;
}UserManageMent;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct
{
	char IP_Address[14];
	int Port;
	char Separator[20];	// “Command”, “Print_Result”, “DisConnect”
	char MyName[20];
	char TargetName[20];
	char buf[1024];
}Packet;
#pragma pack(pop)

int main(int argc, char *argv[])
{
	int fd_max, str_len, fd_num, i, sock;
	char message[BUF_SIZE];
	struct sockaddr_in serv_adr;
	struct timeval timeout;
	fd_set reads, cpy_reads;

	pthread_t snd_thread, rcv_thread, printUI_thread;
	pthread_mutex_init(&mutx, NULL);
	void * thread_return;

	if(argc!=4)
	{
		printf("Usage : %s <IP> <port> <name>\n", argv[0]);
		exit(1);
	}
	
	sprintf(name, "%s", argv[3]);
	sock=socket(PF_INET, SOCK_STREAM, 0);   
	if(sock==-1)
		error_handling("socket() error");
	
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_adr.sin_port=htons(atoi(argv[2]));
	
	if(connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
		error_handling("connect() error!");
	else
		puts("Connected...........");

	UserManageMent *user_packet = malloc(sizeof(user_packet)+UserManageMent_SIZE);
	memset(user_packet,0,sizeof(user_packet)+UserManageMent_SIZE);
	strcpy(user_packet->IP_Address,argv[1]);
	user_packet->Port = serv_adr.sin_port;
	strcpy(user_packet->NickName,name);
	if(write(sock,(char*)user_packet,sizeof(user_packet)+UserManageMent_SIZE) <= 0)
	{
		error_handling("Sending UserInfo Failed!!\n");
	}
	printf("send UserInfo Success\n");
	printf("sock_num : %d\n",sock);
	free(user_packet);

	FD_ZERO(&reads);
	FD_SET(0, &reads);
	FD_SET(sock, &reads);
	fd_max=sock;

	while(1) 
	{
		cpy_reads=reads;
		//timeout.tv_sec=5;
		//timeout.tv_usec=5000;

		if(pthread_create(&printUI_thread,NULL,t_PrintUI,(void*)&sock) !=0 )
		{
			error_handling("PrintUI_Thread create error\n");
			continue;
		}

		if((fd_num=select(fd_max+1, &cpy_reads, 0, 0, NULL))==-1)
		{
			perror("select 함수 에러 내용 : ");
			break;
		}

		if(fd_num==0)
			continue;

		if(FD_ISSET(sock, &reads))
		{
			handle_connection(sock,&reads);
		}
	}
	close(sock);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

void *handle_connection(int sock_num, fd_set *reads)
{
	int sock = sock_num;
	char name_msg[BUF_SIZE];
	int str_len = 0;

	//printf("client readstart!!\n");
	Packet *recv_packet = malloc(sizeof(recv_packet)+PACKET_SIZE);
	memset(recv_packet,0,sizeof(recv_packet)+PACKET_SIZE);
	// 구조체 정보 먼저 받기, 커맨드인지 출력물인지 
	str_len = read(sock,(char*)recv_packet,sizeof(recv_packet)+PACKET_SIZE);
	if(str_len == 0)
	{
		//error_handling("read byte == 0\n");
		FD_CLR(sock, reads);
		close(sock);
		printf("\nclosed client: %d \n",sock);
		exit(1);
	}
	else 
	{
		if(strcmp(recv_packet->Separator,"Command")==0)
		{
			if(strcmp(recv_packet->TargetName,name)==0)
			{
				FILE *fp;
				fp = popen(recv_packet->buf,"r");
				if(fp == NULL)
				{
					perror("popen()실패 또는 없는 리눅스 명령어를 입력하였음.\n");
					return (void*)-1;
				}
				while(fgets(recv_packet->buf,BUF_SIZE,fp))
				{
					//printf("\ninsideof command fgets func\n");
					Packet *send_packet = malloc(sizeof(send_packet)+PACKET_SIZE);
					memset(send_packet,0,sizeof(send_packet)+PACKET_SIZE);
					strcpy(send_packet->Separator,"Print_Result");
					strcpy(send_packet->buf,recv_packet->buf);
					strcpy(send_packet->TargetName,recv_packet->TargetName);
					strcpy(send_packet->MyName,recv_packet->MyName);
					if(write(sock,(char*)send_packet,sizeof(send_packet)+PACKET_SIZE) <= 0)
					{
						close(sock);
						break;
					}
					free(send_packet);
				}
				pclose(fp);
			}
			else
			{
				free(recv_packet);
				return NULL;
			}
			free(recv_packet);
			return NULL;
		}

		else if (strcmp(recv_packet->Separator,"Print_Result")==0)
		{
			//printf("myname : %s\n",recv_packet->MyName);
			if(strcmp(recv_packet->MyName,name)==0)
			{
				printf("\n%s from : %s\n",recv_packet->buf,recv_packet->TargetName);
			}
			else
			{
				free(recv_packet);
				return NULL;
			}
		}

		else if (strcmp(recv_packet->Separator,"Message")==0)
		{
			if (strcmp(recv_packet->TargetName,"ALL")==0)
			{
				printf("\nMessage : %s from : %s \n",recv_packet->buf,recv_packet->MyName);
			}
			else if(strcmp(recv_packet->TargetName,name)==0)
			{
				printf("\n귓속말 Message : %s from : %s \n",recv_packet->buf,recv_packet->MyName);
			}
		}

		else if (strcmp(recv_packet->Separator,"List")==0)
		{
			printf("\nClient List : %s\n",recv_packet->buf);
		}
	}
	free(recv_packet);
	return NULL;
}

void *t_PrintUI(void *arg)
{
	int sock= *((int*)arg);
	int nMenu = 0;

	pthread_mutex_lock(&mutx);
	while((nMenu = PrintUI()) !=0)
	{
		switch(nMenu)
		{
			case 1:
				getList(sock);
				break;
			case 2:
				send_message(sock);
				break;
			case 3:
				send_cmd(sock);
				break;
			case 4:
				getHistory();
				break;
			case 5:
				disConnect(sock);
				break;
			default:
				printf("메뉴의 보기에 있는 숫자 중에서 입력하세요.\n");
				break;
		}
	}
	pthread_mutex_unlock(&mutx);
}


int PrintUI()
{
	int nInput = 0;
	// system("cls");
	printf("\n===================================================\n");
	printf("[1]유저목록보기\t [2] 메세지보내기\t [3]명령어보내기\t [4]명령어기록보기\t [5]연결종료하기\t\n");
	printf("===================================================\n");
	printf("번호를 입력하세요 : ");

	// 사용자가 선택한 메뉴의 값을 반환한다.
	scanf("%d", &nInput);
	//getchar();
	//버퍼에 남은 엔터 제거용
	while (getchar() != '\n'); //scanf_s 버퍼 비우기, 밀림 막음
	if (nInput > 5 || nInput < 1)
	{
		nInput = 6; // 0~2 사이의 메뉴 값이 아니라면 defalut로 보내기
	}
	return nInput;
}

void clearInputBuffer()
{
    // 입력 버퍼에서 문자를 계속 꺼내고 \n를 꺼내면 반복을 중단
    while (getchar() != '\n');
}

void getList(int sock)
{
	char buf[1024];
	Packet *send_packet = malloc(sizeof(send_packet)+PACKET_SIZE);
	memset(send_packet,0,sizeof(send_packet)+PACKET_SIZE);

	printf("===================================================\n");
	printf("접속된 유저들을 보려면 List 를 입력하세요\n");
	scanf("%s",buf);

	if(strcmp(buf,"List")!=0)
	{
		printf("접속 유저를 보려면 List 를 정확하게 입력하세요, 처음 화면으로 돌아갑니다\n");
		free(send_packet);
		return;
	}

	strcpy(send_packet->Separator,buf);
	
	write(sock, (char*)send_packet,sizeof(send_packet)+PACKET_SIZE);
	free(send_packet);
	return;
}

void getHistory()
{
	printf("===================================================\n");
	printf("내가 보낸 명령어 기록을 체크합니다.\n");
	// printf();
	return;
}

void disConnect(int sock)
{
	printf("서버와의 접속이 종료됩니다.\n");
	exit(1);
	//FD_CLR(sock, &reads);
	//close(sock);
	//exit(1);
}

void send_message(int sock)   // send to all
{
	int clnt_sock = sock;
	Packet *send_packet = malloc(sizeof(send_packet)+PACKET_SIZE);
	memset(send_packet,0,sizeof(send_packet)+PACKET_SIZE);
	char name_msg[BUF_SIZE];
	printf("===================================================\n");
	printf("[1] 전체보내기\t [2] 귓속말보내기\t [3] 처음 화면으로 돌아가기\t\n");
	printf("===================================================\n");
	printf("번호를 입력하세요 : ");
	int index;
	scanf("%d",&index);

	strcpy(send_packet->Separator,"Message");
	strcpy(send_packet->MyName,name);

	if (index == 1)
	{
		// 나 자신 제외..?
		printf("===================================================\n");
		printf("전체 보내기 모드입니다.\n");
		printf("===================================================\n");
		printf("보낼 메세지를 입력하세요 : ");
		clearInputBuffer();
		fgets(name_msg,BUF_SIZE,stdin);
		strcpy(send_packet->TargetName,"ALL");
		strcpy(send_packet->buf,name_msg);
		write(sock, (char*)send_packet,sizeof(send_packet)+PACKET_SIZE);
		//printf("Write_Success\n");
	}
	else if (index == 2)
	{
		printf("===================================================\n");
		printf("귓속말 보낼 닉네임을 입력하세요\n");
		char TargetName[20];
		scanf("%s",TargetName);
		if(strcmp(TargetName,name)==0)
		{
			printf("자기 자신한테 귓속말을 보낼 수 없습니다.\n");
		}
		else
		{
			strcpy(send_packet->TargetName,TargetName);
			clearInputBuffer();
			printf("===================================================\n");
			printf("귓속말 보낼 내용을 입력하세요\n");
			fgets(name_msg,BUF_SIZE,stdin);
			strcpy(send_packet->buf,name_msg);
			write(sock, (char*)send_packet,sizeof(send_packet)+PACKET_SIZE);
		}
	}
	else if (index == 3)
	{
		printf("===================================================\n");
		printf("처음 메뉴로 돌아갑니다\n");
	}
	else
	{
		printf("===================================================\n");
		printf("위의 제공된 번호 목록 중에서 선택하세요, 처음 메뉴로 돌아갑니다\n");
	}

	free(send_packet);
	return;
}
void send_cmd(int sock)   // send to all
{
	printf("===================================================\n");
	printf("명령어 보내기 모드입니다.\n");
	printf("===================================================\n");
	printf("명령어를 보낼 타겟 클라이언트 닉네임을 입력하세요 : ");

	Packet *send_packet = malloc(sizeof(send_packet)+PACKET_SIZE);
	memset(send_packet,0,sizeof(send_packet)+PACKET_SIZE);

	char TargetName[20];
	scanf("%s",TargetName);
	if(strcmp(TargetName,name)==0)
	{
		printf("자기 자신한테 귓속말을 보낼 수 없습니다.\n");
	}
	else
	{
		printf("===================================================\n");
		printf("상대방에게 보낼 명령어를 입력하세요 : ");
		char buf[20];
		scanf("%s",buf);
		strcpy(send_packet->Separator,"Command");
		strcpy(send_packet->MyName,name);
		//printf("when send cmd check name :%s",send_packet->MyName);
		strcpy(send_packet->TargetName,TargetName);
		strcpy(send_packet->buf,buf);
		write(sock, (char*)send_packet,sizeof(send_packet)+PACKET_SIZE);
		printf("명령어 전송이 완료되었습니다.\n");
	}
	free(send_packet);
	return;
}

