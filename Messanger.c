#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <windows.h>
#include <process.h>
#include <string.h>


#pragma comment(lib, "Ws2_32.lib")


    WSADATA wsaData;
    int iResult;
    unsigned __stdcall recieveMessage(void *arg);       // initalise listening 
    unsigned __stdcall recieveMessageServer(void *arg);       // initalise listening for Server varient
    struct addrinfo *result = NULL, *ptr = NULL, hints;    // create 3 addrinfo strcuts - or 2 pointers and 1 full, these hold adress infomation for connection
    typedef struct { // I wanna get client addresses so do it with a pointer to a nested struct
        SOCKET *ConnectSocket;
        struct sockaddr_in *clientAddr;
    } ReceiveThreadArgs;

int main(){

    int versionCheck = 0;
    char type[32];
    char ipAddress[20];



    // initalise Ws2 DLL and winsock2 lib, check for errors
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult !=0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    } else {
        printf("WSAStartup Successful\n");
    }



    ZeroMemory(&hints, sizeof(hints)); // ZeroMemory is a macro that fills a block of memory with zeros, so each varible in the structure is set to 0, this is important as it prevents garbage values from being used in the struct.
    hints.ai_family     = AF_INET; // defines if connection accepts IPv4 or IPv6 - I chose INET so it can only be v4
    hints.ai_socktype   = SOCK_DGRAM; // Telling it to use UDP socket
    hints.ai_protocol   = IPPROTO_UDP; // Telling it to use UDP protocol

    printf("Adress Objects Created\n");

    #define DEFAULT_PORT "27015"






    printf("please enter client or server to choose connection type: ");
    if (fgets(type, sizeof(type), stdin) != NULL) {
    type[strcspn(type, "\n")] = '\0';
    } 

    printf("you've chosen %s\n", type);

    versionCheck = (strcmp(type, "client") == 0) ? 0 : 1;

    printf("version check %i\n", versionCheck);



    // Server Side Code ------------------------------------------------------------------------------------------------------------------------------



    if (versionCheck == 1){

        iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
        if (iResult !=0){
            printf("getaddrinfo failed: %d\n", iResult);
            WSACleanup();
            return 1;
        }

        printf("Generating Socket...\n");
        // generate a Socket
        SOCKET ConnectSocket = INVALID_SOCKET;

        ptr = result;

        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

        // checking socket is valid
        if (ConnectSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
        }


        // Binding to socket
        iResult = bind(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            printf("Failed to bind to socket: %ld\n", WSAGetLastError());
            freeaddrinfo(result);
            WSACleanup();
            return 1;
        }

        printf("Successfully Binded\n");

        struct sockaddr_in clientAddr = {0}; //

        ReceiveThreadArgs args = {
        .ConnectSocket = &ConnectSocket,
        .clientAddr = &clientAddr
        };

        // run a thread that listens for incoming messages.
        uintptr_t recieveThreadServer = _beginthreadex(
        NULL,               // Default security attributes
        0,                  // Default stack size
        recieveMessageServer,    // Function the thread runs
        &args,     // Argument passed to the function
        0,                  // Start immediately
        NULL                // Thread ID not needed yet
    );

    if (recieveThreadServer == 0) {
        printf("Failed to create receiving thread\n");
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
        };

        while(1){
            // setting varibles for sending packet
            char sendBuffer[1024];
            Sleep(5);
            printf("Send Message: ");
            fflush(stdin);
            if (fgets(sendBuffer, sizeof(sendBuffer), stdin) != NULL) {
                sendBuffer[strcspn(sendBuffer, "\n")] = '\0';
            }


            // send a 4 letter packet via UDP through assgined socket
            iResult = sendto(ConnectSocket, sendBuffer, (int)sizeof(sendBuffer) - 1, 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
            if(iResult == SOCKET_ERROR){
                printf("Packet Sending Failure %d\n", WSAGetLastError());
                freeaddrinfo(result);
                WSACleanup();
                return 1;
            }
        }   
        //clean up in required order ofc - my comments getting more causal the longer I go lol
        freeaddrinfo(result);
        closesocket(ConnectSocket);
        WSACleanup();
        printf("Process fully complete\n");
    } 
    
    
    
    
    
    
    // Client Side Code ------------------------------------------------------------------------------------------------------------------------------
    
    
    
    
    else {
            printf("Enter the IP address you wish to connect to: ");
            if (fgets(ipAddress, sizeof(ipAddress), stdin) != NULL) {
                ipAddress[strcspn(ipAddress, "\n")] = '\0';
            } 

            // request address info from user using default port assgined above

            iResult = getaddrinfo(ipAddress, DEFAULT_PORT, &hints, &result);
            if (iResult !=0){
                printf("getaddrinfo failed: %d\n", iResult);
                WSACleanup();
                return 1;
            }

            printf("Generating Socket...\n");
            // generate a Socket
            SOCKET ConnectSocket = INVALID_SOCKET;

            ptr = result;

            ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

            // checking socket is valid
            if (ConnectSocket == INVALID_SOCKET) {
            printf("Error at socket(): %ld\n", WSAGetLastError());
            freeaddrinfo(result);
            WSACleanup();
            return 1;
            }


            char sendBuffer[1024] = "Client has joined the chat";

            iResult = sendto(ConnectSocket, sendBuffer, (int)sizeof(sendBuffer) - 1, 0, ptr->ai_addr, (int)ptr->ai_addrlen);
                if(iResult == SOCKET_ERROR){
                    printf("Packet Sending Failure %d\n", WSAGetLastError());
                    freeaddrinfo(result);
                    WSACleanup();
                    return 1;
                }
            printf("sent inital opening msg\n");

            // run a thread that listens for incoming messages.
            uintptr_t recieveThread = _beginthreadex(
            NULL,               // Default security attributes
            0,                  // Default stack size
            recieveMessage,    // Function the thread runs
            &ConnectSocket,     // Argument passed to the function
            0,                  // Start immediately
            NULL                // Thread ID not needed yet
        );

        if (recieveThread == 0) {
            printf("Failed to create receiving thread\n");
            closesocket(ConnectSocket);
            WSACleanup();
            return 1;
            };


            while(1){
                // setting varibles for sending packet
                Sleep(5);
                printf("Send Message: ");
                fgets(sendBuffer, sizeof(sendBuffer), stdin);
                
                iResult = sendto(ConnectSocket, sendBuffer, (int)sizeof(sendBuffer) - 1, 0, ptr->ai_addr, (int)ptr->ai_addrlen);
                if(iResult == SOCKET_ERROR){
                    printf("Packet Sending Failure %d\n", WSAGetLastError());
                    freeaddrinfo(result);
                    WSACleanup();
                    return 1;
                }
            }   
            //clean up in required order ofc - my comments getting more causal the longer I go lol
            freeaddrinfo(result);
            closesocket(ConnectSocket);
            WSACleanup();
            printf("Process fully complete\n");
        }
    

    return 0;
}





