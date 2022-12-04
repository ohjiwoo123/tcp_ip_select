#include "server_func.h"

void send_Cmd(int sock, packet* p, node* list)   // send to all
{
	char buf[1024];
	char num[10];
	int history_Count = 0;

	memset(buf, 0, sizeof(buf));
	//memset(p,0,sizeof(packet));
	int clnt_Sock = sock;
	strcat(buf, "명령어 : \n");

	if (list->next != NULL)
	{
		node* cur = list->next;
		while (cur != NULL)
		{
			if (strcmp(p->my_Name, cur->nickName) == 0)
			{
				history_Count++;
				sprintf(num, "%d", history_Count);
				strcat(buf, num);
				strcat(buf, " : ");
				strcat(buf, cur->cmd);
				strcat(buf, "\n");
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

	if (write(clnt_Sock, (char*)p, sizeof(packet)) <= 0)
	{
		error_Handling("Sending List Error\n");
	}
	return;
}

void send_Msg(int sock, packet* p)   // send to all
{
	int clnt_Sock = sock;
	for (int i = 0; i < clnt_Cnt; i++)
	{
		if (write(socket_Info_Array[i].sock_Num, (char*)p, sizeof(packet)) <= 0)
		{
			error_Handling("send error\n");
		}
	}
	return;
}

int print_Ui()
{
	int input_Num = 0;
	// system("cls");
	printf("===================================================\n");
	printf("서버 Start\n");
	printf("---------------------------------------------------\n");
	printf("[1] 연결현황출력\t [2] 강퇴 기능\t [3] 명령어기록보기\t\n");
	printf("===================================================\n");
	printf("번호를 입력하세요 : ");

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
	if (clnt_Cnt == 0)
	{
		printf("현재 접속한 인원이 없습니다.\n");
		return;
	}
	for (int i = 0; i < clnt_Cnt; i++)
	{
		printf("소켓번호 : %d, IP : %s, Port : %d, 닉네임 : %s, 유저상태 : %s\n", socket_Info_Array[i].sock_Num, socket_Info_Array[i].ip_Address, socket_Info_Array[i].port, socket_Info_Array[i].nickName, socket_Info_Array[i].user_Status);
	}
	return;
}

void disConnect()
{
	int index;
	int flag_Num = -1;
	printf("삭제할 소켓 번호를 입력하세요 :\n");
	scanf("%d", &index);
	for (int i = 0; i < clnt_Cnt; i++)
	{
		if (socket_Info_Array[i].sock_Num == index)
		{
			shutdown(index, SHUT_WR);
			flag_Num = index;
			break;
		}
	}
	if (flag_Num == -1)
	{
		printf("해당 소켓번호 : %d -> 현재 접속 목록에 없습니다.\n", index);
	}
	return;
}

void get_History(node* list)
{
	show_History(list);
	return;
}

void append_History(node* list, packet* p, int sock)
{
	if (list->next == NULL)
	{
		node* new_Node = malloc(sizeof(node));
		strcpy(new_Node->cmd, p->buf);
		strcpy(new_Node->nickName, p->my_Name);
		new_Node->sock_Num = sock;
		new_Node->next = NULL;
		list->next = new_Node;
	}
	else
	{
		node* cur = list;
		while (cur->next != NULL)
		{
			cur = cur->next;
		}
		node* new_Node = malloc(sizeof(node));
		strcpy(new_Node->cmd, p->buf);
		strcpy(new_Node->nickName, p->my_Name);
		new_Node->sock_Num = sock;
		new_Node->next = NULL;
		cur->next = new_Node;
	}
	return;
}

void show_History(node* list)
{
	int cnt = 0;
	int flag_Num = -1;
	char target_Name[20];
	printf("명령어 기록을 확인하고 싶은 닉네임을 입력하세요\n");
	scanf("%s", target_Name);
	for (int i = 0; i < clnt_Cnt; i++)
	{
		if (strcmp(socket_Info_Array[i].nickName, target_Name) == 0)
		{
			flag_Num = 1;
			break;
		}
	}
	if (flag_Num == -1)
	{
		printf("해당 닉네임(%s)은 접속 목록에 존재하지 않습니다.\n", target_Name);
		return;
	}
	if (list->next != NULL)
	{
		node* cur = list->next;
		printf("===================================================\n");
		printf("명령어 기록입니다.\n");
		//printf("list->nickName : %s , target_Name : %s list->cmd : %s\n",cur->nickName,target_Name,cur->cmd);
		while (cur != NULL)
		{
			if (strcmp(cur->nickName, target_Name) == 0)
			{
				++cnt;
				printf("%s의 %d번 명령어 기록: %s\n", cur->nickName, cnt, cur->cmd);
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
			printf("해당 %s 닉네임의 명령어 기록이 %d 입니다.\n", target_Name, cnt);
		}
	}
	else
	{
		printf("전체 명령어 기록이 0입니다.\n");
	}
	return;
}

void free_History(node* list)
{
	while (list != NULL)
	{
		node* cur = list;
		list = cur->next;
		free(cur);
	}
	return;
}

int search_Node(node* list, char* name, int history_Count)
{
	int count = history_Count;
	if (list->next != NULL)
	{
		node* cur = list->next;
		while (cur != NULL)
		{
			if (strcmp(name, cur->nickName) == 0)
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
		printf("%s 고객님의 명령어 기록이 0입니다.\n", name);
	}
	return count;
}

