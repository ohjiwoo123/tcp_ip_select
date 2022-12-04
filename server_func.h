#define MAX_CLNT 1024

void send_List(int sock, packet* p);
void show_History(node* list);
void free_History(node* list);
void get_History(node* list);
void append_History(node* list, packet* p, int sock);
void send_Msg(int sock, packet* p);
void send_Cmd(int sock, packet* p, node* list);
void* handle_Connection(int sock, fd_set* reads, node* list);
int search_Node(node* list, char* name, int history_Count);

/// Thread ///
void* t_Print_Ui(void* data);
/////////////

void disConnect();
void get_List();

extern int clnt_Cnt;
extern int clnt_Socks[MAX_CLNT];
