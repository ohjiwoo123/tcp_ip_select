#include "common.h"
#include "client_func.h"

int main(int argc, char* argv[])
{
	int fd_Max, str_Len, fd_Num, i, sock;

	char message[BUF_SIZE];
	struct sockaddr_in serv_Adr;
	struct timeval timeout;
	fd_set reads, cpy_Reads;

	pthread_t print_Ui_Thread;
	pthread_mutex_init(&mutx, NULL);

	if (argc != 4)
	{
		printf("Usage : %s <IP> <port> <name>\n", argv[0]);
		exit(1);
	}
	if (strlen(argv[3]) > 19)
	{
		printf("닉네임 최대 글자 수 는 19글자 입니다.\n");
		printf("프로그램을 종료합니다.\n");
		exit(1);
	}
	sprintf(name, "%s", argv[3]);

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1)
		error_Handling("socket() error\n");

	memset(&serv_Adr, 0, sizeof(serv_Adr));
	serv_Adr.sin_family = AF_INET;
	serv_Adr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_Adr.sin_port = htons(atoi(argv[2]));

	if (connect(sock, (struct sockaddr*)&serv_Adr, sizeof(serv_Adr)) == -1)
	{
		error_Handling("connect() error!\n");
		exit(1);
	}
	else
	{
		puts("Connected...........");
	}
	user_ManageMent* user_Packet = malloc(sizeof(user_ManageMent));
	memset(user_Packet, 0, sizeof(user_ManageMent));
	strcpy(user_Packet->ip_Address, argv[1]);
	user_Packet->port = serv_Adr.sin_port;
	strcpy(user_Packet->nickName, name);
	strcpy(user_Packet->user_Status, "Online");
	status_Flag = true;
	if (write(sock, (char*)user_Packet, sizeof(user_ManageMent)) <= 0)
	{
		error_Handling("Sending UserInfo Failed!!\n");
	}

	free(user_Packet);

	FD_ZERO(&reads);
	FD_SET(0, &reads);
	FD_SET(sock, &reads);
	fd_Max = sock;

	while (1)
	{
		cpy_Reads = reads;
		//timeout.tv_sec=5;
		//timeout.tv_usec=5000;

		if (pthread_create(&print_Ui_Thread, NULL, t_Print_Ui, (void*)&sock) != 0)
		{
			error_Handling("printUIThread create error\n");
			continue;
		}

		if ((fd_Num = select(fd_Max + 1, &cpy_Reads, 0, 0, NULL)) == -1)
		{
			perror("select 함수 에러 내용 : ");
			break;
		}

		if (fd_Num == 0)
			continue;

		if (FD_ISSET(sock, &reads))
		{
			handle_Connection(sock, &reads);
		}
	}
	close(sock);
	return 0;
}


void* handle_Connection(int sock, fd_set* reads)
{
	char name_Msg[BUF_SIZE];
	int str_Len = 0;

	packet* recv_Packet = malloc(sizeof(packet));
	memset(recv_Packet, 0, sizeof(packet));
	// 구조체 정보 먼저 받기, 커맨드인지 출력물인지 
	str_Len = read(sock, (char*)recv_Packet, sizeof(packet));
	if (str_Len == 0)
	{
		FD_CLR(sock, reads);
		close(sock);
		printf("\nclosed client: %d \n", sock);
		exit(1);
	}
	else
	{
		if (strcmp(recv_Packet->separator, "Command") == 0)
		{
			if (strcmp(recv_Packet->target_Name, name) == 0)
			{
				FILE* fp;
				fp = popen(recv_Packet->buf, "r");
				if (fp == NULL)
				{
					perror("popen()실패 또는 없는 리눅스 명령어를 입력하였음.\n");
					return NULL;
				}
				while (fgets(recv_Packet->buf, BUF_SIZE, fp))
				{
					//printf("\ninsideof command fgets func\n");
					packet* send_Packet = malloc(sizeof(packet));
					memset(send_Packet, 0, sizeof(packet));
					strcpy(send_Packet->separator, "Print_Result");
					strcpy(send_Packet->buf, recv_Packet->buf);
					strcpy(send_Packet->target_Name, recv_Packet->target_Name);
					strcpy(send_Packet->my_Name, recv_Packet->my_Name);
					if (write(sock, (char*)send_Packet, sizeof(packet)) <= 0)
					{
						close(sock);
						break;
					}
					free(send_Packet);
				}
				pclose(fp);
			}
			else
			{
				free(recv_Packet);
				return NULL;
			}
			free(recv_Packet);
			return NULL;
		}

		else if (strcmp(recv_Packet->separator, "Print_Result") == 0)
		{
			//printf("myname : %s\n",recv_packet->MyName);
			if (strcmp(recv_Packet->my_Name, name) == 0)
			{
				printf("\n%s from : %s", recv_Packet->buf, recv_Packet->target_Name);
			}
			else
			{
				free(recv_Packet);
				return NULL;
			}
		}

		else if (strcmp(recv_Packet->separator, "Message") == 0)
		{
			if (strcmp(recv_Packet->target_Name, "ALL") == 0)
			{
				printf("\nMessage : %s from : %s \n", recv_Packet->buf, recv_Packet->my_Name);
			}
			else if (strcmp(recv_Packet->target_Name, name) == 0)
			{
				printf("\n귓속말 Message : %s from : %s \n", recv_Packet->buf, recv_Packet->my_Name);
			}
		}

		else if (strcmp(recv_Packet->separator, "List") == 0)
		{
			printf("%s\n", recv_Packet->buf);
		}

		else if (strcmp(recv_Packet->separator, NOT_CONNECTED_ERROR) == 0)
		{
			if (strcmp(recv_Packet->my_Name, name) == 0)
			{
				printf("\n %s <- %s \n", recv_Packet->target_Name, recv_Packet->buf);
			}
		}

		else if (strcmp(recv_Packet->separator, DUPLICATE_NICKNAME_ERROR) == 0)
		{
			printf("해당 닉네임 : %s -> %s", name, recv_Packet->buf);
			printf("프로그램을 종료합니다.\n");
			exit(0);	// 정상종료 
		}

		else if (strcmp(recv_Packet->separator, "CMDHistory") == 0)
		{
			if (strcmp(recv_Packet->my_Name, name) == 0)
			{
				printf("\n %s \n", recv_Packet->buf);
			}
		}
	}
	free(recv_Packet);
	return NULL;
}

