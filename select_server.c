#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <pthread.h>

// 100 -> 1024
#define BUF_SIZE 1024
#define MAX_CLNT 256
#define PACKET_SIZE 1102
#define UserManageMent_SIZE 38

/// Thread ///
void *handle_clnt(void * arg);
void *t_PrintUI(void *data);
/////////////

void error_handling(char *buf);
int PrintUI();
void getHistory();
void disConnect();
void getList();
void *handle_connection(int sock_num, fd_set *reads);

int clnt_cnt =0;
int clnt_socks[MAX_CLNT];

pthread_mutex_t mutx;

typedef struct
{
	char IP_Address[14];
	char NickName[20];
	int Port;
	int sock_Num;
}socket_info;
socket_info socket_info_array[256];

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


int history_count_C1 = 0;
int history_count_C2 = 0;

char *history_arr_C1[256];
char *history_arr_C2[256];

char *history_arr[256][256];

void send_msg(int sock_num, Packet *p);

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	struct timeval timeout;
	fd_set reads, cpy_reads;

	pthread_t t_id;
	pthread_t thread_PrintUI;

	socklen_t adr_sz;
	int fd_max, str_len, fd_num, i;
	char buf[BUF_SIZE];
	if(argc!=2) 
	{
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
		error_handling("bind() error");
	if(listen(serv_sock, 5)==-1)
		error_handling("listen() error");

	FD_ZERO(&reads);
	FD_SET(serv_sock, &reads);
	fd_max=serv_sock;

	while(1)
	{
		if(pthread_create(&thread_PrintUI,NULL,t_PrintUI,(void*)&clnt_cnt) !=0 )
		{
			error_handling("PrintUI_Thread create error\n");
			continue;
		}

		cpy_reads=reads;
		//timeout.tv_sec=5;
		//timeout.tv_usec=5000;

		//printf("fd_max : %d\n",fd_max);

		if((fd_num=select(fd_max+1, &cpy_reads, 0, 0, NULL))==-1)
		{
			perror("select 함수 에러 내용 : ");
			break;
		}
		if(fd_num==0)
			continue;

		for(i=0; i<fd_max+1; i++)
		{
			if(FD_ISSET(i, &cpy_reads))
			{
				if(i==serv_sock)     // connection request!
				{
					adr_sz=sizeof(clnt_adr);
					clnt_sock=
						accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
					FD_SET(clnt_sock, &reads);
					if(fd_max<clnt_sock)
						fd_max=clnt_sock;

					strcpy(socket_info_array[clnt_cnt].IP_Address,inet_ntoa(clnt_adr.sin_addr));
					socket_info_array[clnt_cnt].Port = (int)ntohs(clnt_adr.sin_port);
					socket_info_array[clnt_cnt].sock_Num = clnt_sock;

					printf("connected client: %d \n", clnt_sock);

					// accept 이후 유저 아이피, 포트, 닉네임 읽어오기 
					UserManageMent *user_packet = malloc(sizeof(user_packet)+UserManageMent_SIZE);
					memset(user_packet,0,sizeof(user_packet)+UserManageMent_SIZE);
					if(read(clnt_sock,(char*)user_packet,sizeof(user_packet)+UserManageMent_SIZE)<=0)
					{
						error_handling("Reading User_Info Failed\n");
					}
					strcpy(socket_info_array[clnt_cnt].NickName,user_packet->NickName);

					//printf("recv User_Info Success\n");
					//printf("NickName : %s\n",socket_info_array[clnt_cnt].NickName);

					free(user_packet);
					clnt_cnt++;
					continue;
				}
				else    // read message!
				{
					handle_connection(i,&reads);
				}
			}
		}
		//printf("Check end of whil	e\n");
	}
	//printf("Check\n");
	close(serv_sock);
	return 0;
}

void error_handling(char *buf)
{
	fputs(buf, stderr);
	fputc('\n', stderr);
	exit(1);
}

void *t_PrintUI(void *arg)
{
	int nMenu = 0;

	pthread_mutex_lock(&mutx);
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
			default:
				printf("메뉴의 보기에 있는 숫자 중에서 입력하세요.\n");
				break;
		}
	}
	pthread_mutex_unlock(&mutx);
}

void send_List(int sock_num, Packet *p)   // send to all
{
	char buf[1024];
	memset(buf,0,sizeof(buf));
	memset(p,0,sizeof(p)+PACKET_SIZE);
	//printf("send List\n");
	int clnt_sock = sock_num; 

	for(int i=0; i<clnt_cnt; i++)
	{
		strcat(buf,socket_info_array[i].NickName);
		strcat(buf,"\n");
	}

	strcpy(p->Separator,"List");
	strcpy(p->buf, buf);

	if(write(clnt_sock, (char*)p,sizeof(p)+PACKET_SIZE)<=0)
	{
		error_handling("Sending List Error\n");
	}
	//printf("send List Success\n");
}

