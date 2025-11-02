#include <stdio.h>
#include <tchar.h>
#include <winsock2.h>
#include <string>
#include <iostream>
#include <atlbase.h>
#include <vector>
#include <string>
#include <time.h>
#include <windows.h>
using namespace std;
int i_am_manager = 1;
int nPORT = 7864;
struct IPv4
{
    unsigned char b1, b2, b3, b4;
};

typedef struct evt_func_data
{
    char ip[17];
    POINT pt_mouse_xy;
    RECT rect_screen_dim;
    int evt;
    float p;
    bool new_evt;
public:
    evt_func_data()
    {
        pt_mouse_xy.x = -1;
        pt_mouse_xy.y = -1;
        rect_screen_dim.left = -1;
        rect_screen_dim.right = -1;
        rect_screen_dim.top = -1;
        rect_screen_dim.bottom = -1;
        evt = 0;
        p = 0.0;
        new_evt = false;
        memset(ip, 0, sizeof(ip));
    }
    evt_func_data(const evt_func_data& efd)
    {
        pt_mouse_xy = efd.pt_mouse_xy;
        rect_screen_dim = efd.rect_screen_dim;
        evt = efd.evt;
        p = efd.p;
        new_evt = false;
        memcpy(ip, efd.ip, sizeof(ip));
    }
}evt_func_data;
std::vector<evt_func_data> v_evt_func_data;
std::vector<std::string> v_broadcasters;

int listen_broadcast = 1;

void add_broadcaster(char* b)
{
    evt_func_data efd;
    for (std::vector<std::string>::iterator it = v_broadcasters.begin(); it != v_broadcasters.end(); ++it)
        if (0 == strcmp((*it).c_str(), b))
            return;
    v_broadcasters.push_back(b);
    strncpy_s(efd.ip, b, sizeof(efd.ip));
    v_evt_func_data.push_back(efd);
}

void set_event(char* b, char* data)
{
    int coma_index[3];
    int i = 0;
    std::vector<evt_func_data>::iterator broadcaster_it;
    if (NULL == data || strlen(data) < 5) //E,X,Y
        return;
    for (std::vector<evt_func_data>::iterator it = v_evt_func_data.begin(); it != v_evt_func_data.end(); ++it)
        if (0 == strcmp((*it).ip, b))
        {
            broadcaster_it = it;
            break;
        }
    if (v_evt_func_data.end() == broadcaster_it)
        return;
    std::string str = data;
    std::size_t found = str.find_first_of(',');
    while (found != std::string::npos)
    {
        if(i < 3)
            coma_index[i++] = found;
        found = str.find_first_of(',', found + 1);
    }
    data[coma_index[0]] = '\0';
    data[coma_index[1]] = '\0';
    data[coma_index[2]] = '\0';
    int evt = atoi(data);
    int x = atoi(data + coma_index[0]);
    int y = atoi(data + coma_index[1]);
    int p_evt = (*broadcaster_it).evt;
    POINT p_mouse_xy = (*broadcaster_it).pt_mouse_xy;
    (*broadcaster_it).evt = evt;
    (*broadcaster_it).pt_mouse_xy.x = x;
    (*broadcaster_it).pt_mouse_xy.y = y;
    if (p_evt != evt)
        (*broadcaster_it).new_evt = true;
    if (p_mouse_xy.x != x || p_mouse_xy.y != y)
        (*broadcaster_it).new_evt = true;
}
void set_screen_config(char* b, char* data)
{
    int coma_index[1];
    int i = 0;
    std::vector<evt_func_data>::iterator broadcaster_it;
    if (NULL == data || strlen(data) < 5) //E,X,Y
        return;
    for (std::vector<evt_func_data>::iterator it = v_evt_func_data.begin(); it != v_evt_func_data.end(); ++it)
        if (0 == strcmp((*it).ip, b))
        {
            broadcaster_it = it;
            break;
        }
    if (v_evt_func_data.end() == broadcaster_it)
        return;
    std::string str = data;
    std::size_t found = str.find_first_of(',');
    while (found != std::string::npos)
    {
        if (i < 1)
            coma_index[i++] = found;
        found = str.find_first_of(',', found + 1);
    }
    data[coma_index[0]] = '\0';
    int w = atoi(data);
    int h = atoi(data + coma_index[0]);
    (*broadcaster_it).rect_screen_dim.left = 0;
    (*broadcaster_it).rect_screen_dim.top = 0;
    (*broadcaster_it).rect_screen_dim.right = w;
    (*broadcaster_it).rect_screen_dim.bottom = h;
}
void set_probability(char* b, char* data)
{
    int i = 0;
    std::vector<evt_func_data>::iterator broadcaster_it;
    if (NULL == data || strlen(data) < 5) //E,X,Y
        return;
    for (std::vector<evt_func_data>::iterator it = v_evt_func_data.begin(); it != v_evt_func_data.end(); ++it)
        if (0 == strcmp((*it).ip, b))
        {
            broadcaster_it = it;
            break;
        }
    if (v_evt_func_data.end() == broadcaster_it)
        return;
    std::string str = data;
    float prob = stof(str);
    (*broadcaster_it).p = prob;
}
void process_message(char* broadcaster, int serverportno, char* data)
{
    if (NULL != data && strlen(data) > 1)
    {
        switch (data[0])
        {
        case 'B'://broadcaster address
            if(0 == strcmp(broadcaster, (data +1)))
                add_broadcaster(broadcaster);
            break;
        case 'C'://x,y coordinate
            set_event(broadcaster, (data + 1));
            break;
        case 'S'://screen configuration
            set_screen_config(broadcaster, (data + 1));//w,h
            break;
        case 'P'://probability
            set_probability(broadcaster, (data + 1));
            break;
        }
    }
}