unsigned __stdcall recieveMessage(void *arg){
    // -----------------------------------------------------------------------------------------------------------------------------------
    // placed inside an 10 message loop - I will be editing this to create a listening thread that is always watching for new messages
    printf("secondary thread now running");
    SOCKET ConnectSocket = *(SOCKET *)arg;
    int i = 0;
    do{
        char recvBuffer[1024];
        struct sockaddr_in whoFrom;
        int senderAddrSize = sizeof (whoFrom);

    // Recieve a PING packet via UDP through assgined socket
        iResult = recvfrom(ConnectSocket, recvBuffer, (int)sizeof(recvBuffer) - 1, 0, (SOCKADDR *)&whoFrom, &senderAddrSize);
        if (iResult == SOCKET_ERROR) {
            printf("recvfrom failed: %ld\n", WSAGetLastError());
            closesocket(ConnectSocket);
            WSACleanup();
            return 1;
        }
        printf("Received: %s\n", recvBuffer);
        i++;
    } while (i < 10);
    
    // -----------------------------------------------------------------------------------------------------------------------------------
    closesocket(ConnectSocket);
    WSACleanup();

    printf("Process fully complete\n");
    return 0;
}









unsigned __stdcall recieveMessageServer(void *arg){
    // -----------------------------------------------------------------------------------------------------------------------------------
    // placed inside an 10 message loop - I will be editing this to create a listening thread that is always watching for new messages
    printf("secondary thread now running\n");

    ReceiveThreadArgs *args = (ReceiveThreadArgs *)arg;
    SOCKET socket = *args->ConnectSocket;
    struct sockaddr_in *clientAddr = args->clientAddr;
    int clientAddrLen = sizeof(*clientAddr);

    int i = 0;
    do{
        char recvBuffer[1024];


    // Recieve a PING packet via UDP through assgined socket
        iResult = recvfrom(socket, recvBuffer, (int)sizeof(recvBuffer) - 1, 0, (struct sockaddr *)clientAddr, &clientAddrLen);
        if (iResult == SOCKET_ERROR) {
            printf("recvfrom failed: %ld\n", WSAGetLastError());
            closesocket(socket);
            WSACleanup();
            return 1;
        }
        printf("Received: %s\n", recvBuffer);
        i++;
    } while (i < 10);
    
    // -----------------------------------------------------------------------------------------------------------------------------------
    closesocket(socket);
    WSACleanup();

    printf("Process fully complete\n");
    return 0;
}





