#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <stdbool.h>

#define BUF_SIZE 1024
#define NAME_SIZE 20

int PrintUI();
void error_handling(char *message);
void clearInputBuffer();
void getList(int sock);
void send_message(int sock);
bool convertStatus(int sock);
void *handle_connection(int sock_num, fd_set *reads);
void *t_PrintUI(void *data);
void *recv_msg(void *arg);

char name[NAME_SIZE]="[DEFAULT]";
char msg[BUF_SIZE];
pthread_mutex_t mutx;
bool statusFlag;
int history_Count = 0;

// 명령어 기록 유지를 위한 NODE 구조체 (링크드리스트)
typedef struct 
{							  // 연결 리스트의 노드 구조체
	char cmd[20];             // 데이터를 저장할 멤버
    struct NODE *next;    // 다음 노드의 주소를 저장할 포인터
}NODE;

typedef struct
{
	int sock;
	NODE *node;
}SockAndNode;

#pragma pack(push,1)
typedef struct
{
	char IP_Address[16];
	char NickName[20];
	char UserStatus[10];
	int Port;
}UserManageMent;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct
{
	char IP_Address[16];
	int Port;
	char Separator[20];	// “Command”, “Print_Result”, “DisConnect”
	char MyName[20];
	char TargetName[20];
	char buf[1024];
}Packet;
#pragma pack(pop)

void disConnect(int sock, NODE *list);
void send_cmd(int sock, NODE *list);
void appendHistory(NODE *list, char *cmd);
void showHistory(NODE *list);
NODE* loadHistory(NODE *list);
void saveHistory(NODE *list);
void freeHistory(NODE *list);
void getHistory(NODE *list);

int main(int argc, char *argv[])
{
	NODE *head = malloc(sizeof(NODE));    // 머리 노드 생성
                                          // 머리 노드는 데이터를 저장하지 않음
	int fd_max, str_len, fd_num, i, sock;
	char message[BUF_SIZE];
	struct sockaddr_in serv_adr;
	struct timeval timeout;
	fd_set reads, cpy_reads;

	pthread_t snd_thread, rcv_thread, printUI_thread;
	pthread_mutex_init(&mutx, NULL);
	void * thread_return;

	SockAndNode NodeArg;

	if(argc!=4)
	{
		printf("Usage : %s <IP> <port> <name>\n", argv[0]);
		exit(1);
	}
	
	sprintf(name, "%s", argv[3]);

	// 처음에 명령어 기록 파일을 Load 해온다.
	head = loadHistory(head);
	sock=socket(PF_INET, SOCK_STREAM, 0);   
	if(sock==-1)
		error_handling("socket() error\n");
	
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_adr.sin_port=htons(atoi(argv[2]));
	
	if(connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
		error_handling("connect() error!\n");
	else
		puts("Connected...........");

	UserManageMent *user_packet = malloc(sizeof(UserManageMent));
	memset(user_packet,0,sizeof(UserManageMent));
	strcpy(user_packet->IP_Address, argv[1]);
	user_packet->Port = serv_adr.sin_port;
	strcpy(user_packet->NickName,name);
	strcpy(user_packet->UserStatus,"Online");
	statusFlag = true;
	if(write(sock,(char*)user_packet,sizeof(UserManageMent)) <= 0)
	{
		error_handling("Sending UserInfo Failed!!\n");
	}
	//printf("send UserInfo Success\n");
	//printf("sock_num : %d\n",sock);
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
		NodeArg.sock = sock;
		NodeArg.node = head;
		if(pthread_create(&printUI_thread,NULL,t_PrintUI,(void*)&NodeArg) !=0 )
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
	free(head);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	//exit(1);
}

void *handle_connection(int sock_num, fd_set *reads)
{
	int sock = sock_num;
	char name_msg[BUF_SIZE];
	int str_len = 0;

	Packet *recv_packet = malloc(sizeof(Packet));
	memset(recv_packet,0,sizeof(Packet));
	// 구조체 정보 먼저 받기, 커맨드인지 출력물인지 
	str_len = read(sock,(char*)recv_packet,sizeof(Packet));
	if(str_len == 0)
	{
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
					Packet *send_packet = malloc(sizeof(Packet));
					memset(send_packet,0,sizeof(Packet));
					strcpy(send_packet->Separator,"Print_Result");
					strcpy(send_packet->buf,recv_packet->buf);
					strcpy(send_packet->TargetName,recv_packet->TargetName);
					strcpy(send_packet->MyName,recv_packet->MyName);
					if(write(sock,(char*)send_packet,sizeof(Packet)) <= 0)
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
				printf("\n%s from : %s",recv_packet->buf,recv_packet->TargetName);
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
			printf("\nClient List : \n%s",recv_packet->buf);
		}

		else if (strcmp(recv_packet->Separator,"Error")==0)
		{
			if(strcmp(recv_packet->MyName,name)==0)
			{
				printf("\n %s <- %s \n",recv_packet->TargetName,recv_packet->buf);
			}
		}
	}
	free(recv_packet);
	return NULL;
}

