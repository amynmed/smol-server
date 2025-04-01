

#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>


#define DEFAULT_PORT    8000

#define ROOT_PATH       "root"
#define INDEX           "index.html"

#define MAX_THREADS     128







#pragma comment(lib, "ws2_32.lib") // winsock lib



void * get_file_content(char *file_path, char *destination_buffer);
DWORD WINAPI th_send(LPVOID lpParam) ;

// Threading
// ===================================
typedef struct 
{
        SOCKET socket;
        char   buf[4096];
        int    len;
        int    flags;
        //
        char  *client_ip;
        int    client_port;
        
} send_params;


DWORD WINAPI th_send(LPVOID lpParam) 
{
        send_params* params = (send_params*)lpParam;

        printf("Sending Response to client ...\n");
        printf("client ip : %s\nclient_port : %d\n", params->client_ip, params->client_port);

        // ** FOR DEBUG
        Sleep(10000);

        char received_buffer[4096];

        int n_bytes = recv(params->socket, received_buffer, sizeof(received_buffer) - 1, 0);

        if(n_bytes)
        {
                received_buffer[n_bytes] = '\0';
                printf("Request received from client:\n %s\n", received_buffer);
        }


        // Default send
        send(params->socket, params->buf, params->len, params->flags);
        
        printf("Response was sent ...\n");
        

        free(params);

        return 0;
}
// ===================================

void * get_file_content(char *file_path, char *destination_buffer)
{

        char full_path[256] = ROOT_PATH;
        strcat(full_path, file_path);

        printf("file path : %s\n", full_path);

        //
        FILE *index_file = fopen(full_path, "r");

        if(index_file == NULL)
        {
                printf("ERROR CANNOT READ INDEX FILE\n");
                return 1;
        }

        
        size_t bytes_read              = fread(destination_buffer, 1, sizeof(destination_buffer) - 1, index_file);
        destination_buffer[bytes_read] = '\0'; 


        fclose(index_file);

}

// **TODO_1 : parse the path chosen by the client and fetch the requested file
// **TODO_2 : use threadpools instead
//

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
	server.sin_port        = htons( DEFAULT_PORT );

        // Binding to socket
        // --> By binding we ensure that incoming data is recieved by this process.
        int bind_state = bind(sock, (struct sockaddr*)&server, sizeof(server));

        if(bind_state == SOCKET_ERROR)
        {
                printf("Binding failed : %d\n", WSAGetLastError());
                WSACleanup();
                return 1;
        }

        printf("Socket Bound\n");

        
        


        

        //new_sock = accept(sock, (struct sockaddr *)&client, &c);

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

        // printf("index file buffer ==============:\n %s", index_content_buffer);

        fclose(index_file);
        //

        // Default HTTP header
        char *HTTP_HEADER = "HTTP/1.1 200 OK\r\n"
                            "Cache-Control: no-cache\r\n"
                            "Content-Type: text/html; charset=UTF-8\r\n"
                            "\r\n";

        char message[8192];
        strcpy(message, HTTP_HEADER);
        strcat(message, index_content_buffer);

        // printf("message content ================ :\n %s", message);

        //======================================

        // Threading
        // ======================================
        HANDLE thread  [MAX_THREADS];
        DWORD  threadId[MAX_THREADS];

        int current_thread_id = 0;

        


        char *client_ip;
        int   client_port;

        // Putting socket to listening mode
        // ================================
        listen(sock, SOMAXCONN);

        int c = sizeof(struct sockaddr_in);
        // cannot handle more than one connection at a time.
        //// Getting index content
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

        // printf("index file buffer ==============:\n %s", index_content_buffer);

        fclose(index_file);
        //

        // Default HTTP header
        char *HTTP_ERROR_HEADER = "HTTP/1.1 400 Bad Request\r\n"
                                  "Cache-Control: no-cache\r\n"
                                  "Content-Type: text/html; charset=UTF-8\r\n"
                                  "\r\n";
        
        // Default HTTP header
        char *HTTP_HEADER = "HTTP/1.1 200 OK\r\n"
                            "Cache-Control: no-cache\r\n"
                            "Content-Type: text/html; charset=UTF-8\r\n"
                            "\r\n";

        char message[8192];
        strcpy(message, HTTP_HEADER);
        strcat(message, index_content_buffer);

        while(new_sock != INVALID_SOCKET)
        {
                printf("awaiting incoming connections ...\n");

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

                printf("got client info\n");
                
                
                
                printf("setting send params\n");
                send_params *sp = (send_params*)malloc(sizeof(send_params));
                sp->client_ip   = client_ip;
                sp->client_port = client_port;
                sp->socket      = new_sock;
                sp->len         = strlen(message);
                sp->flags       = 0;
                strcpy(sp->buf, message);

                printf("params set\n");
                
                printf("Launching thread : %d\n", current_thread_id);
                printf("=====================\n");
                thread[current_thread_id] = CreateThread(NULL, 0, th_send, sp, 0, &threadId[current_thread_id]);

                if (thread[current_thread_id] == NULL) 
                {
                        printf("Thread creation failed. Thread_id :  %d\n", current_thread_id + 1);
                        return 1;
                }

                current_thread_id++;


                // printf("a response was sent to the client:\n");
                // printf("client ip : %s\nclient_port : %d\n", client_ip, client_port);

        }


        printf("Joining threads\n");
        WaitForMultipleObjects(MAX_THREADS, thread, TRUE, INFINITE);

        for (int i = 0; i < MAX_THREADS; i++) CloseHandle(thread[i]);

        
        closesocket(sock);
        WSACleanup();


        return 0;



}