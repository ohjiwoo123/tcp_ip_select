#include <stdbool.h>
#define NAME_SIZE 20

void get_History(int sock);
void disConnect(int sock);
void send_Cmd(int sock);
void clear_Input_Buffer();
void get_List(int sock);
void send_Message(int sock);
bool convert_Status(int sock);
void* handle_Connection(int sock, fd_set* reads);
void* t_Print_Ui(void* data);

char name[NAME_SIZE] = "[DEFAULT]";
char msg[BUF_SIZE];

bool status_Flag;