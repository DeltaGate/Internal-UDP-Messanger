#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <windows.h>
#include <process.h>


#pragma comment(lib, "Ws2_32.lib")


    WSADATA wsaData;
    int iResult;
    unsigned __stdcall recieveMessage(void *arg);
    // create 3 addrinfo strcuts - or 2 pointers and 1 full, these hold adress infomation for connection
    struct addrinfo *result = NULL, *ptr = NULL, hints;

int main(int argc, char *argv[]){

    if (argc < 2)
    {
        printf("No socket address provided, shutting down");
        return 1;
    }
    

    // initalise Ws2 DLL and winsock2 lib, check for errors

    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult !=0) {
    printf("WSAStartup failed: %d\n", iResult);
    return 1;
} else {
    printf("WSAStartup Successful\n");
}



    ZeroMemory(&hints, sizeof(hints)); // ZeroMemory is a macro that fills a block of memory with zeros, so each varible in the structure is set to 0, this is important as it prevents garbage values from being used in the struct.
    hints.ai_family     = AF_UNSPEC; // defines if connection accepts IPv4 or IPv6 - I chose unspec so it can be either
    hints.ai_socktype   = SOCK_DGRAM; // Telling it to use UDP socket
    hints.ai_protocol   = IPPROTO_UDP; // Telling it to use UDP protocol

    printf("Adress Objects Created\n");

    #define DEFAULT_PORT "27015"

    // request address info from user using default port assgined above

    iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
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



    // Allow address reuse - temp code whilst testing on a single device, will remove once switched to server-client model.
    BOOL reuse = TRUE;

    if (setsockopt(
            ConnectSocket,
            SOL_SOCKET,
            SO_REUSEADDR,
            (const char *)&reuse,
            sizeof(reuse)) == SOCKET_ERROR) {
        printf("setsockopt failed: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
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
        // setting varibles for sending ping packet
        char sendBuffer[4];
        scanf("%s", sendBuffer);
        int bufferLength = 4;


        // send a 4 letter packet via UDP through assgined socket
        iResult = sendto(ConnectSocket, sendBuffer, bufferLength, 0, ptr->ai_addr, (int)ptr->ai_addrlen);
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
    return 0;
}





unsigned __stdcall recieveMessage(void *arg){
    // -----------------------------------------------------------------------------------------------------------------------------------
    // placed inside an 10 message loop - I will be editing this to create a listening thread that is always watching for new messages
    SOCKET ConnectSocket = *(SOCKET *)arg;
    int i = 0;
    do{
        char recvBuffer[5];
        int bufferLength = 4;
        struct sockaddr_in whoFrom;
        int senderAddrSize = sizeof (whoFrom);

    // Recieve a PING packet via UDP through assgined socket
        iResult = recvfrom(ConnectSocket, recvBuffer, bufferLength, 0, (SOCKADDR *)&whoFrom, &senderAddrSize);
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

