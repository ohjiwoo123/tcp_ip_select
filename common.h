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

#define BUF_SIZE 1024

#define NOT_CONNECTED_ERROR "NOT_CONNECTED_ERROR"
#define DUPLICATE_NICKNAME_ERROR "DUPLICATE_NICKNAME_ERROR"

pthread_mutex_t mutx;

#pragma pack(push,1)
typedef struct
{
	char ip_Address[16];
	char nickName[20];
	char user_Status[10];
	int port;
	char buf[1024];
}user_ManageMent;

typedef struct
{
	char ip_Address[16];
	int port;
	int file_Size;
	char separator[20];
	char my_Name[20];
	char target_Name[20];
	char buf[1024];
}packet;
#pragma pack(pop)

int print_Ui();
void error_Handling(char* message);

void error_Handling(char* buf)
{
	fputs(buf, stderr);
	fputc('\n', stderr);
	exit(1);
}

