#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#define SIZE 1024
#define FTP_PORT 21

#define PWD "PWD\n"
#define CWD "CWD "
#define LIST "LIST\n"
#define RETRIEVE "RETR "
#define STORE "STOR "
#define QUIT "QUIT\n"
#define EPSV "EPSV\n"

void oneSend(int sock, char data[SIZE]);
void oneRecv(int sock, char recv_data[SIZE]);
void totalRecv(int sock, int isWrite, FILE *fp);

void printMenu();
void signIn(int sock);
void listDirectory(int sock);
void printWorkingDirectory(int sock);      
void changeWorkingDirectory(int sock);
void uploadFile(int sock);
void retrieveFile(int sock);
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

void makeDecision(int *sock, char *userChoice)
{
    //process userChoice to a number
    //remove newline character and replace with \0
    userChoice[strlen(userChoice) - 1] = '\0';
    int userChoiceInt = atoi(userChoice);
    
    // Validify the user input
    if (userChoiceInt < 1 || userChoiceInt > 8 || strlen(userChoice) > 1)
    {
        printf("\nPlease key in a valid decision\n");

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
        case 5:
            uploadFile(*sock); 
            break;
        case 6:
            retrieveFile(*sock);
            break;
        case 7:
            ftpQuit(*sock);
            break;

    }

}

// this does 1 send call
void oneSend(int sock, char data[SIZE])
{
    send(sock, data, strlen(data),0);

}

// this does 1 receive call, print data
void oneRecv(int sock, char recv_data[SIZE])
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

// totalRecv will print/write all data till EOF
void totalRecv(int sock, int isWrite, FILE *fp)
{
    int numBytes;
    char buffer[SIZE] = "";

    // declare timeout constants
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec  = 900000; //0.9 seconds

    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
    {
        perror("error in sockopt\n");
    }

    while (1)
    {
        numBytes = recv(sock, buffer, SIZE-1, 0);
        printf("RECEIVED NUMBER OF BYTES: %d \n", numBytes);
        if (numBytes <= 0) // if no more data
        {
            break;
        }
        if (isWrite == 1)
        {
            // write to file
            fwrite(buffer, 1, sizeof(buffer), fp);
        }
        else
        {
            printf("%s", buffer);
            fflush(stdout);
        }

        memset(buffer, 0, SIZE);
    }

}

void totalSend(int sock, FILE *fp)
{
    int numBytesRead = 0;
    int numBytesSend = 0;
    char buffer[SIZE] = "";

    // declare timeout constants
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec  = 900000; //0.9 seconds

    if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
    {
        perror("error in sockopt\n");
    }

    while (1)
    {

        // read and send
        numBytesRead = fread(buffer, 1, sizeof(buffer), fp);
        
        if (numBytesRead <= 0) // if no more data to be read
        {
            break;
        }
        
        printf("%s", buffer);
        fflush(stdout);

        numBytesSend = send(sock, buffer, sizeof(buffer), 0);
        printf("SEND NUMBER OF BYTES: %d \n", numBytesSend);
        if (numBytesSend <= 0) // if no more data to be sent
        {
            break;
        }
        memset(buffer, 0, SIZE);
    }

}

// This functions connnect to a FTP server
// Returns the socket descriptor
int connectFtpServer(int port)
{
    int sock;  
    char recv_data[SIZE];
    struct hostent *host;
    struct sockaddr_in server_addr;  

    //host = gethostbyname("ftp.ietf.org");
    host = gethostbyname("localhost");

    //create a Socket structure   - "Client Socket"
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;     
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

    return sock;
}

void signIn(int sock)
{
    char username[512] = "USER ";
    char password[512] = "PASS ";
    char buffer[512] = "";
    char recv_data[SIZE] = "";
    
    oneRecv(sock, recv_data); //to receive connection success message
    printf("starting connection to FTP server\n");

    memset(buffer, 0, 512);
    printf("Enter your user: ");
    fgets(buffer, 512, stdin);
    strcat(username, buffer);
    oneSend(sock, username);
    oneRecv(sock, recv_data);

    memset(buffer, 0, 512);
    printf("Enter your password: ");
    fgets(buffer, 512, stdin);
    strcat(password, buffer);
    oneSend(sock, password);
    oneRecv(sock, recv_data);
}

void printWorkingDirectory(int sock)
{
    char recv_data[SIZE];
    oneSend(sock, PWD);
    oneRecv(sock, recv_data);
}