void* t_Print_Ui(void* arg)
{
	int sock = *((int*)arg);
	int menu_Num = 0;

	pthread_mutex_lock(&mutx);
	while ((menu_Num = print_Ui()) != 0)
	{
		switch (menu_Num)
		{
		case 1:
			get_List(sock);
			break;
		case 2:
			send_Message(sock);
			break;
		case 3:
			send_Cmd(sock);
			break;
		case 4:
			get_History(sock);
			break;
		case 5:
			disConnect(sock);
			break;
		case 6:
			convert_Status(sock);
			break;
		default:
			printf("메뉴의 보기에 있는 숫자 중에서 입력하세요.\n");
			break;
		}
	}
	pthread_mutex_unlock(&mutx);
}


int print_Ui()
{
	int input_Num = 0;
	// system("cls");
	printf("\n===================================================\n");
	printf("[1]유저목록보기\t [2] 메세지보내기\t [3]명령어보내기\n");
	printf("[4]명령어기록보기\t [5]연결종료하기 [6]OnLine/OffLine 전환\n");
	printf("===================================================\n");
	printf("번호를 입력하세요 : ");

	// 사용자가 선택한 메뉴의 값을 반환한다.
	scanf("%d", &input_Num);
	//getchar();
	//버퍼에 남은 엔터 제거용
	while (getchar() != '\n'); //scanf_s 버퍼 비우기, 밀림 막음
	if (input_Num > 6 || input_Num < 1)
	{
		input_Num = 7; // 0~2 사이의 메뉴 값이 아니라면 defalut로 보내기
	}
	return input_Num;
}

void clear_Input_Buffer()
{
	// 입력 버퍼에서 문자를 계속 꺼내고 \n를 꺼내면 반복을 중단
	while (getchar() != '\n');
	return;
}

void get_List(int sock)
{
	char buf[1024];
	packet* send_Packet = malloc(sizeof(packet));
	memset(send_Packet, 0, sizeof(packet));

	strcpy(send_Packet->separator, "List");
	write(sock, (char*)send_Packet, sizeof(packet));
	free(send_Packet);

	return;
}

void get_History(int sock)
{
	packet* send_Packet = malloc(sizeof(packet));
	memset(send_Packet, 0, sizeof(packet));
	strcpy(send_Packet->separator, "CMDHistory");
	strcpy(send_Packet->my_Name, name);
	write(sock, (char*)send_Packet, sizeof(packet));
	free(send_Packet);
	return;
}

void disConnect(int sock)
{
	printf("\n서버와의 접속이 종료됩니다.\n");
	exit(0);	// 정상종료 
}

