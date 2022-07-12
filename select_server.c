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

typedef struct
{
	char ip_Address[16];
	char nickName[20];
	char user_Status[10];
	int port;
	int sock_Num;
}socket_Info;
socket_Info socket_Info_Array[256];

#pragma pack(push,1)
typedef struct
{
	char ip_Address[16];
	char nickName[20];
	char user_Status[10];
	int port;
}user_ManageMent;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct
{
	char ip_Address[16];
	int port;
	char separator[20];	// “Command”, “Print_Result”, “DisConnect”
	char my_Name[20];
	char target_Name[20];
	char buf[1024];
}packet;
#pragma pack(pop)

typedef struct node
{
	char nickName[20];
	char cmd[20];
	int sock_Num;
	struct node *next;	
}node;

typedef struct
{
	int sock;
	node *node;
}sock_And_Node;

node* load_History(node *list, user_ManageMent *p);
void show_History(node *list);
void save_History(node *list, int sock);
void free_History(node *list);
void get_History(node *list);
void append_History(node *list, packet *p, int sock);
void send_Msg(int sock, packet *p);
void send_Cmd(int sock, packet *p, node *list);
void *handle_Connection(int sock, fd_set *reads, node *list);
int search_Node(node *list, char *name, int history_Count);

/// Thread ///
void *handle_Clnt(void * arg);
void *t_Print_Ui(void *data);
/////////////
void error_Handling(char *buf);
int print_Ui();
void get_History();
void disConnect();
void get_List();
int clnt_Cnt =0;
int clnt_Socks[MAX_CLNT];
pthread_mutex_t mutx;

int main(int argc, char *argv[])
{
	node *head = malloc(sizeof(node));    // 머리 노드 생성
                                          // 머리 노드는 데이터를 저장하지 않음
	int serv_Sock, clnt_Sock;
	struct sockaddr_in serv_Adr, clnt_Adr;
	struct timeval timeout;
	fd_set reads, cpy_Reads;

	pthread_t t_Id;
	pthread_t print_Ui_Thread;

	socklen_t adr_Sz;
	int fd_Max, str_Len, fd_Num, i;
	char buf[BUF_SIZE];
	sock_And_Node node_Arg;

	if(argc!=2) 
	{
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	pthread_mutex_init(&mutx, NULL);
	serv_Sock=socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_Adr, 0, sizeof(serv_Adr));
	serv_Adr.sin_family=AF_INET;
	serv_Adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_Adr.sin_port=htons(atoi(argv[1]));
	
	if(bind(serv_Sock, (struct sockaddr*) &serv_Adr, sizeof(serv_Adr))==-1)
		error_Handling("bind() error");
	if(listen(serv_Sock, 5)==-1)
		error_Handling("listen() error");

	FD_ZERO(&reads);
	FD_SET(serv_Sock, &reads);
	fd_Max=serv_Sock;

	while(1)
	{
		node_Arg.sock = clnt_Sock;
		node_Arg.node = head;

		if(pthread_create(&print_Ui_Thread,NULL,t_Print_Ui,(void*)&node_Arg) !=0 )
		{
			error_Handling("PrintUI_Thread create error\n");
			continue;
		}

		cpy_Reads=reads;
		//timeout.tv_sec=5;
		//timeout.tv_usec=5000;

		//printf("fd_max : %d\n",fd_max);

		if((fd_Num=select(fd_Max+1, &cpy_Reads, 0, 0, NULL))==-1)
		{
			perror("select 함수 에러 내용 : ");
			break;
		}
		if(fd_Num==0)
			continue;

		for(i=0; i<fd_Max+1; i++)
		{
			if(FD_ISSET(i, &cpy_Reads))
			{
				if(i==serv_Sock)     // connection request!
				{
					adr_Sz = sizeof(clnt_Adr);
					clnt_Sock=
						accept(serv_Sock, (struct sockaddr*)&clnt_Adr, &adr_Sz);
					FD_SET(clnt_Sock, &reads);
					if(fd_Max<clnt_Sock)
						fd_Max=clnt_Sock;

					strcpy(socket_Info_Array[clnt_Cnt].ip_Address,inet_ntoa(clnt_Adr.sin_addr));
					socket_Info_Array[clnt_Cnt].port = (int)ntohs(clnt_Adr.sin_port);
					socket_Info_Array[clnt_Cnt].sock_Num = clnt_Sock;

					printf("connected client: %d \n", clnt_Sock);

					// accept 이후 유저 아이피, 포트, 닉네임 읽어오기 
					user_ManageMent *user_Packet = malloc(sizeof(user_ManageMent));
					memset(user_Packet,0,sizeof(user_ManageMent));
					if(read(clnt_Sock,(char*)user_Packet,sizeof(user_ManageMent))<=0)
					{
						error_Handling("Reading User_Info Failed\n");
					}
					strcpy(socket_Info_Array[clnt_Cnt].nickName,user_Packet->nickName);
					strcpy(socket_Info_Array[clnt_Cnt].user_Status,user_Packet->user_Status);

					head = load_History(head,user_Packet);

					free(user_Packet);
					clnt_Cnt++;
					continue;
				}
				else    // read message!
				{
					handle_Connection(i,&reads,head);
				}
			}
		}
	}
	free(head);
	close(serv_Sock);
	return 0;
}

