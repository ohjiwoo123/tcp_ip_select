#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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


int main()
{
	//Packet Packet;
	printf("%d\n",sizeof(Packet));

	Packet *packet = malloc(sizeof(Packet));
	printf("%d\n",sizeof(packet));

	printf("%d\n",sizeof(Packet));
	printf("%d\n",sizeof(packet));

	return 0;
}
