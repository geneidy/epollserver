// Test with this line in telnet for Login (without the last dot):
//L Amro        Amro        .
#include "EPollServer.h"

pthread_mutex_t *CEpollServer::pR_Mutex;
pthread_cond_t  *CEpollServer::pR_Condl;

pthread_mutex_t *CEpollServer::pW_Mutex;
pthread_cond_t  *CEpollServer::pW_Condl;

pthread_mutex_t R_QueueMutex;
pthread_mutex_t W_QueueMutex;

TASK_QUEUE CEpollServer::m_TaskQue = {0};
// struct task *CEpollServer::readhead = nullptr, *CEpollServer::readtail = nullptr;
// struct task *CEpollServer::writehead = nullptr, *CEpollServer::writetail = nullptr;

int 			CEpollServer::m_efd;
struct epoll_event 	CEpollServer::m_ev ;

THREAD_INFO*       CEpollServer::m_arrReadThreadInfo;
THREAD_INFO*       CEpollServer::m_arrWriteThreadInfo;

string 		   CEpollServer::m_strLogMsg;
bool CEpollServer::m_bTerminate = false;



//********************************************************************************************//
CEpollServer::CEpollServer(EPOLL_CTOR_LIST  CtorList):m_CtorList(CtorList)
{
    string strLog;
    strLog.clear();

    m_iError = -1;
    memset(m_szUserFileName, '\0', MAX_PATH);
    strcpy(m_szUserFileName, m_CtorList.szUserFileName);

    CComLog::instance().log("Epoll Server...Constructing with the following parameters: ", CComLog::Info);

    strLog = "Load Factor: " + to_string(m_CtorList.iLoadFactor);
    CComLog::instance().log(strLog , CComLog::Info);

    strLog = "Number of file descriptors: " + to_string(m_CtorList.iNumOFileDescriptors);
    CComLog::instance().log( strLog, CComLog::Info);

    strLog =  "Timeout: " + to_string(m_CtorList.iTimeOut);
    CComLog::instance().log(strLog , CComLog::Info);

    strLog = "Local Address: " +  to_string(m_CtorList.Local_addr);
    CComLog::instance().log( strLog, CComLog::Info);

    strLog = "Server Listen Port: " +  string(m_CtorList.szServerPort);
    CComLog::instance().log( strLog, CComLog::Info);

    strLog = "Max Bytes to read: " +  to_string(m_CtorList.MaxByte);
    CComLog::instance().log( strLog , CComLog::Info);

    strLog = "Max Events: "  +  to_string(m_CtorList.MaxEvents);
    CComLog::instance().log( strLog , CComLog::Info);

    strLog = "Number of Read Threads: " + to_string(m_CtorList.nReadThreads);
    CComLog::instance().log(strLog , CComLog::Info);

    strLog =  "Number of Write Threads: " + to_string(m_CtorList.nWriteThreads);
    CComLog::instance().log( strLog , CComLog::Info);

    strLog =  "Max Open descriptors: " +  to_string(m_CtorList.Open_Max);
    CComLog::instance().log( strLog , CComLog::Info);

    strLog =   "User DB File Name: " +  string(m_CtorList.szUserFileName);
    CComLog::instance().log(strLog , CComLog::Info);

    m_pCuserDB = nullptr;

    m_pCuserDB = new CuserDB(m_szUserFileName);

    if (!m_pCuserDB) {
        m_iError = 700;  // ::TODO  ENUM LATER
    }

    if (m_pCuserDB->GetError() == INVALID_USER_FILE_NAME) {
        m_iError = INVALID_USER_FILE_NAME;
        m_iError = 1001;  // ::TODO ENUM LATER
    }
    else {

        pthread_mutex_init(&R_QueueMutex, nullptr);
        pthread_mutex_init(&W_QueueMutex, nullptr);


        pR_Thread = new pthread_t [m_CtorList.nReadThreads];  // array of Read threads
        pW_Thread = new pthread_t [m_CtorList.nWriteThreads]; // array of Write threads

        pR_Mutex = new pthread_mutex_t[m_CtorList.nReadThreads ];   // array of Read  thread  mutexs
        pR_Condl = new pthread_cond_t[m_CtorList.nReadThreads];   // array of  Read conditional Variables

        pW_Mutex = new pthread_mutex_t[m_CtorList.nWriteThreads ]; 	// array of Read threads
        pW_Condl = new pthread_cond_t[m_CtorList.nWriteThreads ]; 	// array of  Write conditional Variables

        m_iNumOFileDescriptors = m_CtorList.iNumOFileDescriptors;
        m_MaxEvents = m_CtorList.MaxEvents;
        m_iTimeOut = m_CtorList.iTimeOut ;


        memset(m_szServerPort, '\0', SERVER_PORT_SIZE);
        strncpy(m_szServerPort, m_CtorList.szServerPort, SERVER_PORT_SIZE);;

        m_arrWriteThreadInfo = new  THREAD_INFO[ m_CtorList.nWriteThreads];
        m_arrReadThreadInfo = new  THREAD_INFO[m_CtorList.nReadThreads];

        struct timespec    m_request, m_remain;
        m_request.tv_sec = 0;
        m_request.tv_nsec = 100000000;   // 1/10 of a micro second

        m_remain.tv_sec = 0;
        m_remain.tv_nsec = 0;

        for (uint ii = 0; ii < m_CtorList.nReadThreads; ii++) {
            pthread_mutex_init(&pR_Mutex[ii], nullptr);
            pthread_cond_init(&pR_Condl[ii], nullptr);

            m_arrReadThreadInfo[ii].eState = TS_INACTIVE;

            pthread_create(&m_arrReadThreadInfo[ii].thread_id, nullptr, readtask, &ii);
            nanosleep (&m_request, &m_remain);  // sleep a 1/10 of a second

            m_arrReadThreadInfo[ii].eState = TS_WAITING;
        }

        for (uint ii = 0 ; ii <  m_CtorList.nWriteThreads; ii++) {
            pthread_mutex_init(&pW_Mutex[ii], nullptr);
            pthread_cond_init(&pW_Condl[ii], nullptr);

            m_arrWriteThreadInfo[ii].eState = TS_INACTIVE;

            pthread_create(&m_arrWriteThreadInfo[ii].thread_id, nullptr, writetask, &ii);
            nanosleep (&m_request, &m_remain);  // sleep a 1/10 of a second
            m_arrWriteThreadInfo[ii].eState = TS_WAITING;
//            PutWriteThreadInPool(ii);  // Done....Put it back in  the pool
        }

        m_efd = epoll_create(m_MaxEvents);
        eventList = nullptr;

        eventList = new epoll_event[m_MaxEvents];
        if (!eventList) {
            m_iError = 1000;
            // ::TODO log error
        }

        m_iReadMutexIndex 	= 0;
        m_iWriteMutexIndex  	= 0;

        m_TaskQue.readhead 		= nullptr;
        m_TaskQue.readtail 		= nullptr;

        m_TaskQue.uiReadTasksInQ  	= 0;
        m_TaskQue.uiWriteTasksInQ 	= 0;

        m_TaskQue.uiTotalWriteTasks = 0;
        m_TaskQue.uiTotalReadTasks  = 0;

        m_TaskQue.writehead 	= nullptr;
        m_TaskQue.readtail 	= nullptr;

        m_bTerminate = false;
        CComLog::instance().log("Epoll Server...Successfull Construction", CComLog::Info);
    }
}
//********************************************************************************************//
CEpollServer::~CEpollServer()
{

    // Terminate all threads
    m_bTerminate = true;

    // Release Thread Pool Read and Write

    CComLog::instance().log("Terminating Threads", CComLog::Info);
    TerminateThreads();

    CComLog::instance().log("Closing File descriptors", CComLog::Info);

    close(m_efd);  // Amro added this one...author did not close the file descriptor

    CComLog::instance().log("Deleting Events", CComLog::Info);
    if (eventList)
        delete[] eventList;

    CComLog::instance().log("Deleting Threads Array", CComLog::Info);

    if (pR_Thread)
        delete [] pR_Thread  ;
    if (pW_Thread)
        delete []  pW_Thread ;

    CComLog::instance().log("Deleting Mutex Array", CComLog::Info);

    if (pR_Mutex)
        delete []  pR_Mutex ;
    if (pW_Mutex)
        delete []  pW_Mutex ;

    CComLog::instance().log("Deleting Conditional Variables Array", CComLog::Info);

    if (pR_Condl)
        delete []  pR_Condl ;
    if (pW_Condl)
        delete []  pW_Condl ;

    CComLog::instance().log("Deleting Threads Array", CComLog::Info);
    if (m_arrWriteThreadInfo)
        delete [] m_arrWriteThreadInfo;

    if (m_arrReadThreadInfo)
        delete [] m_arrReadThreadInfo;

    if (m_pCuserDB) {
        delete (m_pCuserDB);
    }
    CComLog::instance().log("===========================================================================================================================", CComLog::Info);
    CComLog::instance().log("Destruction Completed", CComLog::Info);
    CComLog::instance().log("===========================================================================================================================", CComLog::Info);
}
//********************************************************************************************//
int CEpollServer::AuthenticateUser(char* szRecvBuffer)
{
    if (strlen(szRecvBuffer) < SIZE_OF_LOGIN_MESSAGE) {
        return INVALID_LOGIN_MESSAGE;
    }

    char szUserName[SIZE_OF_USERNAME];
    char szPassword[SIZE_OF_PASSWORD];

    if (*szRecvBuffer != 'L') {
        return INVALID_LOGIN_MESSAGE;
    }

    memset(szUserName, '\0', SIZE_OF_USERNAME);
    memset(szPassword,  '\0', SIZE_OF_PASSWORD);

    memmove(szUserName, szRecvBuffer+2 , 11 );
    RemoveBlanks(szUserName);

    memmove(szPassword, szRecvBuffer+14, 11);
    RemoveBlanks(szPassword);

    int iRet = m_pCuserDB->VerifyUser(szUserName, szPassword);

    return iRet;
}
//********************************************************************************************//
void CEpollServer::RemoveBlanks(char* szString)
{
    char* ptr = nullptr;

    ptr = szString + strlen(szString) -1 ;

    while (*(--ptr) ==  0x20)  // ascii 32 = space
        ;
    *(++ptr) = '\0';
}
//********************************************************************************************//
void CEpollServer::setnonblocking(int iSocket)
{
    int opts;
    if ((opts = fcntl(iSocket, F_GETFL)) < 0) {
        m_strLogMsg = "GETFL " + to_string( iSocket) + " Failed"; //"GETFL %d failed", iSocket);
        CComLog::instance().log(m_strLogMsg, CComLog::Error);
    }

    int iRet = 0;
    opts = opts | O_NONBLOCK;
    if (fcntl(iSocket, F_SETFL, opts) < 0) {
        m_strLogMsg = "SETFL " + to_string( iSocket) + " Failed"; //"GETFL %d failed", iSocket);
        CComLog::instance().log(m_strLogMsg, CComLog::Error);


        int iSendBuffSize = 9728;   // Set Send Buffer Size Accordingly
        int iRecvBuffSize = 2048;   // Set Recv


        iRet =   setsockopt(iSocket, SOL_SOCKET, SO_SNDBUF, &iSendBuffSize, sizeof iSendBuffSize);
        // check for errors by getiSocketopt
        iRet =   setsockopt(iSocket, SOL_SOCKET, SO_RCVBUF, &iRecvBuffSize, sizeof iRecvBuffSize);
        // check for errors by getiSocketopt
    }
}
////////////////////////////////////////////////////////////////////////////////////////////
int CEpollServer::PrepListener()
{

//  https://banu.com/blog/2/how-to-use-epoll-a-complete-example-in-c/epoll-example.c

    struct addrinfo hints;
    struct addrinfo *result;
    struct addrinfo *rp;
    int s;

    memset (&hints, 0, sizeof (struct addrinfo));
    hints.ai_family = AF_UNSPEC;     /*  IPv4 and IPv6 choices */
    hints.ai_socktype = SOCK_STREAM; /*  TCP socket */
    hints.ai_flags = AI_PASSIVE| AI_V4MAPPED;     /* All interfaces */

    s = getaddrinfo (nullptr, m_szServerPort, &hints, &result);
    if (s != 0)
    {
//      fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (s));

        m_iError = 510;  // ::TODO enum all the errors
        return -1;

    }

    for (rp = result; rp != nullptr; rp = rp->ai_next)
    {
        m_Socket = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (m_Socket == -1)
            continue;

        s = bind (m_Socket, rp->ai_addr, rp->ai_addrlen);
        if (s == 0) {  // succefull bind
            break;
        }
        close (m_Socket);
    }

    if (rp == nullptr)
    {
        fprintf (stderr, "Could not bind\n");
        m_iError = 510;  // ::TODO enum all the errors
        return -1;
    }

    freeaddrinfo (result);
    s = listen(m_Socket, m_MaxEvents);

    if (s == -1)
    {
        m_iError = 520;  // ::TODO enum all the errors
        return -1;
    }

    m_ev.data.fd = m_Socket;
    m_ev.events = EPOLLIN | EPOLLET;

    s = epoll_ctl (m_efd, EPOLL_CTL_ADD, m_Socket, &m_ev);

    if (s == -1)
    {
        perror ("epoll_ctl");
        abort ();
    }

    return m_Socket;  // an non zero int
}
////////////////////////////////////////////////////////////////////////////////////////////
int CEpollServer::GetError()
{
    return m_iError;
}
////////////////////////////////////////////////////////////////////////////////////////////
int CEpollServer::ProcessEpoll()
{

    struct sockaddr in_addr;
    socklen_t in_len = sizeof (in_addr);

    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

    int iRet = 0;

    int iRetry = 0;

    char szInBuffer[MAXBYTE ];
    memset(szInBuffer, '\0', MAXBYTE) ;

    CComLog::instance().log("Entering epoll loop waiting for connections", CComLog::Error);
    for(;;)
    {
        // TEST ONLY...Threw away code
        iRetry++;
        if (iRetry > 15)
            m_bTerminate = true;
        // TEST ONLY...Threw away code

        // waiting for epoll event
        m_nfds = epoll_wait(m_efd, eventList, m_MaxEvents, m_iTimeOut);
        if (m_bTerminate)
            break;

        // In case of edge trigger, must go over each event
        for (i = 0; i < m_nfds; ++i)
        {
            // Get new connection
            if (eventList[i].data.fd == m_Socket)
            {
                // accept the client connection
//                connfd = accept(m_Socket, (struct sockaddr*)&clientaddr, &in_len);
                connfd = accept(m_Socket, &in_addr, &in_len);

                if (connfd == -1) {
                    CComLog::instance().log("connfd < 0", CComLog::Error);
                    iRet++;
                    if ((errno == EAGAIN) /*|| (errno == EWOULDBLOCK)*/) { /* We have processed all incoming connections. */
                        break;
                    }
                    else {
                        CComLog::instance().log("accept error", CComLog::Error);
                        break;
                    }
                } // if (connfd == -1) {


                int nRecv;
                string strMsg ;
//                if (( nRecv = recv(connfd, szInBuffer, MAXBYTE, MSG_WAITALL)) > 0) {
                if (( nRecv = recv(connfd, szInBuffer, MAXBYTE, 0)) > 0) {
                    nRecv = AuthenticateUser( szInBuffer);
                    if (nRecv != VALID_USER) {
                        strMsg.clear();
                        strMsg = "Connection Accept Error, following message was received ";
                        strMsg += szInBuffer;
                        CComLog::instance().log(strMsg, CComLog::Error);
                        strMsg.clear();
                        strMsg = "R";  // response message
                        strMsg += "R"; // rejection
                        strMsg += to_string(nRecv);
                        send(connfd, strMsg.c_str(), 3, 0);
                        close(connfd);
                        continue;
                    }
                }
                else {
                    switch (errno ) {
                    case EAGAIN:
                        break;
                    case EBADF:
                        break;
                    case ECONNREFUSED:
                        break;
                    case EFAULT:
                        break;
                    case EINTR:
                        break;
                    case EINVAL:
                        break;
                    case ENOMEM:
                        break;
                    case ENOTCONN:
                        break;
                    case ENOTSOCK:
                        break;
                    }
                    continue;
                }
                strMsg = "R";  // response message
                strMsg += "A"; // accepted
                strMsg += to_string(nRecv);
                send(connfd, strMsg.c_str(), 3, 0);

                setnonblocking(connfd);
                m_strLogMsg = "[SERVER] connect from";
                m_strLogMsg += inet_ntoa(clientaddr.sin_addr);
                CComLog::instance().log(m_strLogMsg, CComLog::Debug);

                iRet = getnameinfo (&in_addr, in_len,  hbuf, sizeof hbuf, sbuf, sizeof sbuf, NI_NUMERICHOST | NI_NUMERICSERV);
                if (iRet == 0) {
                    m_strLogMsg = "Accepted connection on descriptor:  " +  to_string(connfd) + "Host: " + hbuf + " Address: " +inet_ntoa(clientaddr.sin_addr) +"Port: " + sbuf  ;
                    CComLog::instance().log(m_strLogMsg, CComLog::Info);

                }

                m_ev.data.fd = connfd;
                m_ev.events = EPOLLIN | EPOLLET;  // Edge Trigger
                epoll_ctl(m_efd, EPOLL_CTL_ADD, connfd, &m_ev); // add fd to epoll queue
            }  // if (eventList[i].data.fd == m_Socket)
            // Received data ?
            else if (eventList[i].events & EPOLLIN)
            {
                if (eventList[i].data.fd < 0)
                    continue;
                m_strLogMsg = "[SERVER] put task: "+ to_string(eventList[i].data.fd) + " To read queue" ;

                CComLog::instance().log(m_strLogMsg, CComLog::Info);

                new_task = new task;
                new_task->data.fd = eventList[i].data.fd;
                new_task->next = nullptr;
                //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d epollin before lock\n", pthread_self());
                // protect task queue (readhead/readtail)

                while ((m_iReadMutexIndex = GetReadThreadFromPool(m_CtorList.nReadThreads)) == -1) {
                    sleep(1);
                    if (m_bTerminate)
                        break;
                }

                pthread_mutex_lock(&pR_Mutex[m_iReadMutexIndex]);
                //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d epollin after lock\n", pthread_self());
                // the queue is empty
                if (m_TaskQue.readhead == nullptr)
                {
                    m_TaskQue.readhead = new_task;
                    m_TaskQue.readtail = new_task;
                }
                // queue is not empty
                else
                {
                    m_TaskQue.readtail->next = new_task;
                    m_TaskQue.readtail = new_task;
                }
                m_TaskQue.uiReadTasksInQ++;
                m_TaskQue.uiTotalReadTasks++;

                pthread_cond_broadcast(&pR_Condl[ m_iReadMutexIndex]);  // activate it

                //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d epollin before unlock\n", pthread_self());
                pthread_mutex_unlock(&pR_Mutex[m_iReadMutexIndex]);
                //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d epollin after unlock\n", pthread_self());
            }
            // Have data to send
            else if (eventList[i].events & EPOLLOUT)
            {
                if (eventList[i].data.ptr == nullptr)
                    continue;

                m_strLogMsg = "[SERVER] put task: " + to_string(((struct task*)eventList[i].data.ptr)->data.fd) + " To write queue";
                CComLog::instance().log(m_strLogMsg, CComLog::Info);

                new_task = new task;
                new_task->data.ptr = (struct user_data*)eventList[i].data.ptr;
                new_task->next = nullptr;
                //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d epollout before lock\n", pthread_self());

                while ((m_iWriteMutexIndex = GetWriteThreadFromPool(m_CtorList.nWriteThreads)) == -1) {
                    sleep(1);  // wait for a thread to be avialable
                    if (m_bTerminate)
                        break;
                }

                pthread_mutex_lock(&pW_Mutex[m_iWriteMutexIndex]);
                //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d epollout after lock\n", pthread_self());
                if (m_TaskQue.writehead == nullptr)
                {
                    m_TaskQue.writehead = new_task;
                    m_TaskQue.writetail = new_task;
                }
                else
                {
                    m_TaskQue.writetail->next = new_task;
                    m_TaskQue.writetail = new_task;
                }
                m_TaskQue.uiWriteTasksInQ++;
                m_TaskQue.uiTotalWriteTasks++;

                // trigger writetask thread
                pthread_cond_broadcast(&pW_Condl[m_iWriteMutexIndex]);
//                PutWriteThreadInPool(m_iWriteMutexIndex);
                //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d epollout before unlock\n", pthread_self());
                pthread_mutex_unlock(&pW_Mutex[m_iWriteMutexIndex]);
                //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d epollout after unlock\n", pthread_self());
            }
            else
            {
                CComLog::instance().log("[SERVER] Error: unknown epoll event", CComLog::Error);
            }
        }
    }
    CComLog::instance().log("Exited from epoll loop ", CComLog::Info);

    return 0;
}
//********************************************************************************************//
void *CEpollServer::readtask(void *args)
{
    int iThreadReadIndex = *((int*) args);

    int fd = -1;
    int n, i;

    struct task* tmp = nullptr;

    struct user_data* data = nullptr;
    while(1)
    {
        //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d readtask before lock\n", pthread_self());
        // protect task queue (readhead/readtail)
        pthread_mutex_lock(&pR_Mutex[iThreadReadIndex ]);
        //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d readtask after lock\n", pthread_self());
        while ((m_TaskQue.readhead == nullptr)  && (!m_bTerminate)) { //  if condl false, will unlock mutex
            m_arrReadThreadInfo[iThreadReadIndex].eState = TS_WAITING;
            pthread_cond_wait(&pR_Condl[iThreadReadIndex ], &pR_Mutex[iThreadReadIndex ]);
        }

        if (m_bTerminate) {
            m_arrReadThreadInfo[iThreadReadIndex].eState = TS_STOPPING;
            pthread_mutex_unlock(&pR_Mutex[iThreadReadIndex ]);
            break;
        }

        m_arrReadThreadInfo[iThreadReadIndex].eState = TS_RUNNING;

        // copy task info and remove it from the Task Queue
        fd = m_TaskQue.readhead->data.fd;
        tmp = m_TaskQue.readhead;
        m_TaskQue.readhead = m_TaskQue.readhead->next;
        delete(tmp);
        m_TaskQue.uiReadTasksInQ--;

        //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d readtask before unlock\n", pthread_self());
        pthread_mutex_unlock(&pR_Mutex[iThreadReadIndex ]);
        //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d readtask after unlock\n", pthread_self());
        m_strLogMsg = "[SERVER] readtask "+ to_string (pthread_self())  + " handling "   +  to_string(fd);

        CComLog::instance().log(m_strLogMsg, CComLog::Info);

//        data = (user_data*)malloc(sizeof(struct user_data));
        data =  new (user_data);

        data->fd = fd;
        if ((n = recv(fd, data->line, MAXBYTE, 0)) < 0) {
            if (errno == ECONNRESET)
                close(fd);
            m_strLogMsg = "[SERVER] Error: readline failed: " ;
            m_strLogMsg +=  strerror(errno);
//            CComLog::instance().log(m_strLogMsg);"[SERVER] Error: readline failed: %s\n", strerror(errno));
            CComLog::instance().log(m_strLogMsg, CComLog::Error);
            if (data != nullptr)
                delete(data);
        }
        else if (n == 0) {
            close(fd);
            m_strLogMsg = "[SERVER] Error: client: " + to_string(fd) + " closed connection " ;
            CComLog::instance().log(m_strLogMsg, CComLog::Error);
            if (data != nullptr)
                delete(data);
        }
        else {
            data->n_size = n;
            for (i = 0; i < n; ++i) {
                if (data->line[i] == '\n' || data->line[i] > 128)
                {
                    data->line[i] = '\0';
                    data->n_size = i + 1;
                }
            }
            m_strLogMsg = "[SERVER] readtask: "  + to_string (pthread_self()) + to_string(fd) + to_string(data->n_size) + data->line;
            CComLog::instance().log(m_strLogMsg, CComLog::Info);

            if (data->line[0] != '\0') {
                // modify monitored event to EPOLLOUT,  wait next loop to send respond
                m_ev.data.ptr = data;
                m_ev.events = EPOLLOUT | EPOLLET;
                epoll_ctl(m_efd, EPOLL_CTL_MOD, fd, &m_ev);
            }
        }
        if (m_arrReadThreadInfo[iThreadReadIndex].eState == TS_STOPPING ) {
            break;
        }
    }
    m_arrReadThreadInfo[iThreadReadIndex].eState = TS_TERMINATED;
    return nullptr;
}
//********************************************************************************************//
void *CEpollServer::writetask(void *args)
{
    int iThreadIndex = *((int*) args);
    unsigned int n;
    // data to write back to client
    struct user_data *rdata = nullptr;

    while(1)
    {
        //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d writetask before lock\n", pthread_self());
        pthread_mutex_lock(&pW_Mutex[iThreadIndex ]);
        //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d writetask after lock\n", pthread_self());
        while ((m_TaskQue.writehead == nullptr) && (!m_bTerminate)) {
            // if condl false, will unlock mutex
            m_arrWriteThreadInfo[iThreadIndex].eState = TS_WAITING;
            pthread_cond_wait(&pW_Condl[iThreadIndex ], &pW_Mutex[iThreadIndex ]);
        }

        if (m_bTerminate) {
            m_arrWriteThreadInfo[iThreadIndex].eState = TS_STOPPING;
            pthread_mutex_unlock(&pW_Mutex[iThreadIndex ]);
            break;
        }

        m_arrWriteThreadInfo[iThreadIndex].eState = TS_RUNNING;

        rdata = (struct user_data*)m_TaskQue.writehead->data.ptr;

        struct task* tmp = m_TaskQue.writehead;  // temp points to Queue head
        m_TaskQue.writehead = m_TaskQue.writehead->next;   // Queue head point to next
        delete(tmp);					// delete Queue head
        m_TaskQue.uiWriteTasksInQ--;

        //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d writetask before unlock\n", pthread_self());
        pthread_mutex_unlock(&pW_Mutex[iThreadIndex ]);
        //CComLog::instance().log(m_strLogMsg);"[SERVER] thread %d writetask after unlock\n", pthread_self());

        m_strLogMsg = "[SERVER] writetask: " + to_string(pthread_self()) + " Sending: " +  to_string(rdata->fd )  + to_string(rdata->n_size) + rdata->line;
//      CComLog::instance().log(m_strLogMsg);"[SERVER] writetask %d sending %d : [%d] %s\n", pthread_self(), rdata->fd, rdata->n_size, rdata->line);
        CComLog::instance().log(m_strLogMsg, CComLog::Info);

        // send responce to client
        if ((n = send(rdata->fd, rdata->line, rdata->n_size, 0)) < 0)
        {
            if (errno == ECONNRESET)
                close(rdata->fd);
            m_strLogMsg = "[SERVER] Error: send responce failed: ";
            m_strLogMsg +=  strerror(errno);
            CComLog::instance().log(m_strLogMsg, CComLog::Error);
        }
        else if (n == 0)
        {
            close(rdata->fd);
            CComLog::instance().log("[SERVER] Error: client closed connection.", CComLog::Info);
        }
        else
        {
            m_ev.data.fd = rdata->fd;
            m_ev.events = EPOLLIN | EPOLLET;
            epoll_ctl(m_efd, EPOLL_CTL_MOD, rdata->fd, &m_ev);
        }
        delete(rdata);
        if (m_arrWriteThreadInfo[iThreadIndex].eState == TS_STOPPING ) {
            break;
        }
    }
    m_arrWriteThreadInfo[iThreadIndex].eState = TS_TERMINATED;

    return nullptr;
}
//********************************************************************************************//
/*
double CEpollServer::Get_CPU_Time(void)  // incomplete method....
{

    double user = 0.0, sys = 0.0;
    

        struct rusage myusage, childusage;
        if (getrusage(RUSAGE_SELF, &myusage) < 0)
        {
            CComLog::instance().log("[SERVER] Error: getrusage(RUSAGE_SELF) failed", CComLog::Error);
            return 0;
        }
        if (getrusage(RUSAGE_CHILDREN, &childusage) < 0)
        {
            CComLog::instance().log("[SERVER] Error: getrusage(RUSAGE_CHILDREN) failed", CComLog::Error);
            return 0;
        }
        user = (double)myusage.ru_utime.tv_sec + myusage.ru_utime.tv_usec/1000000.0;
        user += (double)childusage.ru_utime.tv_sec + childusage.ru_utime.tv_usec/1000000.0;
        sys = (double)myusage.ru_stime.tv_sec + myusage.ru_stime.tv_usec/1000000.0;
        sys += (double)childusage.ru_stime.tv_sec + childusage.ru_stime.tv_usec/1000000.0;
        // show total user time and system time
        char  Msg[100];
        memset(Msg, '\0', 100);
        sprintf(Msg, "[SERVER] user time=%g, sys time=%g\n", user, sys);
        CComLog::instance().log(Msg, CComLog::Info);
    
    return sys;
}
*/
//********************************************************************************************//
int CEpollServer::TerminateThreads()
{
    std::string strExitMessage;

    m_bTerminate = true;
    sleep(1);
    // Clear the Task Queue first here ::TODO

    CComLog::instance().log("Terminating Read Threads", CComLog::Debug);
    for (uint ii = 0;  ii <  m_CtorList.nReadThreads; ii++ ) {  // send a Stop message to all threads
        m_TaskQue.readhead = nullptr;
        m_arrReadThreadInfo[ii].eState = TS_STOPPING;
        pthread_cond_broadcast(&pR_Condl[ii]);   // Wil cause the while(1) loop to continue and check for TS_STOPPING  to terminate
    }
    CComLog::instance().log("Terminating Write Threads", CComLog::Debug);
    for (uint ii = 0;  ii < m_CtorList.nWriteThreads; ii++ ) {  // send a Stop message to all threads
        m_TaskQue.writehead = nullptr;
        m_arrWriteThreadInfo[ii].eState = TS_STOPPING;
        pthread_cond_broadcast(&pW_Condl[ii]);
    }

    CComLog::instance().log("===========================================================================================================================", CComLog::Info);
    CComLog::instance().log("Joining Read Threads", CComLog::Info);
    CComLog::instance().log("===========================================================================================================================", CComLog::Info);

    uint iJoined = 0;
    int iRet = 0;

    while (iJoined < m_CtorList.nReadThreads ) {
        // keep on checking for all terminated threads every three seconds

        for (uint ii = 0;  ii < m_CtorList.nReadThreads; ii++ ) {
            if ((m_arrReadThreadInfo[ii].eState == TS_JOINED))
                continue;
            if (m_arrReadThreadInfo[ii].eState == TS_TERMINATED) {
                iRet =  pthread_join(m_arrReadThreadInfo[ii].thread_id, nullptr);
                if (iRet != 0) {
                    switch (errno) {
                    case EDEADLK:
                        break;
                        //A deadlock was detected (e.g., two threads tried to join with each other); or thread specifies the calling thread.
//		    case EINVAL: break;
                        //thread is not a joinable thread.
                    case EINVAL:
                        break;
                        //Another thread is already waiting to join with this thread.
                    case ESRCH:
                        break;
                        //No thread with the ID thread could be found.
                    }
                }
                m_arrReadThreadInfo[ii].eState = TS_JOINED;

                strExitMessage.clear();
                strExitMessage = "Thread Number: ";
                strExitMessage += to_string(m_arrReadThreadInfo[ii].thread_id);
                strExitMessage += " Joined";
                CComLog::instance().log(strExitMessage, CComLog::Debug);
                strExitMessage = "Joined: ";
                strExitMessage += to_string(iJoined +1) + " Thread  out of: " + to_string(m_CtorList.nReadThreads);
                CComLog::instance().log(strExitMessage, CComLog::Debug);
                iJoined++;
            }
        } // for loop
        sleep(3);
    } // while loop

    CComLog::instance().log("All Read Threads Joined", CComLog::Debug);
    iJoined = 0;

    CComLog::instance().log("===========================================================================================================================", CComLog::Info);
    CComLog::instance().log("Joining Write Threads", CComLog::Info);
    CComLog::instance().log("===========================================================================================================================", CComLog::Info);


    while (iJoined < m_CtorList.nWriteThreads ) {
        // keep on checking for all terminated threads every three seconds

        for (uint ii = 0;  ii < m_CtorList.nWriteThreads; ii++ ) {
            if ((m_arrWriteThreadInfo[ii].eState == TS_JOINED))
                continue;
            if (m_arrWriteThreadInfo[ii].eState == TS_TERMINATED) {

                iRet =  pthread_join(m_arrWriteThreadInfo[ii].thread_id, nullptr);
                if (iRet != 0) {
                    switch (errno) {
                    case EDEADLK:
                        break;
                        //A deadlock was detected (e.g., two threads tried to join with each other); or thread specifies the calling thread.
//		    case EINVAL: break;
                        //thread is not a joinable thread.
                    case EINVAL:
                        break;
                        //Another thread is already waiting to join with this thread.
                    case ESRCH:
                        break;
                        //No thread with the ID thread could be found.
                    }
                }

                m_arrWriteThreadInfo[ii].eState = TS_JOINED;

                strExitMessage.clear();
                strExitMessage = "Thread Number: ";
                strExitMessage += to_string(m_arrWriteThreadInfo[ii].thread_id);
                strExitMessage += " Joined";
                CComLog::instance().log(strExitMessage, CComLog::Debug);
                strExitMessage = "Joined: ";
                strExitMessage += to_string(iJoined +1) + " Thread  out of: " + to_string(m_CtorList.nWriteThreads);
                CComLog::instance().log(strExitMessage, CComLog::Debug);
                iJoined++;
            }
        } // for loop
        sleep(3);
    } // while loop

    CComLog::instance().log("All Write Threads Joined", CComLog::Debug);
    CComLog::instance().log("===========================================================================================================================", CComLog::Info);
    CComLog::instance().log("Destroying Read Mutexes and Cond. Variables", CComLog::Debug);

    for (uint ii = 0; ii < m_CtorList.nReadThreads; ii++) {
        pthread_cond_destroy(&pR_Condl[ii]);   // Destroy write conditional Variables
        pthread_mutex_destroy(&pR_Mutex[ii]);  // Destroy write mutex

    }
    CComLog::instance().log("All Read Mutexes Destroyed", CComLog::Debug);
    CComLog::instance().log("===========================================================================================================================", CComLog::Info);
    CComLog::instance().log("Destroying Write Mutexes and Cond. Variables", CComLog::Debug);
    for (uint ii = 0 ; ii <  m_CtorList.nWriteThreads; ii++) {
        pthread_cond_destroy(&pW_Condl[ii]);   // Destroy read conditional Variables
        pthread_mutex_destroy(&pW_Mutex[ii]);  // Destroy read mutex
    }
    CComLog::instance().log("All Write Mutexes Destroyed", CComLog::Debug);
    CComLog::instance().log("===========================================================================================================================", CComLog::Info);
    return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TASK_QUEUE CEpollServer::GetQueueStatus()
{
    return m_TaskQue;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CEpollServer::GetReadThreadFromPool(int nReadThreads)
{
    pthread_mutex_lock(&R_QueueMutex);
    for (int ii = 0; ii < nReadThreads; ii++) {
        if (m_arrReadThreadInfo[ii].eState == TS_WAITING) {
            m_arrReadThreadInfo[ii].eState = TS_STARTING;
            pthread_mutex_unlock(&R_QueueMutex);
            return ii;
        }
    }
    pthread_mutex_unlock(&R_QueueMutex);
    return -1;

}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CEpollServer::GetWriteThreadFromPool(int nWriteThreads)
{
    pthread_mutex_lock(&W_QueueMutex);
    for (int ii = 0; ii < nWriteThreads; ii++) {
        if (m_arrWriteThreadInfo[ii].eState == TS_WAITING) {
            m_arrWriteThreadInfo[ii].eState = TS_STARTING;
            pthread_mutex_unlock(&W_QueueMutex);
            return ii;
        }
    }
    pthread_mutex_unlock(&W_QueueMutex);
    return -1;

}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////// Experiment:: begin ////////////////////////
float Q_rsqrt( float number );

//********************************************************************************************//
float Q_rsqrt( float number )
{
	long i;
	float x2, y;
	const float threehalfs = 1.5F;
 
	x2 = number * 0.5F;
	y  = number;
	i  = * ( long * ) &y;                       // evil floating point bit level hacking
	i  = 0x5f3759df - ( i >> 1 );               // what the fuck? 
	y  = * ( float * ) &i;
	y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
//	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed
 
	return y;
}
//////// Experiment:: End ////////////////////////
/*

void CTestClass::MyFunction(char* szString)
{
    char* ptr = nullptr;

    ptr = szString + strlen(szString) -1 ;

    while (*(--ptr) ==  0x20) 
        ;
    *(++ptr) = '\0';
}*/