void send_Message(int sock)   // send to all
{
	int clnt_sock = sock;
	packet* send_Packet = malloc(sizeof(packet));
	char name_Msg[BUF_SIZE];
	memset(send_Packet, 0, sizeof(packet));
	memset(name_Msg, 0, sizeof(name_Msg));
	printf("===================================================\n");
	printf("[1] 전체보내기\t [2] 귓속말보내기\t [3] 처음 화면으로 돌아가기\t\n");
	printf("===================================================\n");
	printf("번호를 입력하세요 : ");
	int index;
	scanf("%d", &index);

	strcpy(send_Packet->separator, "Message");
	strcpy(send_Packet->my_Name, name);

	if (index == 1)
	{
		printf("===================================================\n");
		printf("전체 보내기 모드입니다.\n");
		printf("===================================================\n");
		printf("보낼 메세지를 입력하세요 : ");
		clear_Input_Buffer();
		fgets(name_Msg, BUF_SIZE, stdin);
		strcpy(send_Packet->target_Name, "ALL");
		strcpy(send_Packet->buf, name_Msg);
		if (write(sock, (char*)send_Packet, sizeof(packet)) <= 0)
		{
			error_Handling("전체 메세지 전송실패\n");
		}
		printf("전체 메세지 보내기 전송 완료\n");
	}
	else if (index == 2)
	{
		printf("===================================================\n");
		printf("귓속말 보낼 닉네임을 입력하세요\n");
		char target_Name[20];
		scanf("%s", target_Name);
		if (strcmp(target_Name, name) == 0)
		{
			printf("자기 자신한테 귓속말을 보낼 수 없습니다.\n");
		}
		else
		{
			strcpy(send_Packet->target_Name, target_Name);
			clear_Input_Buffer();
			printf("===================================================\n");
			printf("귓속말 보낼 내용을 입력하세요\n");
			fgets(name_Msg, BUF_SIZE, stdin);
			strcpy(send_Packet->buf, name_Msg);
			if (write(sock, (char*)send_Packet, sizeof(packet)) <= 0)
			{
				error_Handling("귓속말 전송실패\n");
			}
			printf("귓속말 전송 완료\n");
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

	free(send_Packet);
	return;
}
void send_Cmd(int sock)   // send to all
{
	printf("===================================================\n");
	printf("명령어 보내기 모드입니다.\n");
	printf("===================================================\n");
	printf("명령어를 보낼 타겟 클라이언트 닉네임을 입력하세요 : ");

	packet* send_Packet = malloc(sizeof(packet));
	memset(send_Packet, 0, sizeof(packet));

	char target_Name[20];
	memset(target_Name, 0, sizeof(target_Name));
	scanf("%s", target_Name);
	if (strcmp(target_Name, name) == 0)
	{
		printf("자기 자신한테 귓속말을 보낼 수 없습니다.\n");
	}
	else
	{
		printf("===================================================\n");
		printf("상대방에게 보낼 명령어를 입력하세요 : ");
		char buf[20];
		scanf("%s", buf);
		strcpy(send_Packet->separator, "Command");
		strcpy(send_Packet->my_Name, name);
		strcpy(send_Packet->target_Name, target_Name);
		strcpy(send_Packet->buf, buf);
		write(sock, (char*)send_Packet, sizeof(packet));
		printf("명령어 전송이 완료되었습니다.\n");
	}
	free(send_Packet);
	return;
}

bool convert_Status(int sock)
{
	packet* send_Packet = malloc(sizeof(packet));
	memset(send_Packet, 0, sizeof(packet));
	if (status_Flag == true)
	{
		printf("===================================================\n");
		printf("현재 OnLine 상태입니다. OffLine 상태로 변환합니다.\n");
		printf("===================================================\n");
		strcpy(send_Packet->buf, "OffLine");
		strcpy(send_Packet->separator, "Change_Status");
		strcpy(send_Packet->my_Name, name);
		if (write(sock, (char*)send_Packet, sizeof(packet)) <= 0)
		{
			error_Handling("유저상태 전송 실패\n");
			exit(0);
		}
		printf("유저상태 전송 완료\n");
		status_Flag = false;
	}
	else
	{
		printf("===================================================\n");
		printf("현재 OffLine 상태입니다. OnLine 상태로 변환합니다.\n");
		printf("===================================================\n");
		strcpy(send_Packet->buf, "OnLine");
		strcpy(send_Packet->separator, "Change_Status");
		strcpy(send_Packet->my_Name, name);
		if (write(sock, (char*)send_Packet, sizeof(packet)) <= 0)
		{
			error_Handling("유저상태 전송 실패\n");
			exit(0);
		}
		printf("유저상태 전송 완료\n");
		status_Flag = true;
	}
	free(send_Packet);
	return status_Flag;
}

