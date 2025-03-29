
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <time.h>
#include <string.h>
#include <stddef.h>

#pragma comment(lib, "ws2_32.lib") // winsock lib

// =====================
// =====================

#define ENABLE_LOGGING 1

#define RESET   "\x1b[0m"
#define RED     "\x1b[31m" 
#define GREEN   "\x1b[32m"  
#define YELLOW  "\x1b[33m"  
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define WHITE   "\x1b[37m"

#define LOG(level, color, format, ...) \
    do \
    { \
        if (ENABLE_LOGGING) \
        { \
            time_t now   = time(NULL); \
            struct tm *t = localtime(&now); \
            char time_buf[20]; \
            strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", t); \
            fprintf(stderr, "%s[%s] %s:%d:%s() [%s] " format "%s\n", \
                    color, time_buf, __FILE__, __LINE__, __func__, level, ##__VA_ARGS__, RESET); \
        } \
    } while (0)

#define LOG_INFO(format, ...)  LOG("INFO",  GREEN,  format, ##__VA_ARGS__)
#define LOG_WARN(format, ...)  LOG("WARN",  YELLOW, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) LOG("ERROR", RED,    format, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...) LOG("DEBUG", BLUE,   format, ##__VA_ARGS__)

// =====================
// =====================

#define DEFAULT_PORT    8000

#define ROOT_PATH       "root"
#define INDEX           "index.html"

#define MAX_THREADS     128




// =====================
// =====================

const char *HTTP_HEADER_FORMAT;

void  get_requested_path(char *request_buffer, char *dest_path);
long  get_file_size(FILE *file);
int   send_file(SOCKET socket , FILE *file);
int   get_file(char *file_path, FILE **file);
DWORD WINAPI th_serve(LPVOID lpParam) ;

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


// ======
// ======

const char *HTTP_HEADER_FORMAT = "HTTP/1.1 %s\r\n"
                                 "Cache-Control: no-cache\r\n"
                                 "Content-Type: %s\r\n"
                                 "\r\n\0";

// ======
// ======

// **TODO_1 : parse the path chosen by the client and fetch the requested file | X
// **TODO_2 : use threadpools instead | ~
//

int main(int argc, char **argv)
{

        WSADATA wsa;
        SOCKET  sock, new_sock;

        struct sockaddr_in server, client;

        LOG_INFO("init winsock...\n");

        if(WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        {
                LOG_ERROR("WSA init failed. %d \n", WSAGetLastError());
                return 1;
        }

        LOG_INFO("WSA initialized.\n");

        // create socket
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if(sock == INVALID_SOCKET)
        {
                LOG_ERROR("cannot create socket : %d\n", WSAGetLastError());
                WSACleanup();
                return 1;
        }

        LOG_INFO("Socket created.\n");
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
                LOG_ERROR("Binding failed : %d\n", WSAGetLastError());
                WSACleanup();
                return 1;
        }

        LOG_INFO("Socket Bound\n");

        
        

        //======================================

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
        while(new_sock != INVALID_SOCKET)
        {
                LOG_INFO("awaiting incoming connections ...\n");

                new_sock = accept(sock , (struct sockaddr *)&client, &c);

                if(new_sock == INVALID_SOCKET)
                {
                        LOG_ERROR("accept failed : ERROR::%d\n", WSAGetLastError());
                        WSACleanup();
                        return 1;
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


                // printf("a response was sent to the client:\n");
                // printf("client ip : %s\nclient_port : %d\n", client_ip, client_port);

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
        send_params* params = (send_params*)lpParam;

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
                        LOG_ERROR("ERROR READING FILE CONTENT");
                else
                        LOG_INFO("GOT FILE");
                
                /*
                char test_buffer[4096];
                fread(test_buffer, 1, sizeof test_buffer, file);
                LOG_DEBUG("TEST_FILE_BUFFER :\n %s", test_buffer);
                */



                // RESPONSE
                //
                if (strcmp(file_extension, "js") == 0)
                {
                        snprintf(
                                response_buffer,
                                sizeof response_buffer,
                                HTTP_HEADER_FORMAT,
                                "200 OK",
                                "application/javascript"
                        );
                }

                else if (strcmp(file_extension, "ico") == 0)

                {
                        snprintf(
                                response_buffer,
                                sizeof response_buffer,
                                HTTP_HEADER_FORMAT,
                                "200 OK",
                                "image/x-icon"
                        );
                }

                else
                {
                        snprintf(
                                response_buffer,
                                sizeof response_buffer,
                                HTTP_HEADER_FORMAT,
                                "200 OK",
                                "text/html; charset=UTF-8"
                        );
                }

                // TEST RESPONSE BUFFER
                //LOG_("TEST_REPONSE_HEADER_BUFFER_____________________ :\n %s", response_buffer);

                
                // add content
                //strcat(response_buffer, file_content);
                
                //
                
                send(params->socket, response_buffer, strlen(response_buffer), params->flags);

                LOG_DEBUG("HEADER SENT :\n %s", response_buffer);
                
                LOG_DEBUG("SENDING CONTENT ...");

                if(send_file(params->socket, file))
                        LOG_ERROR("ERROR SENDING FILE");
                else
                        LOG_DEBUG("CONTENT SENT ...");

                // Default send
                // send(params->socket, params->buf, params->len, params->flags);
                
                LOG_INFO("Response was sent ...\n");
        }

        else if (n_bytes == 0)
                LOG_INFO("Connection closing...\n");
        else  
        {
                LOG_ERROR("recv failed with error: %d\n", WSAGetLastError());
                closesocket(params->socket);
                free(params);
                WSACleanup();
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
        
        // in binary mode
        *file = fopen(full_path, "rb");

        LOG_DEBUG("file opened : %s\n", full_path);

        if(*file == NULL)
        {
                LOG_ERROR("ERROR CANNOT READ INDEX FILE\n");
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
                        // LOG_DEBUG("BYTES READ : %zu", bytes_read);
                        // LOG_DEBUG("BUFFER READ :\n %s", buffer);

                        LOG_DEBUG("Read %zu bytes from file", bytes_read);
                        // Print first 50 bytes for debugging (hex format)
                        LOG_DEBUG("Previewing 64 Bytes from buffer:\n");
                        for (size_t i = 0; i < bytes_read && i < 64; i++) 
                        {
                                printf("%02X ", (unsigned char)buffer[i]);
                                if(!((i+1) % 8)) printf("\n");
                        }
                        printf("\n");


                        if (send(socket, buffer, bytes_read, 0) != bytes_read) 
                        {
                                perror("send failed");
                                fclose(file);
                                closesocket(socket);
                                exit(EXIT_FAILURE);
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