void error_Handling(char *buf)
{
	fputs(buf, stderr);
	fputc('\n', stderr);
	exit(1);
}

void *t_Print_Ui(void *arg)
{
	int menu_Num = 0;

	sock_And_Node *data = (sock_And_Node*)arg;
	int sock = data->sock;
	node *head = data -> node;

	pthread_mutex_lock(&mutx);
	while((menu_Num = print_Ui()) !=0)
	{
		switch(menu_Num)
		{
			case 1:
				get_List();
				break;
			case 2:
				disConnect();
				break;
			case 3:
				get_History(head);
				break;
			default:
				printf("메뉴의 보기에 있는 숫자 중에서 입력하세요.\n");
				break;
		}
	}
	pthread_mutex_unlock(&mutx);
}

void send_Cmd(int sock, packet *p, node *list)   // send to all
{
	char buf[1024];
	char num[10];
	int history_Count=0;

	memset(buf,0,sizeof(buf));
	//memset(p,0,sizeof(packet));
	int clnt_Sock = sock; 
	strcat(buf,"명령어 : \n");

	if(list->next != NULL)
	{
		node *cur = list->next;
		while(cur != NULL)
		{
			if(strcmp(p->my_Name,cur->nickName) == 0)
			{
				history_Count++;
				sprintf(num, "%d", history_Count);
				strcat(buf,num);
				strcat(buf, " : ");
				strcat(buf,cur->cmd);
				strcat(buf,"\n");
				//printf("%d : %s\n",cnt,cur->cmd);
				cur = cur->next;
			}
			else
			{
				cur = cur->next;
			}
		}
	}
	strcpy(p->buf, buf);

	if(write(clnt_Sock, (char*)p,sizeof(packet))<=0)
	{
		error_Handling("Sending List Error\n");
	}
}

void send_List(int sock, packet *p)   // send to all
{
	char buf[1024];
	memset(buf,0,sizeof(buf));
	memset(p,0,sizeof(packet));
	int clnt_Sock = sock; 

	for(int i=0; i<clnt_Cnt; i++)
	{
		strcat(buf,"닉네임 : ");
		strcat(buf,socket_Info_Array[i].nickName);
		strcat(buf," ");
		strcat(buf, "온라인 상태여부 : ");
		strcat(buf,socket_Info_Array[i].user_Status);
		strcat(buf,"\n");
	}

	strcpy(p->separator,"List");
	strcpy(p->buf, buf);

	if(write(clnt_Sock, (char*)p,sizeof(packet))<=0)
	{
		error_Handling("Sending List Error\n");
	}
}

void send_Msg(int sock, packet *p)   // send to all
{
	int clnt_Sock = sock; 
	for(int i=0; i<clnt_Cnt; i++)
	{
		if(write(socket_Info_Array[i].sock_Num, (char*)p,sizeof(packet))<=0)
		{
			error_Handling("send error\n");
		}
	}
}