void *t_PrintUI(void *arg)
{
	//int sock= *((int*)arg);
	SockAndNode *data = (SockAndNode*)arg;
	int sock = data->sock;
	NODE *head = data -> node;
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
				send_cmd(sock,head);
				break;
			case 4:
				getHistory(head);
				break;
			case 5:
				disConnect(sock, head);
				break;
			case 6:
				convertStatus(sock);
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
	printf("[1]유저목록보기\t [2] 메세지보내기\t [3]명령어보내기\n");
	printf("[4]명령어기록보기\t [5]연결종료하기 [6]OnLine/OffLine 전환\n");
	printf("===================================================\n");
	printf("번호를 입력하세요 : ");

	// 사용자가 선택한 메뉴의 값을 반환한다.
	scanf("%d", &nInput);
	//getchar();
	//버퍼에 남은 엔터 제거용
	while (getchar() != '\n'); //scanf_s 버퍼 비우기, 밀림 막음
	if (nInput > 6 || nInput < 1)
	{
		nInput = 7; // 0~2 사이의 메뉴 값이 아니라면 defalut로 보내기
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
	Packet *send_packet = malloc(sizeof(Packet));
	memset(send_packet,0,sizeof(Packet));
	
	strcpy(send_packet->Separator,"List");
	write(sock, (char*)send_packet,sizeof(Packet));
	free(send_packet);
 
	return;
}

void getHistory(NODE *list)
{
	showHistory(list);
	return;
}

void disConnect(int sock, NODE *list)
{
	printf("\n서버와의 접속이 종료됩니다.\n");
	saveHistory(list);
	freeHistory(list);
	history_Count = 0;
	exit(0);	// 정상종료 
}

void send_message(int sock)   // send to all
{
	int clnt_sock = sock;
	Packet *send_packet = malloc(sizeof(Packet));
	memset(send_packet,0,sizeof(Packet));
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
		printf("===================================================\n");
		printf("전체 보내기 모드입니다.\n");
		printf("===================================================\n");
		printf("보낼 메세지를 입력하세요 : ");
		clearInputBuffer();
		fgets(name_msg,BUF_SIZE,stdin);
		strcpy(send_packet->TargetName,"ALL");
		strcpy(send_packet->buf,name_msg);
		if(write(sock, (char*)send_packet,sizeof(Packet))<=0)
		{
				error_handling("전체 메세지 전송실패\n");
		}
		printf("전체 메세지 보내기 전송 완료\n");
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
			if(write(sock, (char*)send_packet,sizeof(Packet))<=0)
			{
				error_handling("귓속말 전송실패\n");
			}
			printf("유저상태 전송 완료\n");
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
void send_cmd(int sock, NODE *list)   // send to all
{
	printf("===================================================\n");
	printf("명령어 보내기 모드입니다.\n");
	printf("===================================================\n");
	printf("명령어를 보낼 타겟 클라이언트 닉네임을 입력하세요 : ");

	Packet *send_packet = malloc(sizeof(Packet));
	memset(send_packet,0,sizeof(Packet));

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
		appendHistory(list,buf);
		strcpy(send_packet->Separator,"Command");
		strcpy(send_packet->MyName,name);
		//printf("when send cmd check name :%s",send_packet->MyName);
		strcpy(send_packet->TargetName,TargetName);
		strcpy(send_packet->buf,buf);
		write(sock, (char*)send_packet,sizeof(Packet));
		printf("명령어 전송이 완료되었습니다.\n");
	}
	history_Count++;
	free(send_packet);
	return;
}

void appendHistory(NODE *list, char *cmd)
{
	if(list->next == NULL)
	{
		NODE *newNode = malloc(sizeof(NODE));
		strcpy(newNode->cmd,cmd);
		newNode->next = NULL;
		list->next = newNode;
	}
	else 
	{
		NODE *cur = list;
		while(cur->next != NULL)
		{
			cur = cur->next;
		}
		NODE *newNode = malloc(sizeof(NODE));
		strcpy(newNode->cmd,cmd);
		newNode->next = NULL;
		cur->next = newNode;
	}
	return;
}

void showHistory(NODE *list)
{
	if(list->next != NULL)
	{
		int cnt = 1;
		NODE *cur = list->next;
		printf("===================================================\n");
		printf("내가 보낸 명령어 기록입니다.\n");
		while(cur != NULL)
		{
			printf("%d : %s\n",cnt,cur->cmd);
			cur = cur->next;
			cnt++;
		}
	}
	else
	{
		printf("고객님의 명령어 기록이 0입니다.\n");
		return;
	}
}

void saveHistory(NODE *list)
{
	int str_len=0;
	char cmd[20];
	char FilePath[30];

	FILE *fp;
	strcpy(cmd,"pwd");
	fp = popen(cmd,"r");
	if(fp == NULL)
	{
		perror("popen()실패 또는 없는 리눅스 명령어를 입력하였음.\n");
		return (void*)-1;
	}
	fgets(FilePath,30,fp);
	pclose(fp);

	str_len = strlen(FilePath);
	FilePath[str_len-1] = '/';

	FILE* stream;
	NODE* pHead = list;
	strcat(FilePath,name);
	strcat(FilePath,".dat");
	//printf("FilePath : %s\n",FilePath);

	stream = fopen(FilePath, "wb");
	if (stream == NULL) 
	{
		error_handling("파일 스트림 생성 실패\n");
		return;
	}
	fwrite(&history_Count, sizeof(history_Count), 1, stream);
	if(pHead!=NULL)
	{
		pHead = pHead->next;
		while (pHead != NULL) 
		{
			//printf("저장부분 명령어 : %s, 주소 : %p",pHead->cmd, pHead->next);
			fwrite(pHead, sizeof(NODE), 1, stream);
			pHead = pHead->next;
		}
	}
	fclose(stream);
}

NODE* loadHistory(NODE *list)
{
	int str_len=0;
	char cmd[20];
	char FilePath[30];

	FILE *fp;
	strcpy(cmd,"pwd");
	fp = popen(cmd,"r");
	if(fp == NULL)
	{
		perror("popen()실패 또는 없는 리눅스 명령어를 입력하였음.\n");
		return (void*)-1;
	}
	fgets(FilePath,30,fp);
	pclose(fp);

	str_len = strlen(FilePath);
	FilePath[str_len-1] = '/';

	strcat(FilePath,name);
	strcat(FilePath,".dat");

	//printf("FilePath : %s\n",FilePath);
	FILE* stream;
	NODE* pHead = malloc(sizeof(NODE));;
	NODE* pLast = NULL;

	stream = fopen(FilePath, "rb");
	if (stream == NULL) 
	{
		error_handling("현재 닉네임 이전 명령어기록 파일 없음.\n");
		return list;
	}
	fread(&history_Count, sizeof(history_Count), 1, stream);
	if (history_Count == 0) 
	{
		error_handling("이전 같은 닉네임의 명령어 송신 기록은 0입니다.\n");
		return list;
	}
	printf("현재 닉네임으로 저장 된 명령어 기록 수는 %d개 입니다.\n\n", history_Count);

	for (int i = 0; i < history_Count; i++) 
	{
		if(pHead->next == NULL)
		{
			NODE *newNode = malloc(sizeof(NODE));
			pHead->next = newNode;
			pLast = newNode;
			fread(newNode, sizeof(NODE), 1, stream);
			strcpy(pLast->cmd,newNode->cmd);
			pLast->next = NULL;
		}
		else 
		{
			NODE *newNode = malloc(sizeof(NODE));
			pLast->next = newNode;
			pLast = pLast->next;
			fread(newNode, sizeof(NODE), 1, stream);
			strcpy(pLast->cmd,newNode->cmd);
			pLast->next = NULL;
		}
	}
	fclose(stream);
	return pHead;
}

void freeHistory(NODE *list)
{
	while (list != NULL) 
	{
		NODE* cur = list;
		list = cur->next;
		free(cur);
	}
	return;
}

bool convertStatus(int sock)
{
	Packet *send_packet = malloc(sizeof(Packet));
	memset(send_packet,0,sizeof(Packet));
	if (statusFlag == true)
	{
		printf("===================================================\n");
		printf("현재 OnLine 상태입니다. OffLine 상태로 변환합니다.\n");
		printf("===================================================\n");
		strcpy(send_packet->buf,"OffLine");
		strcpy(send_packet->Separator,"Change_Status");
		strcpy(send_packet->MyName,name);
		if(write(sock, (char*)send_packet,sizeof(Packet))<=0)
		{
			error_handling("유저상태 전송 실패\n");
			exit(0);
		}
		printf("유저상태 전송 완료\n");
		statusFlag = false;
	}
	else 
	{
		printf("===================================================\n");
		printf("현재 OffLine 상태입니다. OnLine 상태로 변환합니다.\n");
		printf("===================================================\n");
		strcpy(send_packet->buf,"OnLine");
		strcpy(send_packet->Separator,"Change_Status");
		strcpy(send_packet->MyName,name);
		if(write(sock, (char*)send_packet,sizeof(Packet))<=0)
		{
			error_handling("유저상태 전송 실패\n");
			exit(0);
		}
		printf("유저상태 전송 완료\n");
		statusFlag = true;
	}
	free(send_packet);
	return statusFlag;
}
