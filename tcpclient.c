#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#define SIZE 1024
#define FTP_PORT 21

#define PWD "PWD\n"
#define CWD "CWD "
#define LIST "LIST\n"
#define RETR "RETR\n"
#define STOR "STOR\n"
#define QUIT "QUIT\n"
#define EPSV "EPSV\n"

void printMenu();
void mySend(int sock, char data[SIZE]);
void myRecv(int sock, char recv_data[SIZE]);
void myRecvData(int sock);
void signIn(int sock);
void listDirectory(int sock);
void printWorkingDirectory(int sock);
void changeWorkingDirectory(int sock);
void ftpQuit(int sock);
void makeDecision(int *sock, char *userChoice); 
int enterEpsvMode(int sock);
int connectFtpServer(int port);

int main()

{
    int sock = 0;
    char userInput[SIZE];

    while(1)
    {
        //Input data from user through Standard Input device
        printMenu();

        // get user command
        fgets((userInput), sizeof(userInput), stdin);

        makeDecision(&sock, userInput);

    }

    return 0;
}

void makeDecision(int *sock, char* userChoice)
{
    //process userChoice to a number
    //remove newline character and replace with \0
    userChoice[strlen(userChoice) - 1] = '\0';
    int userChoiceInt = atoi(userChoice);
    
    // Validify the user input
    if (userChoiceInt < 1 || userChoiceInt > 7)
    {
        printf("Please key in a valid decision\n");

    }
    else if(*sock == 0 && userChoiceInt != 1)
    {
        printf("Please connect to a FTP server(choice 1) first\n");
        return;
    } 
    
    // Demultiplex decisions
    switch(userChoiceInt)
    {
        case 1:
            *sock = connectFtpServer(FTP_PORT);
            signIn(*sock);
            break;
        case 2:
            printWorkingDirectory(*sock); 
            break;
        case 3:
            changeWorkingDirectory(*sock);
            break;
        case 4:
            listDirectory(*sock); 
            break;
        case 7:
            ftpQuit(*sock);
            break;

    }

}

void mySend(int sock, char data[SIZE])
{
    send(sock, data, strlen(data),0);

}

void myRecv(int sock, char recv_data[SIZE])
{
    int numBytes;

    //always reset buffer to 0
    memset(recv_data, 0, SIZE);

    // get reply from server
    numBytes = recv(sock,recv_data,SIZE,0);
    recv_data[numBytes] = '\0';

    // print reply
    printf("\n%s", recv_data);
    fflush(stdout);
}

void myRecvData(int sock)
{
    int numBytes;
    char buffer[SIZE];

    /* Error checking
    //make socket non blocking
    fcntl(sock, F_SETFL, O_NONBLOCK);    
    */

    while (1)
    {
        numBytes = recv(sock, buffer, SIZE-1, 0);
        //printf("RECEIVED NUMBER OF BYTES: %d ", numBytes);
        printf("%s", buffer);
        fflush(stdout);
        memset(buffer, 0, SIZE);
        if (numBytes <= 512) // TODO 512 assumes that it is the last packet
        {
            break;
        }
    }

    /* make socket blocking
       int opts = fcntl(sock, F_GETFL);
       opts = opts & (~O_NONBLOCK);

       if (fcntl(sock,F_SETFL,opts) < 0) {
       perror("fcntl(F_SETFL)");
       exit(EXIT_FAILURE);
       }*/
}

// This functions connnect to a FTP server
// Returns the socket descriptor
int connectFtpServer(int port)
{
    int sock;  
    char recv_data[SIZE];
    struct hostent *host;
    struct sockaddr_in server_addr;  

    //host = gethostbyname("127.0.0.1");
    host = gethostbyname("ftp.ietf.org");

    //create a Socket structure   - "Client Socket"
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;     
    //server_addr.sin_port = htons(5000);   
    server_addr.sin_port = htons(port);   
    server_addr.sin_addr = *((struct in_addr *)host->h_addr);
    bzero(&(server_addr.sin_zero),8); 

    if (connect(sock, (struct sockaddr *)&server_addr,
                sizeof(struct sockaddr)) == -1) 
    {
        perror("Connect");
        exit(1);
    }

    printf("\nI am connected to (%s , %d)\n",
            inet_ntoa(server_addr.sin_addr),ntohs(server_addr.sin_port));
    fflush(stdout);

    //myRecv(sock, recv_data); //to receive success message from server
    myRecvData(sock);

    return sock;
}

void signIn(int sock)
{
    char username[512] = "USER ";
    char password[512] = "PASS ";
    char buffer[512] = "";
    char recv_data[SIZE] = "";
    printf("starting to connect to FTP server\n");

    memset(buffer, 0, 512);
    printf("Enter your user: ");
    fgets(buffer, 512, stdin);
    strcat(username, buffer);
    mySend(sock, username);
    myRecv(sock, recv_data);

    memset(buffer, 0, 512);
    printf("Enter your password: ");
    fgets(buffer, 512, stdin);
    strcat(password, buffer);
    mySend(sock, password);
    myRecv(sock, recv_data);
}

void printWorkingDirectory(int sock)
{
    char recv_data[SIZE];
    mySend(sock, PWD);
    myRecv(sock, recv_data);
}

void changeWorkingDirectory(int sock)
{
    //TODO could overflow, what if more tha 512
    char recv_data[SIZE]; 
    char userInput[SIZE] = CWD;
    char buffer[512];

    printf("Which directory do you want to change to?\n");
    fgets(buffer, SIZE, stdin);
    
    strcat(userInput, buffer);
    printf("%s", userInput);
    mySend(sock, userInput);
    myRecv(sock, recv_data);
    
}

void listDirectory(int sock)
{
    int port_issued = enterEpsvMode(sock); 

    mySend(sock, LIST);

    int processId = fork();

    // if childprocess, connect to new port
    if (processId == 0)
    {
        connectFtpServer(port_issued);    
        exit(0);
    }

    // Let parent sleep, else will flood terminal
    else 
    {
        myRecvData(sock); // 150 ascii message
        wait();
        myRecvData(sock); // transfer complete message
        //printf("children died, parent revived");
    }
}

int enterEpsvMode(int sock)
{
    char ftpType[6] = EPSV;
    char recv_data[SIZE] = "";

    mySend(sock, ftpType);
    myRecv(sock, recv_data);

    //Retrieve port number
    char *ret = strstr(recv_data, "|");
    char port[6];
    memcpy(port, &ret[3], 5);
    port[6] = '\0';
    int port_int = atoi(port);

    return port_int;
}

void printMenu()
{

    printf("\nWhat do you feel like doing today?\n1.Connect to FTP server\n2.Print Working Directory\n3.Change Working Directory\n4.List all files\n5.Upload File\n6.Download File\n7.Quit\n");
    printf("Enter Optiooooooooooooooooon : ");

}

void ftpQuit(int sock)
{
    char recv_data[SIZE] = "";

    mySend(sock, QUIT);
    myRecv(sock, recv_data); //must receive goodbye message
    exit(0);
}
