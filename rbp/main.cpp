/// Emil Hedemalm
/// 2015-12-02
/// 

// #include <cassert>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sys/time.h>
#include <time.h>

#include <errno.h>
// For Sleeping
#include <unistd.h>
void Sleep(int milliseconds)
{
	usleep(milliseconds*1000);
}

// Static Parsing functions
#include <cstdio>
#include <cstring>
bool ToString(const int value, char * dstBuffer)
{
	char buf[50];
    int success;
	success = snprintf(buf, sizeof(buf), "%d", value);
    if (!success)
    	return false;
    strncpy(dstBuffer, buf, 50);
    return true;
}

/// Socket
#define INPUT_BUFFER_SIZE 1000
char inputBuffer[INPUT_BUFFER_SIZE];

/// Network includes and stuff for linux
#define GetLastError() errno
#define closesocket(b)  close(b)

/// To set non-blocking for sockets. http://stackoverflow.com/questions/1543466/how-do-i-change-a-tcp-socket-to-be-non-blocking
#include <fcntl.h>
/// Used for big byte transfer operations like memcpy
#include <cstring>

/// for poll instead of select: http://pic.dhe.ibm.com/infocenter/iseries/v6r1m0/index.jsp?topic=/rzab6/poll.htm
#include <poll.h>

#include <sys/time.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>      // sockaddr_in
#include <netinet/tcp.h>    // TCP_NODELAY
#include <time.h>


#if PLATFORM == PLATFORM_MAC
	#define SOL_IP IPPROTO_IP
#endif
#ifndef SO_DONTLINGER
    #define SO_DONTLINGER   (~SO_LINGER)  /* Older SunOS compat. hack */
#endif
#ifndef INVALID_SOCKET
	#define INVALID_SOCKET -1
#endif
#ifndef SOCKET_ERROR
    #define SOCKET_ERROR -1
#endif
#define sockerrno errno
#define SOCKET int


/// Main socket
SOCKET sockfd;
/// Info about server to search for, found servers and selected server
struct addrinfo hints, *servInfo, *server;
sockaddr fromAddress;


#define SERVER_PORT_3333 3333

/// Closes target socket.
void CloseSocket(SOCKET sock)
{
	if(sock != -1 && sock)
		close(sock);
	sock = NULL;
}



struct pollfd pollfds;
int millisecondsToWait = 20000;
int result;
int sizeofPollfd = sizeof(pollfd);
int sizeofHints = sizeof(hints);
int sizeofSockaddr = sizeof(sockaddr);

#define PRINT_NUMBERS true

/// Returns state of a socket, if it is ready to be read or not!
bool ReadyToRead(SOCKET sock, int millisToPoll = millisecondsToWait)
{
//	std::cout<<"\nReady to read? ";//<<sizeofPollfd<<" vs "<<sizeof(pollfds);
    memset(&pollfds, 0, sizeofPollfd);
    pollfds.fd = sock;
    pollfds.events = POLLIN;
    result = poll(&pollfds, 1, millisecondsToWait);
	/// Error
	if (result == SOCKET_ERROR){
        return false;
	}
	/// None ready
	else if (result == 0)
		return false;
    return true;
}

/// Returns state of a socket, if we can write to it.
bool ReadyToWrite(SOCKET sock)
{
    memset(&pollfds, 0, sizeofPollfd);
    pollfds.fd = sock;
    pollfds.events = POLLOUT;
    result = poll(&pollfds, 1, millisecondsToWait);
	/// Error
	if (result == SOCKET_ERROR || result == 0)
	{
        std::cout<<"\nUnable to write to socket!";
        return false;
	}
	return true;
}

