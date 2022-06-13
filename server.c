#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 100
#define MAX_CLNT 256
#define PACKET_SIZE 1082

void * handle_clnt(void * arg);
void error_handling(char * msg);
void *t_PrintUI(void *data);

int PrintUI();
void getHistory();
void getMenu();
void disConnect();
void getList();

int clnt_cnt=0;
int clnt_socks[MAX_CLNT];

pthread_mutex_t mutx;

typedef struct socket_info
{
	char IP_Address[14];
	int Port;
	int sock_Num;
}socket_info;
socket_info socket_info_array[5];

#pragma pack(push,1)
typedef struct
{
	char IP_Address[14];
	int Port;
	char Name[20];
	char cmdOrResult[20];
	char buf[BUF_SIZE];
}Packet;
#pragma pack(pop)

void send_msg(char * msg, int len, int sock_num, Packet *p);

int history_count_C1 = 0;
int history_count_C2 = 0;

char *history_arr_C1[50];
char *history_arr_C2[50];

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_sz;
	pthread_t t_id;
	pthread_t thread_PrintUI;

	if(argc!=2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	pthread_mutex_init(&mutx, NULL);
	serv_sock=socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_adr.sin_port=htons(atoi(argv[1]));

	if(bind(serv_sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr))==-1)
		error_handling("bind() error\n");
	if(listen(serv_sock, 5)==-1)
		error_handling("listen() error\n");

	while(1)
	{
		if(pthread_create(&thread_PrintUI,NULL,t_PrintUI,(void*)&clnt_cnt) !=0 )
		{
			error_handling("PrintUI_Thread create error\n");
			continue;
		}
		clnt_adr_sz=sizeof(clnt_adr);
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr,&clnt_adr_sz);

		printf("accept success\n");

		//pthread_mutex_lock(&mutx);
		//printf("mutx lock\n");
		clnt_socks[clnt_cnt]=clnt_sock;
		//pthread_mutex_unlock(&mutx);
		//printf("lock end\n");

		strcpy(socket_info_array[clnt_cnt].IP_Address,inet_ntoa(clnt_adr.sin_addr));
		socket_info_array[clnt_cnt].Port = (int)ntohs(clnt_adr.sin_port);
		socket_info_array[clnt_cnt].sock_Num = clnt_sock;

		if(pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock) != 0)
		{
			error_handling("Handle Thread create error\n");
			continue;
		}
		pthread_detach(t_id);
		printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
		clnt_cnt++;
	}
	close(serv_sock);
	return 0;
}

