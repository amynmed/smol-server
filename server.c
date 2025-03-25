

#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>


#define ROOT_PATH       "root"
#define INDEX           "index.html"


#pragma comment(lib, "ws2_32.lib") // winsock lib

// Threading
// ===================================
typedef struct 
{
        SOCKET socket;
        char *buf;
        int len;
        int flags;
        
} send_params;


DWORD WINAPI th_send(LPVOID lpParam) 
{
        send_params* params = (send_params*)lpParam;
        send(params->socket, params->buf, params->len, params->flags);
        return 0;
}
// ===================================


int main(int argc, char **argv)
{

        WSADATA wsa;
        SOCKET  sock, new_sock;

        struct sockaddr_in server, client;

        printf("init winsock...\n");

        if(WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        {
                printf("WSA init failed. %d \n", WSAGetLastError());
                return 1;
        }

        printf("WSA initialized.\n");

        // create socket
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if(sock == INVALID_SOCKET)
        {
                printf("cannot create socket : %d\n", WSAGetLastError());
                WSACleanup();
                return 1;
        }

        printf("Socket created.\n");
        //

        // Prepare the sockaddr_in structure
        //
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_family      = AF_INET;
	server.sin_port        = htons( 8888 );

        // Binding to socket
        // --> By binding we ensure that incoming data is recieved by this process.
        int bind_state = bind(sock, (struct sockaddr*)&server, sizeof(server));

        if(bind_state == SOCKET_ERROR)
        {
                printf("Binding failed : %d\n", WSAGetLastError());
                WSACleanup();
                return 1;
        }

        printf("Bound\n");

        // Putting socket to listening mode
        listen(sock, SOMAXCONN);

        printf("awaiting incoming connections ...\n");

        int c = sizeof(struct sockaddr_in);

        //new_sock = accept(sock, (struct sockaddr *)&client, &c);

        char *client_ip;
        int client_port;

        // Getting index content
        // =======================================
        
        char index_file_path[256] = ROOT_PATH;
        strcat(index_file_path, "\\index.html");

        printf("current index path : %s\n", index_file_path);

        //
        FILE *index_file = fopen(index_file_path, "r");

        if(index_file == NULL)
        {
                printf("ERROR CANNOT READ INDEX FILE\n");
                return 1;
        }

        
        char index_content_buffer[4096];
        size_t bytes_read                = fread(index_content_buffer, 1, sizeof(index_content_buffer) - 1, index_file);
        index_content_buffer[bytes_read] = '\0'; 

        printf("index file buffer ==============:\n %s", index_content_buffer);

        fclose(index_file);
        //

        char *HTTP_HEADER = "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/html; charset=UTF-8\r\n"
                            "\r\n";

        char message[8192];
        strcpy(message, HTTP_HEADER);
        strcat(message, index_content_buffer);

        printf("message content ================ :\n %s", message);

        //======================================

        // Threading
        //======================================
        HANDLE thread;
        DWORD threadId;

        // cannot handle more than one connection at a time.
        //
        while(new_sock != INVALID_SOCKET)
        {
                new_sock = accept(sock , (struct sockaddr *)&client, &c);

                if(new_sock == INVALID_SOCKET)
                {
                        printf("accept failed : ERROR::%d\n", WSAGetLastError());
                        WSACleanup();
                        return 1;
                }

                printf("connection accepted\n");

                client_ip   = inet_ntoa(client.sin_addr);
                client_port = ntohs(client.sin_port);

                printf("client ip : %s\nclient_port : %d\n", client_ip, client_port);

                
                //Reply to client
                send(new_sock , message , strlen(message) , 0);
                
                //Reply to client (with threads)

                //thread = CreateThread(NULL, 0, th_send, (void *){new_sock, message, strlen(message), 0}, 0, &threadId);


                printf("a message was sent to the client\n");

        }

        


        //WaitForSingleObject(thread, INFINITE);
        //CloseHandle(thread);
        
        closesocket(sock);
        WSACleanup();


        return 0;



}