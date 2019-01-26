//*********************************************************************************************//
// A LOT OF CREDIT IS DUE TO:
//https://github.com/linfan/TCP-IP-Network-basic-examples/blob/master/chapter9/EpollServer.c
//
//********************************************************************************************//
#pragma once

#include <sys/socket.h>
#include <netdb.h>
// epoll interface
#include <sys/epoll.h>
// struct sockaddr_in
#include <netinet/in.h>
// IP addr convertion
#include <arpa/inet.h>
// File descriptor controller
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
// bzero()
#include <string.h>
// malloc(), free()
#include <stdlib.h>
#include <errno.h>
#include <string>
// #include <sys/resource.h>

#include "ComLog.h"
#include "Cuserdb.h"

// maximum received data byte
#define MAXBYTE     100
#define OPEN_MAX    100

#define SIZE_OF_LOGIN_MESSAGE  25+1
#define SIZE_OF_USERNAME       11+1
#define SIZE_OF_PASSWORD      11+1

//#define LISTENQ     20  >> Changed to MaxEvents

#define SERV_PORT   10012
#define INFTIM      1000
#define LOCAL_ADDR  "127.0.0.1"

#define   SERVER_PORT_SIZE 15
// #define TIMEOUT     500

using namespace std;

enum tstate {
    TS_INACTIVE,
    TS_STARTING,
    TS_STARTED,
    TS_ALIVE,
    TS_WAITING,
    TS_RUNNING,
    TS_TERMINATED,
    TS_STOPPING,
    TS_JOINED
};

typedef struct thread_info{   /* Used as argument to thread_start() */
    pthread_t 	thread_id;        /* ID returned by pthread_create() */
    enum 	tstate eState;  // refer to enum tstate above
    int       	iThread_num;       /* Application-defined thread # */
    char     	*thread_message;      /* Saying Hello */
} THREAD_INFO;

typedef struct ThreadPool {
  int iThreadID;
  uint16_t uiThreadState; 
  struct ThreadPool* pNext; 
}THREAD_POOL;

// task item in thread pool
struct task
{
    // file descriptor or user_data
    epoll_data_t data;
    // next task
    struct task* next;
};

// for data transporting
struct user_data {
    int fd;
    // real received data size
    unsigned int n_size;
    // content received
    char line[MAXBYTE];
};

typedef struct CEpollServerCtorList {

    int MaxByte;     	// 10
    int Open_Max;    	//100
    int MaxEvents;     	//20
    char szServerPort[SERVER_PORT_SIZE];   	//10012
    int _INFTIM;      	//1000
    int Local_addr; 	// "127.0.0.1"
    int iTimeOut;     	//500

    int iNumOFileDescriptors;

    uint 	nReadThreads;  // Thread Pool for Read
    uint 	nWriteThreads; // Thread Pool for Write

    int   iLoadFactor;  // Max load to a given thread until a new thread is added from the pool

    char  szUserFileName[MAX_PATH];

} EPOLL_CTOR_LIST;

typedef struct TaskQueue {
    struct task *readhead;
    struct task *readtail;

    struct task *writehead;
    struct task *writetail;

    uint64_t uiTotalReadTasks;
    uint64_t uiTotalWriteTasks;

    unsigned uiReadTasksInQ;
    unsigned uiWriteTasksInQ;

} TASK_QUEUE;

class CEpollServer {

public:
    CEpollServer(EPOLL_CTOR_LIST);
    CEpollServer(char *szFileName); //Constructor for adding user names

    ~CEpollServer();

private:  // yes yes it is by default

    int i, maxi, m_nfds;
//    int listenfd;
    int  m_Socket;
    int  connfd;

    static    int m_efd;
    int m_MaxEvents;

    int   m_iTimeOut;
    // task node
    struct task *new_task = NULL;


    char   m_szServerPort[SERVER_PORT_SIZE];
    struct sockaddr_in clientaddr;
    struct sockaddr_in serveraddr;

    // epoll descriptor from epoll_create()
    int m_epfd;

    static    struct epoll_event m_ev;
    struct epoll_event* eventList;

    int   m_iNumOFileDescriptors;

    pthread_t  *pR_Thread;  // array of read thread
    pthread_t  *pW_Thread;  // array of write threads

    static    pthread_mutex_t *pR_Mutex;
    static    pthread_cond_t  *pR_Condl;

    static    pthread_mutex_t *pW_Mutex;
    static    pthread_cond_t  *pW_Condl;

    static    void *readtask(void *args);
    static    void *writetask(void *args);
    
    static    int GetReadThreadFromPool(int  );  

    static    int GetWriteThreadFromPool(int  );  

    EPOLL_CTOR_LIST m_CtorList;

    int m_iError;
    void setnonblocking(int sock);

    int m_iReadMutexIndex;
    int m_iWriteMutexIndex;
    double Get_CPU_Time(void);

    static    string m_strLogMsg;
    static TASK_QUEUE m_TaskQue;

    char  m_szUserFileName[MAX_PATH];
    CuserDB* m_pCuserDB;

    void RemoveBlanks(char* szString);
    static bool  m_bTerminate;

    static    THREAD_INFO* m_arrReadThreadInfo;
    static    THREAD_INFO* m_arrWriteThreadInfo;    
    
public:
    int  PrepListener();
    int AuthenticateUser(char* szRecvBuffer);
    
    int GetError();
    int ProcessEpoll();
    int TerminateThreads();
    TASK_QUEUE GetQueueStatus();



};