DWORD WINAPI recv_broadcast_thread(void* param)//int port)
{
    SOCKET clientsocket;
    int portno = nPORT;// port;

    clientsocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (clientsocket == -1)
    {
        return -1;
    }
    SOCKADDR_IN UDPserveraddr;
    memset(&UDPserveraddr, 0, sizeof(UDPserveraddr));
    UDPserveraddr.sin_family = AF_INET;
    UDPserveraddr.sin_port = htons(portno);
    UDPserveraddr.sin_addr.s_addr = INADDR_ANY;
    int len = sizeof(UDPserveraddr);
    if (bind(clientsocket, (SOCKADDR*)&UDPserveraddr, sizeof(SOCKADDR_IN)) < 0)
    {
        return -1;
    }
	while (listen_broadcast)
	{
		fd_set fds;
		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 100;
		FD_ZERO(&fds);
		FD_SET(0 ,&fds);
		int rc = select(sizeof(fds) * 8, &fds, NULL, NULL, &timeout);
		if (rc > 0)
		{
			char rbuf[1024];
			SOCKADDR_IN clientaddr;
			int len = sizeof(clientaddr);
			if (recvfrom(clientsocket, rbuf, 1024, 0, (sockaddr*)&clientaddr, &len) > 0)
			{
				for (int i = 1023; i >= 1; i--)
				{
					if (rbuf[i] == '\n' && rbuf[i - 1] == '\r')
					{
						rbuf[i - 1] = '\0';
						break;
					}
				}
				char* broadcaster = inet_ntoa(clientaddr.sin_addr);
				int serverportno = ntohs(clientaddr.sin_port);
                process_message(broadcaster, serverportno, rbuf);
			}
		}
	}
    return 0;
}

char szip_address[1024];
char szBuffer[1024];
int option = 0;
float probability = 0.;
float probability_bk = 0.;
DWORD Evt_bk = 0, X_bk = 0, Y_bk = 0;
DWORD Evt = 0, X = 0, Y = 0;

