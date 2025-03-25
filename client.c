

#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib") // winsock lib



int main(int argc, char **argv)
{
        WSADATA wsa;
        SOCKET  sock;

        struct sockaddr_in server;

        // host resolution
        char *hostname = "www.google.com";
	char ip[100];
	struct hostent *he;
	struct in_addr **addr_list;


        printf("=========================\n");
        printf("Socket initialization ...\n");

        if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
        {
                printf("Initialization failed. Code: %d", WSAGetLastError());
                return 1;
        }

        printf("Socket initialised.\n");

        //

        // AF_NET      --> IPV4
        // SOCK_STREAM --> TCP
        //
        // creating socket
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (sock == INVALID_SOCKET) 
        {
                printf("Socket creation failed with error: %d\n", WSAGetLastError());
                WSACleanup();
                return 1;
        }

        printf("Socket created.\n");

        // =============
        // =============

        //
        if ( (he = gethostbyname( hostname ) ) == NULL) 
	{
		//gethostbyname failed
		printf("gethostbyname failed : %d" , WSAGetLastError());
		return 1;
	}
	
	// Cast the h_addr_list to in_addr , since h_addr_list also has the ip address in long format only
	addr_list = (struct in_addr **) he->h_addr_list;
	
	for(int i = 0; addr_list[i] != NULL; i++) 
	{
		//Return the first one;
		strcpy(ip , inet_ntoa(*addr_list[i]) ); // inet_ntoa --> IP long to IP dotted
	}
	
        // hostname --> IP
	printf("%s resolved to : %s\n" , hostname , ip);

        // =============
        // =============
        // using localhost instead
        //
        server.sin_family = AF_INET;
        server.sin_port   = htons(8080);
        //server.sin_port   = htons(80);

        //if (inet_pton(AF_INET, ip, &(server.sin_addr)) <= 0) 
        if (inet_pton(AF_INET, "127.0.0.1", &(server.sin_addr)) <= 0) 
        {
                printf("Address Invalid.\n");
                closesocket(sock);
                WSACleanup();
                return 1;
        }

        printf("Server address : %s\n", inet_ntoa(server.sin_addr));


        printf("IP address is valid.\n");


        // connect to server
        if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) 
        {
                printf("Connection failed with error code: %d\n", WSAGetLastError());
                closesocket(sock);
                WSACleanup();
                return 1;
        }

        puts("Connected to socket");

        // sending data

        char *message = "GET / HTTP/1.1\r\n\r\n";

        if( send(sock, message, strlen(message), 0) < 0)
        {
                printf("cannot send data: %d \n", WSAGetLastError());
                return 1;
        }

        puts("Data sent.\n");

        char server_response[2048];

        int recv_size = recv(sock , server_response , sizeof(server_response) , 0);

        //Receive a reply from the server
	if(recv_size == SOCKET_ERROR)
	{
		puts("recv failed");
	}
	
	puts("Reply received\n");

	//Add a NULL terminating character to make it a proper string before printing
	server_response[recv_size] = '\0';
	printf("Server response : %s\n", server_response);


        // clean
        closesocket(sock);
        WSACleanup();

        return 0;
}