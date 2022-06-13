#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
	int a;
	int b;
	int c;
	int d;
	char *name;
}Packet;

int main()
{
	//Packet Packet;
	printf("%d\n",sizeof(Packet));

	Packet *packet = malloc(sizeof(Packet));
	printf("%d\n",sizeof(packet));

	Packet.name = (char*)malloc(sizeof(char*20));
	strcpy(Packet->name,"ohjiwoo");
	strcpy(packet->name,Packet.name);

	printf("%d\n",sizeof(Packet));
	printf("%d\n",sizeof(packet));

	return 0;
}
