//Example code: A simple server side code, which echos back the received message.
//Handle multiple socket connections with select and fd_set on Linux 
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
#include <ctype.h>
    
#define TRUE   1 
#define FALSE  0 
// #define PORT 8888 
    

int client_socket[30],total_amt[30],PORT;

int max_clients = 30,master_socket;

int isnumber(char * in)
{
    for (int i = 0; in[i]!='\0'; ++i)
    {
        if(!isdigit(in[i]))
        {
            return 0;
        }
    }
    return 1;
}

void sigintHandler(int sig_num)
{
    signal(SIGINT, sigintHandler);
    for (int i = 0; i < max_clients; ++i)
    {
        if(client_socket[i]!=0)
        {
            printf("Closing the socket :%d\n", client_socket[i]);
            close(client_socket[i]);
            total_amt[i] = 0;
        }
    }
    close(master_socket);
}

int getPrice(char* product_id)
{
    FILE *products;
    products = fopen("./products.txt","r");
    char UPC_code[4],line[200];
    int price;
    while(fgets(line,100,products)!=NULL)
    {
        sscanf(line, "%[^,],%d,%*s",UPC_code,&price);
        if(strcmp(UPC_code,product_id)==0)
        {
            return price;
        }
    }
    return -1;    
}

char * getName(char* product_id)
{
    FILE *products;
    products = fopen("./products.txt","r");
    char UPC_code[4],line[200];
    char *name;
    name = (char *)malloc(100*sizeof(char));
    int price;
    while(fgets(line,100,products)!=NULL)
    {
        sscanf(line, "%[^,],%d,%s",UPC_code,&price,name);
        if(strcmp(UPC_code,product_id)==0)
        {
            return name;
        }
    }
    return NULL;
}