void changeWorkingDirectory(int sock)
{
    //TODO could overflow, what if more tha 512
    char recv_data[SIZE]; 
    char userInput[SIZE] = CWD;
    char buffer[512];

    printf("Which directory do you want to change to? : ");
    fflush(stdout);
    fgets(buffer, SIZE, stdin);
    
    strcat(userInput, buffer);
    printf("%s", userInput);
    oneSend(sock, userInput);
    oneRecv(sock, recv_data);
    
}

void listDirectory(int sock)
{
    char recv_data[SIZE];
    int port_issued = enterEpsvMode(sock); 

    oneSend(sock, LIST);

    int processId = fork();

    // if childprocess, connect to new port
    if (processId == 0)
    {
        // close the main connection socket
        close(sock);
        int connectionSocket = connectFtpServer(port_issued);    
        
        //receive and print output
        totalRecv(connectionSocket, 0, NULL);

        // close socket and terminate process
        close(connectionSocket);
        exit(0);
    }

    else 
    {
        oneRecv(sock, recv_data); // 150 ascii message
        wait();
        totalRecv(sock, 0, NULL); // transfer complete message

    }
}

void uploadFile(int sock)
{
    char recv_data[SIZE] = "";
    char userInput[SIZE] = STORE;
    char filename[512] = "";

    printf("Which file do you want to upload? : ");
    fflush(stdout);
    fgets(filename, SIZE, stdin);

    strcat(userInput, filename); // put testfile.txt
    printf("%s\n", userInput);
    fflush(stdout);

    // replace filename last char with \0
    filename[strlen(filename) - 1] = '\0';

    // check if file exists locally TODO

    // enter epsv
    int portIssued = enterEpsvMode(sock);
    oneSend(sock, userInput);
    
    int processId = fork();

    if(processId == 0)
    {
        // close the main connection socket
        close(sock);
        connectFtpServer(portIssued);

        // open file to read from
        FILE *fp = fopen(filename, "r");
        totalSend(sock, fp);
        fclose(fp);
        exit(0);
    }
    else
    {
        oneRecv(sock, recv_data); // 150 msg - okay to send data
        if (strstr(recv_data, "150"))
        {
            wait();
            oneRecv(sock, recv_data); // 226 msg - transfer complete
        }
        else {
            // kill children process
            kill(processId, SIGKILL);
        } 
    }

    printf("End of uploading file\n");
    fflush(stdout);
}

void retrieveFile(int sock)
{
    //TODO could overflow, what if more tha 512
    char recv_data[SIZE]; 
    char userInput[SIZE] = RETRIEVE;
    char filename[512];

    printf("Which file do you want to download? : ");
    fgets(filename, SIZE, stdin);
    
    strcat(userInput, filename);
    // replace filename last char with \0
    filename[strlen(filename) - 1] = '\0';
    
    int portIssued = enterEpsvMode(sock);
    oneSend(sock, userInput);
    
    int processId = fork();

    if(processId == 0)
    {
        // close the main connection socket
        close(sock);
        connectFtpServer(portIssued);

        // open file to write
        FILE *fp = fopen(filename, "w");
        totalRecv(sock, 1, fp); // to write full data to file
        fclose(fp);
        exit(0);
    }
    else
    {
        oneRecv(sock, recv_data); // 150 msg - opening ascii mode
        if (strstr(recv_data, "150"))
        {
            wait();
            oneRecv(sock, recv_data); // 226 msg - transfer complete
        }
        else {
            // kill children process
            kill(processId, SIGKILL);
        } 
    }
    
}

int enterEpsvMode(int sock)
{
    char ftpType[6] = EPSV;
    char recv_data[SIZE] = "";

    oneSend(sock, ftpType);
    oneRecv(sock, recv_data); // 229 msg - entering epsv

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
    printf("\n*******************************************\n");
    printf("**************Mini-FTP client**************\n");
    printf("*******************************************\n");
    printf("What do you feel like doing today?\n");
    printf("1.Connect to FTP server\n");
    printf("2.Print Working Directory\n");
    printf("3.Change Working Directory\n");
    printf("4.List all files\n");
    printf("5.Upload File\n");
    printf("6.Download File\n");
    printf("7.Quit\n");
    printf("Enter Optiooooooooooooooooon : ");
    fflush(stdout);

}

void ftpQuit(int sock)
{
    char recv_data[SIZE] = "";

    oneSend(sock, QUIT);
    oneRecv(sock, recv_data); //must receive goodbye message
    exit(0);
}