int print_Ui()
{
	int input_Num = 0;
	// system("cls");
	printf("===================================================\n");
	printf("서버 Start\n");
	printf("---------------------------------------------------\n");
	printf("[1] 연결현황출력\t [2] 연결종료\t [3] 명령어기록보기\t\n");
	printf("===================================================\n");

	// 사용자가 선택한 메뉴의 값을 반환한다.
	scanf("%d", &input_Num);
	//getchar();
	//버퍼에 남은 엔터 제거용
	while (getchar() != '\n'); //scanf_s 버퍼 비우기, 밀림 막음
	if (input_Num > 3 || input_Num < 1)
	{
		input_Num = 4; // 0~2 사이의 메뉴 값이 아니라면 defalut로 보내기
	}
	return input_Num;
}

void get_List()
{
	if(clnt_Cnt==0)
	{
		printf("현재 접속한 인원이 없습니다.\n");
		return;
	}
	for(int i=0; i<clnt_Cnt; i++)
	{
		printf("소켓번호 : %d, IP : %s, Port : %d, 닉네임 : %s, 유저상태 : %s\n",socket_Info_Array[i].sock_Num, socket_Info_Array[i].ip_Address, socket_Info_Array[i].port, socket_Info_Array[i].nickName, socket_Info_Array[i].user_Status);
	}
	return;
}

void disConnect()
{
	int index;
	printf("삭제할 소켓 번호를 입력하세요 :\n");
	scanf("%d",&index);
	shutdown(index,SHUT_WR);
}

void get_History(node *list)
{
	int cnt = 0;
	char target_Name[20];
	printf("명령어 기록을 확인하고 싶은 닉네임을 입력하세요\n");
	scanf("%s",target_Name);
	if(list->next != NULL)
	{
		node *cur = list->next;
		printf("===================================================\n");
		printf("명령어 기록입니다.\n");
		printf("list->nickName : %s , target_Name : %s list->cmd : %s\n",cur->nickName,target_Name,cur->cmd);
		while(cur != NULL)
		{
			if(strcmp(cur->nickName,target_Name)==0)
			{
				++cnt;
				printf("%s의 %d번 명령어 기록: %s\n",cur->nickName,cnt,cur->cmd);
				cur = cur->next;
				//cnt++;
			}
			else
			{
				cur = cur->next;
			}
		}
		if (cnt == 0)
		{
			printf("해당 닉네임의 명령어 기록이 %d 입니다.\n",cnt);
		}
	}
	else
	{
		printf("명령어 기록이 0입니다.\n");
	}
	return;
}