enum broadcast_type {
    BROADCASTER = 1,
    COORDINATE = 2,
    SCREEN_CONFIG = 3,
    PROBABILITY = 4
};
BOOL facing_me()
{
    int i = 0;
    std::vector<evt_func_data>::iterator broadcaster_it;
    for (std::vector<evt_func_data>::iterator it = v_evt_func_data.begin(); it != v_evt_func_data.end(); ++it)
    {
        if (probability < (*it).p)
            return FALSE;
    }
    return TRUE;

}
DWORD WINAPI send_broadcast_thread(void* param)//int port)
{
    IPv4 myIP;
    int portno = nPORT;// port;

    struct hostent* host = gethostbyname(szip_address);
    if (host == NULL)
    {
#ifdef WIN32
        WSACleanup();
#endif
        return false;
    }
    //Obtain the computer's IP
    myIP.b1 = ((struct in_addr*)(host->h_addr))->S_un.S_un_b.s_b1;
    myIP.b2 = ((struct in_addr*)(host->h_addr))->S_un.S_un_b.s_b2;
    myIP.b3 = ((struct in_addr*)(host->h_addr))->S_un.S_un_b.s_b3;
    myIP.b4 = ((struct in_addr*)(host->h_addr))->S_un.S_un_b.s_b4;
    

    DWORD dwWidth = GetSystemMetrics(SM_CXSCREEN);
    DWORD dwHeight = GetSystemMetrics(SM_CYSCREEN);

    SOCKET s = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == -1)
    {
        std::cout << "Error in creating socket";
        return 0;
    }
    char opt = 1;
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char*)&opt, sizeof(char));
    SOCKADDR_IN brdcastaddr;
    memset(&brdcastaddr, 0, sizeof(brdcastaddr));
    brdcastaddr.sin_family = AF_INET;
    brdcastaddr.sin_port = htons(portno);
    brdcastaddr.sin_addr.s_addr = INADDR_BROADCAST;
    int len = sizeof(brdcastaddr);
 
    clock_t begin_BROADCASTER = clock();
    clock_t end_BROADCASTER = clock();
    clock_t begin_SCREEN_CONFIG = clock();
    clock_t end_SCREEN_CONFIG = clock();
    while (1)
    {
        szBuffer[0] = '\0';
        double time_spent = (double)(end_BROADCASTER - begin_BROADCASTER) / CLOCKS_PER_SEC;
        if (time_spent > 5.)
        {
            snprintf(szBuffer, sizeof(szBuffer), "B%d.%d.%d.%d\r\n", myIP.b1, myIP.b2, myIP.b3, myIP.b4);
            int ret = sendto(s, szBuffer, strlen(szBuffer), 0, (sockaddr*)&brdcastaddr, len);
            if (ret < 0)
            {
                std::cout << "Error broadcasting to the clients";
            }
            else if (ret < (int)strlen(szBuffer))
            {
                std::cout << "Not all data broadcasted to the clients";
            }
            else
            {
                std::cout << "Broadcasting is done";
            }
            begin_BROADCASTER = clock();
        }
        else
        {
            end_BROADCASTER = clock();
        }
        szBuffer[0] = '\0';
        if (time_spent > 5.)
        {
            snprintf(szBuffer, sizeof(szBuffer), "S%d,%d\r\n", dwWidth, dwHeight);
            int ret = sendto(s, szBuffer, strlen(szBuffer), 0, (sockaddr*)&brdcastaddr, len);
            if (ret < 0)
            {
                std::cout << "Error broadcasting to the clients";
            }
            else if (ret < (int)strlen(szBuffer))
            {
                std::cout << "Not all data broadcasted to the clients";
            }
            else
            {
                std::cout << "Broadcasting is done";
            }
            begin_SCREEN_CONFIG = clock();
        }
        else
        {
            end_SCREEN_CONFIG = clock();
        }
        szBuffer[0] = '\0';
        if (i_am_manager)
        {
            if (Evt_bk != Evt || X_bk != X || Y_bk != Y)
            {
                Evt_bk = Evt;
                X_bk = X;
                Y_bk = Y;
                snprintf(szBuffer, sizeof(szBuffer), "C%d,%d,%d\r\n", Evt, X, Y);
                int ret = sendto(s, szBuffer, strlen(szBuffer), 0, (sockaddr*)&brdcastaddr, len);
                if (ret < 0)
                {
                    std::cout << "Error broadcasting to the clients";
                }
                else if (ret < (int)strlen(szBuffer))
                {
                    std::cout << "Not all data broadcasted to the clients";
                }
                else
                {
                    std::cout << "Broadcasting is done";
                }
            }
        }
        szBuffer[0] = '\0';
        if(probability_bk != probability)
        {
            probability_bk = probability;
            snprintf(szBuffer, sizeof(szBuffer), "P%0.3f\r\n", probability);
            int ret = sendto(s, szBuffer, strlen(szBuffer), 0, (sockaddr*)&brdcastaddr, len);
            if (ret < 0)
            {
                std::cout << "Error broadcasting to the clients";
            }
            else if (ret < (int)strlen(szBuffer))
            {
                std::cout << "Not all data broadcasted to the clients";
            }
            else
            {
                std::cout << "Broadcasting is done";
            }
        }
    }

    ::closesocket(s);
    return 0;
}