/// Attempts to bind socket for reading.
bool BindUdpSocket(int port)
{
    std::cout<<"\nBindingToSocket";
	/// SOCK_STREAM for TCP, SOCK_DGRAM for UDP
	int connectionType = SOCK_DGRAM;
	int protocol = IPPROTO_UDP;
	int error;

	char portStr[50];
	bool ok = ToString(port, portStr);
//	assert(ok);
	std::cout<<"\nPortStr: "<<portStr;

//	String portStr = String::ToString(port);
	bool connectedSuccessfully = false;

	// Set up some settings on the connection
	memset(&hints, 0, sizeofHints);		// Make sure the struct is empty
	hints.ai_family		= AF_INET;	// Use IPv4 or IPv6
	hints.ai_socktype	= connectionType;	// Use selected type of socket
	hints.ai_flags		= AI_PASSIVE;		// Fill in my IP for me
	hints.ai_protocol	= protocol;			// Select correct Protocol for the connection
	std::cout<<"\ngetaddrinfo...";
	int result = getaddrinfo(NULL, portStr, &hints, &servInfo);
    if(result != 0)
	{
		std::cout<<"\nGetaddrinfo failed, errorCode: " <<result;
		return false;
	}
	std::cout<<"done";
    /// Try start the server
	bool boundSuccessfully = false;
	std::cout<<"\nEntering loop to bind socket... ";
	for(server = servInfo; server != NULL; server = server->ai_next)
	{
		sockfd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
		if(sockfd == INVALID_SOCKET)
			continue;
		if(bind(sockfd, server->ai_addr, server->ai_addrlen) != SOCKET_ERROR)
		{
			std::cout<<"done";
			boundSuccessfully = true;
			break;	// Success!
		}
		CloseSocket(sockfd);
	}
	/// Error handling
	if (!boundSuccessfully)
	{
		std::cout<<"failed";
		error = errno;
		std::cout<<"\nCould not bind socket, error code "<<error;
		return false;
	}
	std::cout<<"\nSUCESSS";
	return true;
}

const char * serverIP = "51.255.62.60";
const char portStr[5] = "3333";

bool serverAddrInfoGotten = false;

/// Returns bytes sent
int SendToServer(char * fromBuffer, int maxBytesToWrite)
{
//	std::cout<<"\nPreparing to send to server...";
	/// Check that the socket is writable
//	timeval t = {0, 1000};
	if (!ReadyToWrite(sockfd))
        return 0;

	int flags = 0;

//	bool ok = ToString(SERVER_PORT_3333, portStr);
//	assert(ok);
//	std::cout<<"\nSending to "<<ip<<"@"<<portStr;
//	std::cout<<"\nPortStr: "<<portStr;

	if (!serverAddrInfoGotten)
	{
		// Set up some settings on the connection
		memset(&hints, 0, sizeofHints);		// Make sure the struct is empty
		hints.ai_family		= AF_INET;	// Use IPv4 or IPv6
		hints.ai_socktype	= SOCK_DGRAM;	// Use selected type of socket
		hints.ai_flags		= AI_PASSIVE;		// Fill in my IP for me
		hints.ai_protocol	= IPPROTO_UDP;			// Select correct Protocol for the connection

		int result = getaddrinfo(serverIP, portStr, &hints, &servInfo);
	    if(result != 0)
		{
			std::cout<<"\nGetaddrinfo failed, "<<GetLastError();
			freeaddrinfo(servInfo);
			return false;
		}
		else 
			serverAddrInfoGotten = true;
	}
    /// Try start the server
	bool boundSuccessfully = false;
	int bytesWritten = 0;
	for(server = servInfo; server != NULL; server = server->ai_next)
	{
		// Get good address.
		bytesWritten += sendto(sockfd, fromBuffer, maxBytesToWrite, flags, server->ai_addr, server->ai_addrlen);
		if (bytesWritten == -1)
		{
			// Add extra handling maybe.
			std::cout<<"Blugg error D:";
		}
		if(bytesWritten)
		{
//			std::cout<<"\nSent successfully";
			boundSuccessfully = true;
			break;	// Success!
		}
		else 
			std::cout<<"Unable to send on ...";
	}
	return bytesWritten;
}

/// 10k
#define TOTAL_INPUT_BUFFER_SIZE 2000 
#define SEND_THRESHOLD 1000
char totalInputBuffer[TOTAL_INPUT_BUFFER_SIZE];
int totalReceivedBytes = 0;

int SendBatchToServer()
{
//	std::cout<<"Sending to server";
	int result = SendToServer(totalInputBuffer, totalReceivedBytes);
	totalReceivedBytes = 0;
}

//Edited
time_t now;
struct tm  tstruct;
char       timeBuf[40];
// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const std::string currentDateTime() 
{
    now = time(0);
    tstruct = *localtime(&now);
    // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
    // for more information about date/time format
    strftime(timeBuf, sizeof(timeBuf), "%b %e %X ", &tstruct);
    return timeBuf;
}

// End Edit
long TimeMs()
{
    // New version: http://man7.org/linux/man-pages/man2/clock_gettime.2.html
    timespec time;//, resolution; // ,
//	clock_getres(CLOCK_REALTIME, &resolution);
    clock_gettime(CLOCK_REALTIME, &time);
    long timeMs = time.tv_sec * 1000;
//    timeMs *= 1000;
    timeMs += time.tv_nsec / 1000000;
	return timeMs;
} 