void *handle_Connection(int sock, fd_set *reads, node *list)
{
	int clnt_Sock= sock;
	int str_Len=0, i;
	char msg[BUF_SIZE];

	packet *recv_Packet = malloc(sizeof(packet));
	if (recv_Packet == NULL)
	{
		error_Handling("recv_Packet Pointer is NULL\n");
		return (void*)-1;
	}
	memset(recv_Packet,0,sizeof(packet));

	str_Len=read(clnt_Sock, (char*)recv_Packet, sizeof(packet));
	if(str_Len == 0)    // close request!
	{
		printf("Close Request\n");
		FD_CLR(clnt_Sock, reads);
		close(clnt_Sock);
		printf("%d 번 소켓 연결이 종료되었습니다.\n", clnt_Sock);
		save_History(list,clnt_Sock);
		for(int i=0; i<clnt_Cnt; i++)
		{
			if(socket_Info_Array[i].sock_Num == clnt_Sock)
			{
				if(i==clnt_Cnt-1)
				{
					clnt_Cnt--;
					break;
				}
				else
				{
					for(int j=i; j<clnt_Cnt;j++)
					{
						strcpy(socket_Info_Array[j].ip_Address,socket_Info_Array[j+1].ip_Address);
						strcpy(socket_Info_Array[j].nickName,socket_Info_Array[j+1].nickName);
						strcpy(socket_Info_Array[j].user_Status,socket_Info_Array[j+1].user_Status);
						socket_Info_Array[j].port = socket_Info_Array[j+1].port;
						socket_Info_Array[j].sock_Num = socket_Info_Array[j+1].sock_Num;
					}
					clnt_Cnt--;
				}
			}
		}
		save_History(list,clnt_Sock);
		//free_History(list);

		free(recv_Packet);
		return NULL;
	}
	else 
	{
		if(strcmp(recv_Packet->separator,"Command") == 0)
		{
			int str_Length = strlen(recv_Packet->buf);
			char* new_Str_Ptr = (char*)malloc(sizeof(char)*(str_Length+1));
			strcpy(new_Str_Ptr,recv_Packet->buf);
			append_History(list,recv_Packet,clnt_Sock);
			for(int i=0; i<clnt_Cnt;i++)
			{
				if(strcmp(socket_Info_Array[i].nickName,recv_Packet->target_Name) == 0)
				{
					send_Msg(clnt_Sock,recv_Packet);
					free(recv_Packet);
					return NULL;
				}
			}
			strcpy(recv_Packet->separator,"Error");
			strcpy(recv_Packet->buf,"현재 해당 닉네임의 클라이언트는 접속되어 있지 않습니다.(명령어전송불가)");
			send_Msg(clnt_Sock,recv_Packet);
		}

		else if(strcmp(recv_Packet->separator,"Print_Result") == 0)
		{
			send_Msg(clnt_Sock,recv_Packet);
		}

		else if(strcmp(recv_Packet->separator,"Message") == 0)
		{
			if(strcmp(recv_Packet->target_Name,"ALL") == 0)
			{
				send_Msg(clnt_Sock,recv_Packet);
				free(recv_Packet);
				return NULL;
			}
			for(int i=0; i<clnt_Cnt;i++)
			{
				if(strcmp(socket_Info_Array[i].nickName,recv_Packet->target_Name) == 0)
				{
					send_Msg(clnt_Sock,recv_Packet);
					free(recv_Packet);
					return NULL;
				}
			}
			strcpy(recv_Packet->separator,"Error");
			strcpy(recv_Packet->buf,"현재 해당 닉네임의 클라이언트는 접속되어 있지 않습니다.(메세지전송불가)");
			send_Msg(clnt_Sock,recv_Packet);
		}

		else if(strcmp(recv_Packet->separator,"List") == 0)
		{
			send_List(clnt_Sock,recv_Packet);
		}

		else if(strcmp(recv_Packet->separator,"Change_Status") == 0)
		{
			for (int i=0; i<clnt_Cnt; i++)
			{
				if(strcmp(socket_Info_Array[i].nickName,recv_Packet->my_Name) == 0)
				{
					strcpy(socket_Info_Array[i].user_Status,recv_Packet->buf);
				}
				else 
				{
					continue;
				}
			}
		}
		else if (strcmp(recv_Packet->separator,"CMDHistory")==0)
		{
			int history_Count = 0;
			history_Count = search_Node(list,recv_Packet->my_Name,history_Count);
			if(history_Count == 0)
			{
				strcpy(recv_Packet->buf,"해당 닉네임의 명령어 기록이 0입니다.");
				send_Msg(clnt_Sock,recv_Packet);
				//send_Cmd(clnt_sock,recv_Packet,head);
			}
			else
			{
				send_Cmd(clnt_Sock,recv_Packet,list);
			}
		}
	}
	free(recv_Packet);
	return NULL;
}

void append_History(node *list, packet *p, int sock)
{
	if(list->next == NULL)
	{
		node *new_Node = malloc(sizeof(node));
		strcpy(new_Node->cmd, p->buf);
		strcpy(new_Node->nickName, p->my_Name);
		new_Node->sock_Num = sock;
		new_Node->next = NULL;
		list->next = new_Node;
	}
	else 
	{
		node *cur = list;
		while(cur->nex != NULL)
		{
			cur = cur->next;
		}
		node *new_Node = malloc(sizeof(node));
		strcpy(new_Node->cmd,p->buf);
		strcpy(new_Node->nickName,p->my_Name);
		new_Node->sock_Num = sock;
		new_Node->next = NULL;
		cur->next = new_Node;
	}
	return;
}