HANDLE _get_server_handle()
{
    LPTSTR lpszPipename = (LPTSTR)TEXT("\\\\.\\pipe\\ipc");
    // Error handling omitted for security descriptor creation.
    SECURITY_DESCRIPTOR sd;
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, static_cast<PACL>(0), FALSE);

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;

    // Create a bi-directional message pipe.
    HANDLE handle = CreateNamedPipe(lpszPipename,
        PIPE_ACCESS_DUPLEX,
        //PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,//,PIPE_UNLIMITED_INSTANCES
        4096,
        4096,
        NMPWAIT_USE_DEFAULT_WAIT,
        &sa);

    if (INVALID_HANDLE_VALUE == handle)
    {
        const DWORD last_error = GetLastError();
        printf("Failed to create named pipe handle: last_error=", last_error);
    }

    return handle;
}

DWORD WINAPI recv_prob_IPC_thread(void* param)
{
    HANDLE hPipe;
    const char* lpvMessage = "Ready";
    wchar_t  chBuf[32];
    BOOL   fSuccess = FALSE;
    DWORD  cbRead, cbToWrite, cbWritten, dwMode;
    HANDLE handle = (HANDLE)param;
    LPTSTR lpszPipename = (LPTSTR)TEXT("\\\\.\\pipe\\ipc");
    BOOL result = ConnectNamedPipe(handle, 0);
    DWORD last_error = GetLastError();
    memset(chBuf, 0, sizeof(chBuf));
    //if (result == 0 && (last_error == ERROR_PIPE_LISTENING || last_error == ERROR_PIPE_CONNECTED))
    //    ;
    //else
    //{
    //    printf("Failed to connect to named pipe handle: last_error=", last_error);
    //    CloseHandle(handle);
    //    return 0;
    //}
    if (result == FALSE) return 0;
    hPipe = handle;
    //while (1)
    //{
    //    hPipe = CreateFile(
    //        lpszPipename,   // pipe name 
    //        GENERIC_READ |  // read and write access 
    //        GENERIC_WRITE,
    //        0,              // no sharing 
    //        NULL,           // default security attributes
    //        OPEN_EXISTING,  // opens existing pipe 
    //        0,              // default attributes 
    //        NULL);          // no template file 
    //    if (hPipe != INVALID_HANDLE_VALUE)
    //        break;
    //    DWORD e = GetLastError();
    //    if (e != ERROR_PIPE_BUSY)
    //    {
    //        _tprintf(TEXT("Could not open pipe. GLE=%d\n"), e);
    //        return -1;
    //    }
    //    if (!WaitNamedPipe(lpszPipename, 20000))
    //    {
    //        printf("Could not open pipe: 20 second wait timed out.");
    //        return -1;
    //    }
    //}
    //dwMode = PIPE_READMODE_MESSAGE;
    //fSuccess = SetNamedPipeHandleState(
    //    hPipe,    // pipe handle 
    //    &dwMode,  // new pipe mode 
    //    NULL,     // don't set maximum bytes 
    //    NULL);    // don't set maximum time 
    //if (!fSuccess)
    //{
    //    _tprintf(TEXT("SetNamedPipeHandleState failed. GLE=%d\n"), GetLastError());
    //    return -1;
    //}
    //cbToWrite = (strlen(lpvMessage) + 1);
    //printf("Sending %d byte message: \"%s\"\n", cbToWrite, lpvMessage);
    //fSuccess = WriteFile(
    //    hPipe,                  // pipe handle 
    //    lpvMessage,             // message 
    //    cbToWrite,              // message length 
    //    &cbWritten,             // bytes written 
    //    NULL);                  // not overlapped 
    //if (!fSuccess)
    //{
    //    _tprintf(TEXT("WriteFile to pipe failed. GLE=%d\n"), GetLastError());
    //    return -1;
    //}
    //printf("\nMessage sent to server, receiving reply as follows:\n");
    while (1)
    {
        do
        {
            fSuccess = ReadFile(
                hPipe,    // pipe handle 
                chBuf,    // buffer to receive reply 
                sizeof(chBuf) - 1,  // size of buffer 
                &cbRead,  // number of bytes read 
                NULL);    // not overlapped 
            last_error = GetLastError();
            if (fSuccess == TRUE)// && last_error != ERROR_MORE_DATA)
            {
                //else if (last_error != ERROR_PIPE_LISTENING)
               //     break;
                chBuf[cbRead] = '\0';
                _tprintf(TEXT("\"%s\"\n"), chBuf);
                probability = _wtof(chBuf);
                break;

            }
            else
            {
                if (last_error != ERROR_MORE_DATA)
                    break;
            }

        } while (!fSuccess);  // repeat loop if ERROR_MORE_DATA 
        if (fSuccess)
        {
            last_error = GetLastError();
            _tprintf(TEXT("ReadFile %s GLE=%d %d\n"), chBuf, last_error, fSuccess);
            //return -1;
        }
        Sleep(1000);
    }
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);
    return 0;
}
int run_networkers()//int port)
{
    USES_CONVERSION;
#ifdef WIN32
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 0);
    if (::WSAStartup(wVersionRequested, &wsaData) != 0)
        return false;
#endif
    if (gethostname(szip_address, sizeof(szip_address)) == SOCKET_ERROR)
    {
#ifdef WIN32
        WSACleanup();
#endif
        return false;
    }
    HANDLE threadSender = CreateThread(NULL, 0, send_broadcast_thread, NULL, 0, NULL);
    HANDLE threadReceiver = CreateThread(NULL, 0, recv_broadcast_thread, NULL, 0, NULL);
    HANDLE handle = _get_server_handle();

    if (INVALID_HANDLE_VALUE != handle)
    {
        //BOOL result = ConnectNamedPipe(handle, 0);
        //const DWORD last_error = GetLastError();
        //if (result == 0 && (last_error == ERROR_PIPE_LISTENING || last_error == ERROR_PIPE_CONNECTED))
        //{
            HANDLE threadIPC = CreateThread(NULL, 0, recv_prob_IPC_thread, handle, 0, NULL);
        //}
        //else
        //{
        //    printf("Failed to connect to named pipe handle: last_error=", last_error);
        //    CloseHandle(handle);
        //}
    }
    
    return true;
}