long lastReceiveTime = 0;

int main (int argc, char ** argv )
{
	/// first time inits.
    pollfds.events = POLLIN;
    serverAddrInfoGotten = false;
	
	int udpPort = SERVER_PORT_3333;
	int tcpPort = 4444;
	int counter = 0; 
	int MAX_COUNTER_SIZE=20;
	std::cout<<"\nArgc: "<<argc;
	bool isPi = false;

	/// Parse inputs.
	if (argc > 1)
	{
		// Parse ports
		char * portStr = argv[1];
		udpPort = atoi(portStr);
		std::cout<<"\nUsing udp port: "<<udpPort;
		if (udpPort == 514)
			isPi = true;
	}

	/// Open ports
	bool ok = BindUdpSocket(udpPort);
	if (!ok)
	{
		std::cout<<"\nUnable to bind UDP socket to port "<<udpPort<<" D:";
		return -1;
	}

	/// Edited
	/// Set on receiving first message of the batch. Compared to when determining when to start the long sleep.
	long firstReceiveTime = TimeMs();
	
	/// Var to store it and not call TimeMs twice.
	long nowMs = 0;
	bool waitingForLastMessage = false;
	int receivedMessages = 0;
	/// If true, will try and batch all messages to send.
	bool batchMessages = true;

	/// Format with time-stamp 
	std::string dateStr;
	while(true)
	{
		bool readyToRead = false;
		// Check time since last message.
		if (waitingForLastMessage)
		{
			long timeDiff = TimeMs() - firstReceiveTime;
			if (timeDiff > 240000)
			{
				if (totalReceivedBytes)
					SendBatchToServer();
			}			
			/// If sufficient time has passed since first message (4 minutes), start the long sleep (9 minutes).
			if (timeDiff > 300000)
			{

				waitingForLastMessage = false;
				/// Send acc later
//				std::cout<<"\nBeginning long sleep for 480 seconds...\n";
//				Sleep(480000);
				readyToRead = ReadyToRead(sockfd, 600000);
				receivedMessages = 0;
			}	
		}
		/// Poll socket.
		if (!readyToRead)
			readyToRead = ReadyToRead(sockfd);
		if (!readyToRead)
		{
			/// Until 
			continue;
		}
		dateStr = currentDateTime();

		int flags = 0;
		int bytesRead = recvfrom(sockfd, inputBuffer, INPUT_BUFFER_SIZE-1, flags, &fromAddress, (socklen_t*) &sizeofSockaddr);
		inputBuffer[bytesRead] = '\0'; // Add null-char so it is printable.
//		std::cout<<"\nReceived: "<<inputBuffer;
		if (bytesRead == SOCKET_ERROR)
		{
			int error = GetLastError();
			std::cout<<"\nError reading socket from given address. ErrorCode: "<<error;
			continue;
		}
		lastReceiveTime = nowMs = TimeMs();
		// Flag as waiting for last message.
		waitingForLastMessage = true;
		/// First message, initialize stuff.
		if (receivedMessages == 0)
		{
			firstReceiveTime = nowMs;
		}
		++receivedMessages;

		/// Move the body of the text.
		int dateStrLen = dateStr.length();
//		int fullString = bytesRead + dateStrLen;
		/// Send to the server straight away, first test.
//		if (batchMessages)
//		{
		/// Append into total input.
		memcpy(totalInputBuffer + totalReceivedBytes, dateStr.c_str(), dateStrLen);
		totalReceivedBytes += dateStrLen;
		memcpy(totalInputBuffer + totalReceivedBytes, inputBuffer, bytesRead);
		totalReceivedBytes += bytesRead;
		/// Magic separator 141 between each line. 
		totalInputBuffer[totalReceivedBytes - 1] = 141;

		if (totalReceivedBytes > SEND_THRESHOLD)
			SendBatchToServer();
//		}
		/*
		else 
		{
			memmove(inputBuffer + dateStrLen, inputBuffer, bytesRead);
			memcpy(inputBuffer, dateStr.c_str(), dateStrLen);

///			std::cout<<"\nFull string to send: "<<inputBuffer;
			SendToServer(inputBuffer, fullString);
			std::cout<<" sending to server";
		}*/
	}
	/// Free up resources (if we ever exit...)
	freeaddrinfo(servInfo);
	return 0;
}