void * handle_clnt(void * arg)
{
	int clnt_sock=*((int*)arg);
	printf("hi iam handle thread %d\n",clnt_sock);
	int str_len=0, i;
	char msg[BUF_SIZE];

	Packet *recv_packet = malloc(sizeof(recv_packet)+PACKET_SIZE);
	if (recv_packet == NULL)
	{
		error_handling("recv_packet Pointer is NULL\n");
		return (void*)-1;
	}
	memset(recv_packet,0,sizeof(recv_packet)+PACKET_SIZE);

	while((str_len=read(clnt_sock, (char*)recv_packet, sizeof(recv_packet)+PACKET_SIZE))!=0)
	{
		printf("str_len = %d\n",str_len);
		printf("Command : %s\n", recv_packet->cmdOrResult);
		printf("buf : %s\n", recv_packet->buf);
		if(strcmp(recv_packet->cmdOrResult,"Command") == 0)
		{
			int str_Length = strlen(recv_packet->buf);
			char* newStrPtr = (char*)malloc(sizeof(char)*(str_Length+1));
			printf("check1\n");
			if(clnt_sock == 4)
			{
				printf("check1_1\n");
				history_arr_C1[history_count_C1] = newStrPtr;
				history_count_C1++;
			}

			printf("check1_2\n");

			if(clnt_sock == 5)
			{
				printf("check1_2\n");
				history_arr_C2[history_count_C2] = newStrPtr;
                                history_count_C2++;
			}

			printf("Before Send_msg\n");
			send_msg(msg, str_len,clnt_sock,recv_packet);
			printf("Before free recvpacket\n");
			free(recv_packet);
			continue;
		}

		if(strcmp(recv_packet->cmdOrResult,"Print_Result") == 0)
                {
			printf("Print Result:%s\n",recv_packet->buf);
			send_msg(msg, str_len,clnt_sock,recv_packet);
		}
		printf("check2\n");
		//send_msg(msg, str_len,clnt_sock,recv_packet);
		free(recv_packet);
		Packet *recv_packet = malloc(sizeof(recv_packet)+PACKET_SIZE);
		memset(recv_packet,0,sizeof(recv_packet)+PACKET_SIZE);
	}


	//pthread_mutex_lock(&mutx);
	for(i=0; i<clnt_cnt; i++)   // remove disconnected client
	{
		if(clnt_sock==clnt_socks[i])
		{
			while(i++<clnt_cnt-1)
				clnt_socks[i]=clnt_socks[i+1];
			break;
		}
	}
	clnt_cnt--;
	//pthread_mutex_unlock(&mutx);
	close(clnt_sock);
	printf("close_sock\n");
	return NULL;
}
void send_msg(char * msg, int len, int sock_num, Packet *p)   // send to all
{
	printf("send\n");
	int i;
	int clnt_sock = sock_num; 

	//pthread_mutex_lock(&mutx);
	for(i=0; i<clnt_cnt; i++)
	{
		if(clnt_socks[i]==clnt_sock)
		{
			continue;
		}
		// 구조체 먼저
		write(clnt_socks[i], (char*)p,sizeof(p)+PACKET_SIZE);
	//pthread_mutex_unlock(&mutx);
	}
	printf("write success!!\n");
}
void error_handling(char * msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

void *t_PrintUI(void *arg)
{
	int nMenu = 0;

	//pthread_mutex_lock(&mutx);
	while((nMenu = PrintUI()) !=0)
	{
		switch(nMenu)
		{
			case 1:
				getList();
				break;
			case 2:
				disConnect();
				break;
			case 3:
				getHistory();
				break;
			case 4:
				getMenu();
				break;
		}
	}
	//pthread_mutex_unlock(&mutx);
}

int PrintUI()
{
	int nInput = 0;
	// system("cls");
	printf("===================================================\n");
	printf("서버 Start\n");
	printf("---------------------------------------------------\n");
	printf("[1] 연결현황출력\t [2] 연결종료\t [3] 명령어기록보기\t [4] 전체보기\t");
	printf("===================================================\n");

	// 사용자가 선택한 메뉴의 값을 반환한다.
	scanf("%d", &nInput);
	//getchar();
	//버퍼에 남은 엔터 제거용
	return nInput;
}

void getList()
{
	for(int i=0; i<clnt_cnt; i++)
	{
		printf("소켓번호 : %d, IP : %s, Port : %d\n",socket_info_array[i].sock_Num, socket_info_array[i].IP_Address,socket_info_array[i].Port);
	}
}

void disConnect()
{
	int index;
	printf("삭제할 소켓 번호를 입력하세요 :\n");
	scanf("%d",&index);
	for(int i=clnt_cnt-1; i>=0; i--)
	{
		if(socket_info_array[i].sock_Num == index)
		{
			close(index);
			printf("%d 번 소켓 연결이 종료되었습니다.\n",index);
			if(i==clnt_cnt-1)
			{
				clnt_cnt--;
				break;
			}
			else
			{
				strcpy(socket_info_array[i].IP_Address,socket_info_array[i+1].IP_Address);
				socket_info_array[i].Port = socket_info_array[i+1].Port;
				socket_info_array[i].sock_Num = socket_info_array[i+1].sock_Num;
				clnt_cnt--;
				break;
			}
		}
		printf("찾는 소켓 번호가 존재 하지 않습니다.\n");
	}
}

void getHistory()
{
	int index;
	printf("명령어 기록을 확인하고 싶은 소켓 번호를 입력하세요\n");
	scanf("%d",&index);
	if(index == 4)
	{
		for (int i=0; i<history_count_C1; i++)
		{
			printf("%d번 소켓의 명령어 기록 %d : %s\n",index,i+1,history_arr_C1[i]);
		}
	}
	else if (index == 5)
	{
		for (int i=0; i<history_count_C2; i++)
		{
			printf("%d번 소켓의 명령어 기록 %d : %s\n",index, i+1, history_arr_C2[i]);
		}
	}
	else
	{
		printf("해당번호에 관한 기록이 없습니다.\n");
	}
}

void getMenu()
{
	printf("위의 메뉴가 다 보이게 하자 \n");
	//getList();
	//disConnect();
	//getHistory();
}