int main(int argc , char *argv[])  
{ 

    if(argv[1])
    {
        char* port = argv[1];
        PORT = atoi(port);
    }
    else
    {
        PORT = 8080;
    }
    signal(SIGINT, sigintHandler);

    int opt = TRUE;  
    int addrlen , new_socket , activity, i , valread , sd , max_sd;  
    struct sockaddr_in address;  
        
    char buffer[1025];  //data buffer of 1K 
        
    //set of socket descriptors 
    fd_set readfds;  
        
    //a message 
    char message[100] ;  
    
    //initialise all client_socket[] to 0 so not checked 
    for (i = 0; i < max_clients; i++)  
    {  
        client_socket[i] = 0;  
    }  
        
    //create a master socket 
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)  
    {  
        perror("socket failed");  
        exit(EXIT_FAILURE);  
    }  
    
    //set master socket to allow multiple connections , 
    //this is just a good habit, it will work without this 
    // int setsockopt(int sockfd, int level, int optname,const void *optval, socklen_t optlen);
	//The SO_REUSEADDR socket option allows a socket to forcibly bind to a port in use by another socket.

    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )  
    {  
        perror("setsockopt");  
        exit(EXIT_FAILURE);  
    }  
    
    //type of socket created 
    // INADDR_ANY is used when you don't need to bind a socket to a specific IP. When you use this value as the address when calling bind() , the socket accepts connections to all the IPs of the machine.
    address.sin_family = AF_INET;  
    address.sin_addr.s_addr = INADDR_ANY;  
    address.sin_port = htons( PORT );  
        
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)  
    {  
        perror("bind failed");  
        exit(EXIT_FAILURE);  
    }  
    printf("Listener on port %d \n", PORT);  
        
    if (listen(master_socket, max_clients) < 0)  
    {  
        perror("listen");  
        exit(EXIT_FAILURE);  
    }  
        
    //accept the incoming connection 
    addrlen = sizeof(address);  
    puts("Waiting for connections ...");  
        
    while(TRUE)  
    {  
        //clear the socket set 
        FD_ZERO(&readfds);  
    
        //add master socket to set 
        FD_SET(master_socket, &readfds);  
        max_sd = master_socket;  
            
        //add child sockets to set 
        for ( i = 0 ; i < max_clients ; i++)  
        {  
            //socket descriptor 
            sd = client_socket[i];  
                
            //if valid socket descriptor then add to read list 
            if(sd > 0)  
                FD_SET( sd , &readfds);  
                
            //highest file descriptor number, need it for the select function 
            if(sd > max_sd)  
                max_sd = sd;  
        }  
    
        //wait for an activity on one of the sockets , timeout is NULL , 
        //so wait indefinitely
        //int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
 
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);  
      
        if ((activity < 0) && (errno!=EINTR))  
        {  
            printf("select error");  
        }  
            
        //If something happened on the master socket , 
        //then its an incoming connection

        char recvdata[25],ufccode[4],qty[10];
        int type,quantity=0,price=0;
        char * name;
        name = (char *)malloc(100*sizeof(char));
        if (FD_ISSET(master_socket, &readfds))  
        {  
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)  
            {  
                // perror("accept");  
                exit(EXIT_FAILURE);  
            }  
            memset(recvdata,0,sizeof(recvdata));
            read(new_socket,recvdata,25);
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));  
            printf("\t\tData from client : %s\n",recvdata);        
            sscanf(recvdata,"%d,%[^,],%s",&type,ufccode,qty);
            quantity = atoi(qty);
            int flag = 1;
            if (!isnumber(qty))
            {
                flag = 0;
            }

            printf("in first request %d,%s,%d\n",type,ufccode,quantity);
          	if(type == 0)
          	{
                price = getPrice(ufccode);
                name = getName(ufccode);
                if(price>=0 && name!=NULL && quantity >=0 && flag)
          		{
                    memset(message, 0, sizeof (message));
                    strcpy(message, "0,");
          			char pricetemp[10];
                    snprintf (pricetemp, sizeof(pricetemp), "%d",price);
                    // printf("price of current upc code %s\n",pricetemp);   
                    strcat(message,pricetemp);
                    // printf("name of current upc code %s\n",name);   
                    strcat(message,",");
                    strcat(message,name);
                    printf("\t\tmessage to send : %s\n",message);
          		}
          		else
          		{
          			if(quantity<0){
                        strcpy(message, "1,PROTOCOL ERROR: quantity can't be negative\n");
                    }
                    else if(flag==0){
                        strcpy(message, "1,PROTOCOL ERROR: quantity should be int type\n");
                    }
                    else{
                        strcpy(message, "1,UPC is not found in database\n");
                    }
          		}
          		if( send(new_socket, message, strlen(message), 0) != strlen(message) )  
	            {  
	                perror("send");  
	            }  
	            for (i = 0; i < max_clients; i++)  
	            {  
	                //if position is empty 
	                if( client_socket[i] == 0 )  
	                {  
	                    client_socket[i] = new_socket;
                        if( price!=-1 && quantity>=0 )  
                        {
                            total_amt[i] += price*quantity;
                        }
	                    // printf("Adding to list of sockets as  %d and initialise total amt as %d\n" , i,total_amt[i]);  
	                    break;  
	                }  
            	}  
          	}
          	else if(type == 1)
          	{
                memset(message, 0, sizeof (message));
                strcpy(message, "0,0");
                printf("\t\tmessage to send %s\n",message );
      			if( send(new_socket, message, strlen(message), 0) != strlen(message) )  
	            {  
	                perror("send");  
	            }  
          	}
            puts("Response sent to client successfully");  
        }  
        //else its some IO operation on some other socket
        for (i = 0; i < max_clients; i++)  
        {  
            sd = client_socket[i];  
                
            if (FD_ISSET( sd , &readfds))  
            {  
                memset(buffer,0,sizeof(buffer));
                if ((valread = read(sd,buffer,1024)) == 0)  
                {  
                    //Somebody disconnected , get his details and print 
                    getpeername(sd , (struct sockaddr*)&address ,(socklen_t*)&addrlen);  
                    printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));  
                    close( sd );  
                    client_socket[i] = 0;
                    total_amt[i]=0;

                }                      
                else
                {  
                    buffer[valread] = '\0';  
                    printf("\t\tData from client : %s\n",buffer);
            		// sscanf(buffer,"%d,%s,%d",&type,ufccode,&quantity);
                    sscanf(buffer,"%d,%[^,],%s",&type,ufccode,qty);
                    quantity = atoi(qty);
                    int flag = 1;
                    if (!isnumber(qty))
                    {
                        flag = 0;
                    }
                    // printf("in second request and further %d,%s,%d\n",type,ufccode,quantity);
		          	if(type == 0)
		          	{
                        name = getName(ufccode);
		          		price = getPrice(ufccode);
                        if(price >=0 && name!=NULL && quantity>=0 && flag)
		          		{
                            memset(message, 0, sizeof (message));
		          			strcpy(message,"0,");
                            printf("price: %d\n",price);

		          			char pricetemp[10];
		          			// itoa(price,pricetemp,10);
                            snprintf (pricetemp, sizeof(pricetemp), "%d",price);
                            // printf("price of current upc code : %s\n",pricetemp);   
		          			strcat(message,pricetemp);
                            // printf("name of current upc code %s\n",name);   
                            strcat(message,",");
                            strcat(message,name);
                            printf("\t\tmessage to send : %s\n",message);
		          			total_amt[i] = total_amt[i] + price*quantity;
                            // printf("total_amt is as of now %d\n",total_amt[i]);
		          		}
		          		else
		          		{
		          			if(quantity<0){
                                strcpy(message, "1,PROTOCOL ERROR: quantity can't be negative\n");
                            }
                            else if(flag==0){
                                strcpy(message, "1,PROTOCOL ERROR: quantity should be int type\n");
                            }
                            else{
                                strcpy(message, "1,UPC is not found in database\n");
                            }
		          		}
		          		if( send(sd, message, strlen(message), 0) != strlen(message) )  
			            {  
			                perror("send");  
			            }  
                        message[0] = '\0';
		          	}
		          	else if(type == 1)
		          	{
		      			memset(message, 0, sizeof (message));
                        strcpy(message, "0,");
	          			char totalamttemp[10];
	          			// itoa(total_amt[i],totalamttemp,10);
                        snprintf (totalamttemp, sizeof(totalamttemp), "%d",total_amt[i]);
	          			strcat(message,totalamttemp);
                        printf("\t\tmessage to send %s\n",message );
		      			if( send(sd,  message, strlen(message), 0) != strlen(message) )  
			            {  
			                perror("send");  
			            }  
		          	}
            		puts("Response sent to client successfully");  
                }  
            }  
        }  
    }  
        
    return 0;  
    
}  