void save_History(node *list, int sock)
{
	node* head_Ptr = list;
	int str_Len = 0;
	int history_Count = 0;
	int count = 0;
	char cmd[20];
	char file_Path[30];
	char name[20];
	memset(name,0,sizeof(name));
	FILE *fp;
	strcpy(cmd,"pwd");
	fp = popen(cmd,"r");
	if(fp == NULL)
	{
		error_Handling("popen()실패 또는 없는 리눅스 명령어를 입력하였음.\n");
		return;
	}
	fgets(file_Path,30,fp);
	pclose(fp);

	str_Len = strlen(file_Path);
	file_Path[str_Len-1] = '/';

	for(int i=0; i<clnt_Cnt; i++)
	{
		if(socket_Info_Array[i].sock_Num == sock)
		{
			strcpy(name,socket_Info_Array[i].nickName);
			break;
		}
	}
	// 해당 name을 Search 하고, 명령어 개수를 Count하시오.
	count = search_Node(list,name,history_Count);
	if(count == 0)
	{
		return;
	}
	FILE* stream;

	strcat(file_Path,name);
	strcat(file_Path,".dat");
	//printf("file_Path : %s\n",file_Path);

	stream = fopen(file_Path, "wb");
	if (stream == NULL) 
	{
		error_Handling("파일 스트림 생성 실패\n");
		return;
	}
	fwrite(&count, sizeof(count), 1, stream);
	if(head_Ptr!=NULL)
	{
		head_Ptr = head_Ptr->next;
		while (head_Ptr != NULL) 
		{
			//printf("저장부분 명령어 : %s, 주소 : %p",pHead->cmd, pHead->next);
			if(strcmp(head_Ptr->nickName,name) == 0)
			{
				fwrite(head_Ptr, sizeof(node), 1, stream);
				head_Ptr = head_Ptr->next;
			}
			else
			{
				head_Ptr = head_Ptr->next;
			}
		}
	}
	printf("파일 저장 성공!\n");
	fclose(stream);
	return;
}

void free_History(node *list)
{
	while (list != NULL) 
	{
		node* cur = list;
		list = cur->next;
		free(cur);
	}
	return;
}

node* load_History(node *list, user_ManageMent *p)
{
	int str_Len = 0;
	int history_Count = 0;

	char cmd[20];
	char file_Path[30];
	char name[20];
	memset(name,0,sizeof(name));
	strcpy(name,p->nickName);

	node* head_Ptr = malloc(sizeof(node));
	node* end_Ptr = NULL;

	FILE *fp;
	strcpy(cmd,"pwd");
	fp = popen(cmd,"r");
	if(fp == NULL)
	{
		error_Handling("popen()실패 또는 없는 리눅스 명령어를 입력하였음.\n");
		return list;
	}
	fgets(file_Path,30,fp);
	pclose(fp);

	str_Len = strlen(file_Path);
	file_Path[str_Len-1] = '/';

	strcat(file_Path,name);
	strcat(file_Path,".dat");

	//printf("file_Path : %s\n",file_Path);
	FILE* stream;
	
	history_Count = search_Node(list,p->nickName,history_Count);
	if (history_Count == 0)
	{
		return list;
	}

	stream = fopen(file_Path, "rb");
	if (stream == NULL) 
	{
		error_Handling("현재 닉네임 이전 명령어기록 파일 없음.\n");
		return list;
	}
	fread(&history_Count, sizeof(history_Count), 1, stream);
	if (history_Count == 0) 
	{
		error_Handling("이전 같은 닉네임의 명령어 송신 기록은 0입니다.\n");
		return list;
	}
	printf("현재 닉네임으로 저장 된 명령어 기록 수는 %d개 입니다.\n\n", history_Count);

	for (int i = 0; i < history_Count; i++) 
	{
		if(head_Ptr->next == NULL)
		{
			node *new_Node = malloc(sizeof(node));
			head_Ptr->next = new_Node;
			end_Ptr = new_Node;
			fread(new_Node, sizeof(node), 1, stream);
			strcpy(end_Ptr->cmd,new_Node->cmd);
			end_Ptr->next = NULL;
		}
		else 
		{
			node *new_Node = malloc(sizeof(node));
			end_Ptr->next = new_Node;
			end_Ptr = end_Ptr->next;
			fread(new_Node, sizeof(node), 1, stream);
			strcpy(end_Ptr->cmd,new_Node->cmd);
			end_Ptr->next = NULL;
		}
	}
	fclose(stream);
	return head_Ptr;
}

int search_Node(node *list, char *name, int history_Count)
{
	int count = history_Count;
	if(list->next != NULL)
	{
		node *cur = list->next;
		while(cur != NULL)
		{
			if(strcmp(name,cur->nickName) == 0)
			{
				//printf("SearchNode - count : %d : , NickName : %s\n",count,cur->NickName);
				cur = cur->next;
				count++;
			}
			else
			{
				cur = cur->next;
			}
		}
	}
	else
	{
		printf("%s 고객님의 명령어 기록이 0입니다.\n",name);
	}
	return count;
}
