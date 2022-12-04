typedef struct
{
	char ip_Address[16];
	char nickName[20];
	char user_Status[10];
	int port;
	int sock_Num;
}socket_Info;
socket_Info socket_Info_Array[256];

typedef struct node
{
	char nickName[20];
	char cmd[20];
	int sock_Num;
	struct node* next;
}node;

typedef struct
{
	int sock;
	node* node;
}sock_And_Node;