/*
 * 
 */

#ifndef C_LOGGER_H
#define C_LOGGER_H

#ifdef __cplusplus
extern "C" {
#endif
    
/*
 * log formatter defines
 */
#define C_LOGGER_TIME 
#undef C_LOGGER_THREAD_NAME
#define C_LOGGER_THREAD_ID
#define C_LOGGER_LEVEL_TEXT 
#define C_LOGGER_FILE 
#undef C_LOGGER_FUNCTION
#define C_LOGGER_LINE 
    
/*
 * log level enums
 */
typedef enum {
    C_LOGGER_LEVEL_MIN,
    C_LOGGER_DEBUG,
    C_LOGGER_TRACE,
    C_LOGGER_INFO,
    C_LOGGER_WARNING,
    C_LOGGER_ERROR,
    C_LOGGER_LEVEL_MAX
}
c_logger_level_enum_t;

extern int c_logger_level;

/*
 * init main logger with given log_level and log_file_path
 * this function is optional
 */
int c_logger_init(int level, const char* log_file_path);

/*
 * log_level setter
 */
int set_logger_level(int level);

/*
 * logging function, don't use
 * use logging macros
 */
void c_logger(c_logger_level_enum_t level, 
#ifdef C_LOGGER_TIME
              struct timespec* time, 
#endif
#ifdef C_LOGGER_FILE
              const char* file, 
#endif
#ifdef C_LOGGER_FUNCTION
              const char* function, 
#endif
#ifdef C_LOGGER_LINE
              int line, 
#endif
              const char* format, ...);

#ifdef C_LOGGER_TIME
    #define LOGGER_TIME_FUNC struct timespec time;\
        clock_gettime(CLOCK_REALTIME, &time);
    #define LOGGER_TIME_ARG &time, 
#else
    #define LOGGER_TIME_FUNC
    #define LOGGER_TIME_ARG 
#endif

#ifdef C_LOGGER_FILE
    #define LOGGER_FILE_ARG __FILE__,
#else
    #define LOGGER_FILE_ARG
#endif

#ifdef C_LOGGER_FUNCTION
    #define LOGGER_FUNC_ARG __FUNCTION__,
#else
    #define LOGGER_FUNC_ARG
#endif

#ifdef C_LOGGER_LINE
    #define LOGGER_LINE_ARG __LINE__,
#else
    #define LOGGER_LINE_ARG
#endif
                      
#define logger(level,format,args...) {\
    if(level>=c_logger_level){\
        LOGGER_TIME_FUNC\
        c_logger(level, LOGGER_TIME_ARG LOGGER_FILE_ARG LOGGER_FUNC_ARG LOGGER_LINE_ARG format,##args);\
    }\
} 

/*
 * logging macros
 */
#define trace(format,args...) logger(C_LOGGER_TRACE,format,##args)
#define debug(format,args...) logger(C_LOGGER_DEBUG,format,##args)
#define info(format,args...) logger(C_LOGGER_INFO,format,##args)
#define warning(format,args...) logger(C_LOGGER_WARNING,format,##args)
#define error(format,args...) logger(C_LOGGER_ERROR,format,##args)

#ifdef __cplusplus
}
#endif

#endif /* C_LOGGER_H */
