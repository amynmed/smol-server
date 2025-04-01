
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <time.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include "server.h"

#pragma comment(lib, "ws2_32.lib") // winsock lib


const char *HTTP_HEADER_FORMAT = "HTTP/1.1 %s\r\n"
                                 "Cache-Control: no-cache\r\n"
                                 "Content-Type: %s\r\n"
                                 "\r\n";

// ======
// ======

// **TODO : use threadpools instead | ~
//
int main(int argc, char **argv)
{

        WSADATA wsa;
        SOCKET  sock, new_sock;

        struct sockaddr_in server, client;

        LOG_INFO("init winsock...\n");
        LOG_INFO("init winsock...\n");

        if(WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        {
                LOG_ERROR("WSA init failed. %d \n", WSAGetLastError());
                LOG_ERROR("WSA init failed. %d \n", WSAGetLastError());
                return 1;
        }

        LOG_INFO("WSA initialized.\n");
        LOG_INFO("WSA initialized.\n");

        // create socket
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if(sock == INVALID_SOCKET)
        {
                LOG_ERROR("cannot create socket : %d\n", WSAGetLastError());
                LOG_ERROR("cannot create socket : %d\n", WSAGetLastError());
                WSACleanup();
                return 1;
        }

        LOG_INFO("Socket created.\n");

        //
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_family      = AF_INET;
	server.sin_port        = htons( DEFAULT_PORT );
	server.sin_port        = htons( DEFAULT_PORT );

        // Binding to socket
        // --> By binding we ensure that incoming data is recieved by this process.
        int bind_state = bind(sock, (struct sockaddr*)&server, sizeof(server));

        if(bind_state == SOCKET_ERROR)
        {
                LOG_ERROR("Binding failed : %d\n", WSAGetLastError());
                LOG_ERROR("Binding failed : %d\n", WSAGetLastError());
                WSACleanup();
                return 1;
        }

        LOG_INFO("Socket Bound\n");


        // Threading
        // ======================================
        HANDLE thread  [MAX_THREADS];
        DWORD  threadId[MAX_THREADS];
        
        int current_thread_id = 0;
        
        // ======================================

        char *client_ip;
        int   client_port;

        // Putting socket to listening mode
        // ================================
        listen(sock, SOMAXCONN);

        int c = sizeof(struct sockaddr_in);

        // cannot handle more than one connection at a time.
        // while(new_sock != INVALID_SOCKET)
        while(true)
        {
                LOG_INFO("awaiting incoming connections ...\n");

                LOG_INFO("awaiting incoming connections ...\n");

                new_sock = accept(sock , (struct sockaddr *)&client, &c);

                if(new_sock == INVALID_SOCKET)
                {
                        int error = WSAGetLastError();
                        LOG_ERROR("accept failed : ERROR::%d\n", error);
                        if(error == WSAENETDOWN || error == WSAEINTR) {
                                LOG_ERROR("Critical error, shutting down...\n");
                                break;
                        }
                        
                        Sleep(100);
                        continue;
                }

                if (current_thread_id >= MAX_THREADS) 
                {
                        LOG_ERROR("Thread limit exeeded.\n");
                        closesocket(new_sock);
                        continue;
                }


                LOG_INFO("connection accepted\n");

                client_ip   = inet_ntoa(client.sin_addr);
                client_port = ntohs(client.sin_port);

                LOG_INFO("got client info\n");
                
                LOG_INFO("setting send params\n");

                send_params *sp = (send_params*)malloc(sizeof(send_params));
                sp->client_ip   = client_ip;
                sp->client_port = client_port;
                sp->socket      = new_sock;
                sp->len         = 0;
                sp->flags       = 0;

                LOG_INFO("params set\n");
                
                LOG_INFO("Launching thread : %d\n", current_thread_id);
                LOG_INFO("========================================\n");

                thread[current_thread_id] = CreateThread(NULL, 0, th_serve, sp, 0, &threadId[current_thread_id]);

                if (thread[current_thread_id] == NULL) 
                {
                        LOG_ERROR("Thread creation failed. Thread_id :  %d\n", current_thread_id + 1);
                        return 1;
                }

                current_thread_id++;

        }

        LOG_INFO("Joining threads\n");
        WaitForMultipleObjects(MAX_THREADS, thread, TRUE, INFINITE);

        for (int i = 0; i < MAX_THREADS; i++)
                CloseHandle(thread[i]);

        closesocket(sock);
        WSACleanup();


        return 0;

} 
// Main


        
void get_requested_path(char *request_buffer, char *dest_path)
{
        char *file_path, *cp;
        const char delimiters[] = " ,;:!-";

        // this assumes that the path is the second token in a HTTP request
        // I don't know if this is valid for all cases
        cp        = strdup((const char*)request_buffer);
        file_path = strtok(cp  , delimiters);
        file_path = strtok(NULL, delimiters);

        LOG_DEBUG("GOT PATH : %s", file_path);

        if(file_path)
                strcpy(dest_path, file_path);

}

void get_file_extension(char *file_path, char *dest_extension)
{
        char *prev, *curr, *cp;
        const char delimiters[] = "/.";

        LOG_DEBUG("GETTING EXTENSION");
        
        // avoid this cast
        cp   = strdup((const char*)file_path);
        curr = strtok(cp, delimiters);
        

        while(prev = strtok(NULL, delimiters))
        {
                LOG_DEBUG("PARSING ...");
                curr = prev;
                LOG_DEBUG("CURRENT TOKEN : %s", curr);
        }

        LOG_DEBUG("PARSED");
        
        if(curr != NULL)
        {
                strcpy(dest_extension, curr);
                LOG_DEBUG("GOT EXTENSION");
        }

}


DWORD WINAPI th_serve(LPVOID lpParam) 
{
        send_params *params = (send_params*)lpParam;

        LOG_INFO("Sending Response to client ...");
        LOG_INFO("client ip : %s, client_port : %d",
                 params->client_ip,
                 params->client_port);

        // ** FOR DEBUG
        // Sleep(3000);

        char received_buffer     [4096];
        char requested_file_path [256];
        char file_extension      [128];

        char file_content        [4096];
        char response_buffer     [256];

        int n_bytes;
        
        // this receives once 
        // this will fail if the client sends data that is bigger than the buffer

        n_bytes = recv(params->socket, received_buffer, sizeof(received_buffer) - 1, 0);

        if(n_bytes > 0)
        {

                received_buffer[n_bytes] = '\0';
                LOG_INFO("Request received from client:=========================\n %s\n", received_buffer);
                LOG_INFO("======================================================");

                get_requested_path(received_buffer, requested_file_path);

                get_file_extension(requested_file_path, file_extension);

                LOG_DEBUG("PATH REQUESTED BY CLIENT: %s\n", requested_file_path);
                LOG_DEBUG("PATH EXTENSTION : %s\n"        , file_extension);
                // getting file content
                LOG_DEBUG("Requested PATH: %s", requested_file_path);

                FILE *file = NULL;

                if(get_file(requested_file_path, &file))
                {
                        LOG_ERROR("error reading file");

                        snprintf
                        (
                                response_buffer,
                                sizeof response_buffer,
                                HTTP_HEADER_FORMAT,
                                "404 Not Found",
                                "text/html;"
                        );

                        strcat(response_buffer, "<html>\n <body> <h1> 404 Not Found </h1> </body></html>");

                        send(params->socket, response_buffer, strlen(response_buffer), 0);

                        closesocket(params->socket);
                        free(params);

                        return 1;
                }
                        
                else
                        LOG_INFO("GOT FILE");


                // RESPONSE
                //
                if (strcmp(file_extension, "js") == 0)
                {
                        snprintf
                        (
                                response_buffer,
                                sizeof response_buffer,
                                HTTP_HEADER_FORMAT,
                                "200 OK",
                                "application/javascript"
                        );
                }

                else if (strcmp(file_extension, "ico") == 0)
                {
                        snprintf
                        (
                                response_buffer,
                                sizeof response_buffer,
                                HTTP_HEADER_FORMAT,
                                "200 OK",
                                "image/x-icon"
                        );
                }

                else
                {
                        snprintf
                        (
                                response_buffer,
                                sizeof response_buffer,
                                HTTP_HEADER_FORMAT,
                                "200 OK",
                                "text/html; charset=UTF-8"
                        );
                }
                
                send(params->socket, response_buffer, strlen(response_buffer), params->flags);

                LOG_DEBUG("HEADER SENT :\n %s", response_buffer);
                
                LOG_DEBUG("SENDING CONTENT ...");

                if(send_file(params->socket, file))
                        LOG_ERROR("ERROR SENDING FILE");
                else
                        LOG_DEBUG("CONTENT SENT ...");
                
                LOG_INFO("Response was sent ...\n");
        }

        else if (n_bytes == 0)
                LOG_INFO("Connection closing...\n");
        else  
        {
                LOG_ERROR("recv failed with error: %d\n", WSAGetLastError());
                closesocket(params->socket);
                free(params);
                return 1;
        }

        closesocket(params->socket);
        free(params);

        return 0;
}

// ===================================

int get_file(char *file_path, FILE **file)
{
        char full_path[256] = ROOT_PATH;

        if(file_path == NULL)
                strcat(full_path, "\\index.html");
        else if(strcmp(file_path, "/") != 0)
                strcat(full_path, file_path);
        else
                strcat(full_path, "\\index.html");
        

        LOG_DEBUG("file path : %s\n", full_path);
        
        *file = fopen(full_path, "rb");

        LOG_DEBUG("file opened : %s\n", full_path);

        if(*file == NULL)
        {
                LOG_ERROR("Cannot read file.\n");
                return 1;
        }

        return 0;

}

int send_file(SOCKET socket, FILE *file)
{
        char buffer[4096];

        size_t bytes_read;

        LOG_INFO("SENDING FILE ...");
        LOG_INFO("FILE SIZE : %d", get_file_size(file));

        if(file)
        {
                LOG_DEBUG("FILE NOT NULL");

                // Read file in chunks and send
                while ((bytes_read = fread(buffer, 1, sizeof buffer, file)) > 0) 
                {

                        LOG_DEBUG("Read %zu bytes from file", bytes_read);
                        LOG_DEBUG("Previewing 64 Bytes from buffer:\n");
                        for (size_t i = 0; i < bytes_read && i < 64; i++) 
                        {
                                printf("%02X ", (unsigned char)buffer[i]);
                                // square shaped buffer
                                if(!((i+1) % 8)) printf("\n");
                        }
                        printf("\n");


                        if (send(socket, buffer, bytes_read, 0) != bytes_read) 
                        {
                                perror("send failed");
                                fclose(file);
                                closesocket(socket);
                                return 1;
                        }
                }
        }
        else
        {
                LOG_ERROR("ERROR READING FILE");
                return 1;
        }
        
        fclose(file);
        return 0;
}

long get_file_size(FILE *file) 
{
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        rewind(file);          
        return size;
}

// ======================================================================
