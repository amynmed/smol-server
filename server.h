#ifndef SMOLSERVER
#define SMOLSERVER

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



#define DEFAULT_PORT    8000

#define ROOT_PATH       "root"
#define INDEX           "index.html"

#define MAX_THREADS     256

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

#endif