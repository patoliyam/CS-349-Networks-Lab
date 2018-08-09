#include <stdio.h> 
#include <string.h>   
#include <stdlib.h> 
#include <errno.h> 
#include <unistd.h>   
#include <arpa/inet.h>    
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/time.h> 
#include <signal.h>

int main(int argc, char *argv[])
{
    char* ip = argv[1];
    char *p = argv[2];
    int port = atoi(p);
    printf("IP : %s, PORT: %d\n",ip,port );
    struct sockaddr_in address;
    int valread,sock;
    struct sockaddr_in serv_addr;
    char hello[20];
    char buffer[1024] = {0};
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }
  	printf("Socket Descriptor: %d\n",sock);
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
      
    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, ip, &serv_addr.sin_addr)<=0) 
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }
    while(1)
    {
        int type;
        printf("\n\t0: For product purcase \n\t1:End Purchase \n\n\tChoose :");
        scanf("%d",&type);
        if(type == 0)
        {
        	char upccode[4];
        	char quantity[20];
        	char datatosend[25];
        	printf("\tEnter upccode :");
        	scanf("%s",upccode);
        	printf("\tEnter quantity :");
        	scanf("%s",quantity);
        	sprintf(datatosend,"0,%s,%s",upccode,quantity);
        	int temp = strlen(upccode)+ strlen(quantity)+3;
        	datatosend[temp] = '\0';
            printf("\n\tRequest we will be sending is %s\n",datatosend);
            if( send(sock , datatosend, temp+1, 0) != temp+1)  
            {  
                perror("\terror in send in client");  
            } 
	        printf("\n\tRequest of 0 type sent\n");
            memset(buffer, 0, sizeof (buffer));    
	        if ((valread = read( sock , buffer, 1024))==0)
            {
                printf("\terror in client while reading response from server\n");
            }
	        printf("\nRESPONSE FROM SERVER : %s\n",buffer );
            buffer[0]='\0';
        }
        else if(type == 1)
        {
            char datatosend[25];
            sprintf(datatosend,"1");
            datatosend[1]='\0';
            printf("\n\tRequest we will be sending is %s\n",datatosend);
            if( send(sock , datatosend, 2, 0) != 2 )  
            {  
                perror("\terror in send in client");  
            } 
            printf("\n\tRequest of 1 type sent\n");
            memset(buffer, 0, sizeof (buffer));    
            if ((valread = read( sock , buffer, 1024))==0)
            {
                printf("\terror in client while reading response from server\n");
            }
            printf("\nRESPONSE FROM SERVER : %s\n",buffer);
            buffer[0]='\0';
            printf("\n%s\n\n","***Your connection will be closed now****" );
        	close(sock);
        	break;
        }

    }
    return 0;
}