void send_msg(int sock_num, Packet *p)   // send to all
{
	//printf("send\n");
	int clnt_sock = sock_num; 
	//printf("sendmsg buf : %s",p->buf);
	for(int i=0; i<clnt_cnt; i++)
	{
		if(write(socket_info_array[i].sock_Num, (char*)p,sizeof(p)+PACKET_SIZE)<=0)
		{
			error_handling("send error\n");
		}
	}
	//printf("send msg Success\n");
}

int PrintUI()
{
	int nInput = 0;
	// system("cls");
	printf("===================================================\n");
	printf("서버 Start\n");
	printf("---------------------------------------------------\n");
	printf("[1] 연결현황출력\t [2] 연결종료\t [3] 명령어기록보기\t\n");
	printf("===================================================\n");

	// 사용자가 선택한 메뉴의 값을 반환한다.
	scanf("%d", &nInput);
	//getchar();
	//버퍼에 남은 엔터 제거용
	while (getchar() != '\n'); //scanf_s 버퍼 비우기, 밀림 막음
	if (nInput > 3 || nInput < 1)
	{
		nInput = 4; // 0~2 사이의 메뉴 값이 아니라면 defalut로 보내기
	}
	return nInput;
}

void getList()
{
	for(int i=0; i<clnt_cnt; i++)
	{
		printf("소켓번호 : %d, IP : %s, Port : %d, 닉네임 : %s\n",socket_info_array[i].sock_Num, socket_info_array[i].IP_Address, socket_info_array[i].Port, socket_info_array[i].NickName);
	}
}

void disConnect()
{
	int index;
	printf("삭제할 소켓 번호를 입력하세요 :\n");
	scanf("%d",&index);
	shutdown(index,SHUT_WR);
	close(index);
	clnt_cnt--;
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

void *handle_connection(int sock_num, fd_set *reads)
{
	int clnt_sock= sock_num;
	//printf("hi iam handle thread %d\n",clnt_sock);
	int str_len=0, i;
	char msg[BUF_SIZE];

	Packet *recv_packet = malloc(sizeof(recv_packet)+PACKET_SIZE);
	if (recv_packet == NULL)
	{
		error_handling("recv_packet Pointer is NULL\n");
		return (void*)-1;
	}
	memset(recv_packet,0,sizeof(recv_packet)+PACKET_SIZE);

	str_len=read(clnt_sock, (char*)recv_packet, sizeof(recv_packet)+PACKET_SIZE);
	if(str_len == 0)    // close request!
	{
		printf("Close Request\n");
		//printf("str_len check\n");
		//printf("clnt_cnt :%d\n",clnt_cnt);
		FD_CLR(clnt_sock, reads);
		close(clnt_sock);
		printf("%d 번 소켓 연결이 종료되었습니다.\n", clnt_sock);
		for(int i=clnt_cnt-1; i>=0; i--)
		{
			if(socket_info_array[i].sock_Num == clnt_sock)
			{
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
					continue;
				}
			}
		}
		if(clnt_sock == 4)
		{
			for(int i=0; i<history_count_C1; i++)
			{
				free(history_arr_C1[i]);
				history_count_C1 = 0;
			}
		}
		else if(clnt_sock == 5)
		{
			for(int i=0; i<history_count_C2; i++)
			{
				free(history_arr_C2[i]);
				history_count_C2 = 0;
			}
		}

		free(recv_packet);
		return NULL;
	}
	else 
	{
		if(strcmp(recv_packet->Separator,"Command") == 0)
		{
			int str_Length = strlen(recv_packet->buf);
			char* newStrPtr = (char*)malloc(sizeof(char)*(str_Length+1));
			strcpy(newStrPtr,recv_packet->buf);
			if(clnt_sock == 4)
			{
				history_arr_C1[history_count_C1] = newStrPtr;
				history_count_C1++;
			}

			if(clnt_sock == 5)
			{
				history_arr_C2[history_count_C2] = newStrPtr;
				history_count_C2++;
			}
			send_msg(clnt_sock,recv_packet);
		}

		else if(strcmp(recv_packet->Separator,"Print_Result") == 0)
		{
			//printf("Print Result:%s\n",recv_packet->buf);
			send_msg(clnt_sock,recv_packet);
		}

		else if(strcmp(recv_packet->Separator,"Message") == 0)
		{
			//printf("Server Recv Message : %s\n",recv_packet->buf);
			send_msg(clnt_sock,recv_packet);
		}

		else if(strcmp(recv_packet->Separator,"List") == 0)
		{
			//printf("Server Accepts List Request : %s\n",recv_packet->Separator);
			send_List(clnt_sock,recv_packet);
		}

		//printf("after read\n");
	}
	free(recv_packet);
	//printf("end of handle func\n");
	return NULL;
}

