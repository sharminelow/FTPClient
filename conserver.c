/* Demo TCP ECHO Server Program 
 (c) Author: Bhojan Anand
 Last Modified: 2011 Feb
 Course: CS3204L/CS2105
 School of Computing, NUS
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>


int main()    //AaBee TCP Concurrent Server
{
        int sock, connected, bytes_recieved , true = 1;  
        char reply[1024] , recv_data[1024];       

        struct sockaddr_in server_addr,client_addr;    
        unsigned int sin_size;
		int processID; //for concurrent server
	
		//Create a "Listening socket" or "Server socket"
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("Socket");
            exit(1);
        }

        if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&true,sizeof(int)) == -1) {
            perror("Setsockopt");
            exit(1);
        }
        
        server_addr.sin_family = AF_INET;         
        server_addr.sin_port = htons(5000);     
        server_addr.sin_addr.s_addr = INADDR_ANY; 
        bzero(&(server_addr.sin_zero),8); 
		
		//bind the socket to local Internet Address (IP and PORT).  
		
        if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr))
                                                                       == -1) {
            perror("Unable to bind");
            exit(1);
        }

	    //Listening for connections from client. 
	    //Maximum 5 connection requests are accepted.
        if (listen(sock, 5) == -1) {
            perror("Listen");
            exit(1);
        }
		
	    printf("\nMy TCP ECHO Server is Waiting for client on port 5000");
        fflush(stdout);


        while(1)
        {  

            sin_size = sizeof(struct sockaddr_in);

			//Accept connection request from the client and create new socket (name:connected) for the client
			//"Connect socket"
            connected = accept(sock, (struct sockaddr *)&client_addr,&sin_size);
			
            processID = fork();
			if (processID == 0)   //if Child process
			{	
			printf("\n I got a connection from (%s , %d)",
                   inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
				//Process data. Here we, prepare ECHO reply
			    strcpy(reply, "");
				strcat(reply, "ECHO FROM SERVER: dummy line");
				strcat(reply, recv_data);
				
				//send reply to client
			    send(connected, reply,strlen(reply), 0);
                
			close(sock);  //close server socket or listening socket
            
			while (1)
            {
			  //receive data from the client
			  bytes_recieved = recv(connected,recv_data,1024,0);
              recv_data[bytes_recieved] = '\0';
			  printf("\n RECIEVED DATA = %s " , recv_data);
			  fflush(stdout);
				
			  //if the data is q or Q, break the loop and exit
			  if (strcmp(recv_data , "q") == 0 || strcmp(recv_data , "Q") == 0)
              {
                break;
              }

              else    
			  {
				//Process data. Here we, prepare ECHO reply
			    strcpy(reply, "");
				strcat(reply, "ECHO FROM SERVER: ");
				strcat(reply, recv_data);
				
				//send reply to client
			    send(connected, reply,strlen(reply), 0);
			  }
            }
			close(connected); //close connect socket used for the client 
				printf("\n Connection is closed with (%s , %d)",
					   inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
	
			exit(0); //Close child process 
			}  //End of child process
			
			close(connected);   //Now we are in parent process. close connect socket (used for the clinet) in parent process
			
        }       
      //close server socket 
      close(sock);
      return 0;
} 
