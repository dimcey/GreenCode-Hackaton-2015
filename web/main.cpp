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
#define INPUT_BUFFER_SIZE 4000
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
int millisecondsToWait = 10000;
int result;
int sizeofPollfd = sizeof(pollfd);
int sizeofHints = sizeof(hints);
int sizeofSockaddr = sizeof(sockaddr);

#define PRINT_NUMBERS true

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

/// Attempts to bind host socket.
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

/// Clear log file on server-side.
void ClearLogFile()
{
	std::cout<<"\nClearing log file...";
	std::fstream file;
	file.open("/opt/gclc/gclc.log", std::ios_base::out);
	if (file.is_open())
	{
		file.close();
		std::cout<<"done";
	}
	else
		std::cout<<"failed";
}

/// Looks for and replaces all, returns new length 
int ReplaceStringKeys(char * forString, const int length)
{
	int newLength = length;
//	std::cout<<"\nreplace length : "<<length;
	char buf[5];
	memset(buf, '\0', 4); //ptr, val, num
	buf[0] = '#'; // First #
	buf[1] = '0';

	for (int i = 0; i < newLength; ++i)
	{
		if (i < length -1 && ((unsigned char)forString[i]) < 32 && forString[i] >= 0)
		{
			int val = forString[i];
			/// dst, src, length
			memmove(forString + i + 3, forString + i, length);
			sprintf(buf+2, "%o", val); // target, "format", data
			int intLen = strlen(buf);	
//			std::cout<<"\nbuf: "<<buf;
			memmove(forString + i, buf, intLen);
			newLength += 3;
		}
		/// Convert our magic character 141 to \n to separate the log messages approriately. 
		if (((unsigned char)forString[i]) == 141)
		{
			forString[i] = '\n';
		}
	}
	return newLength;
}

long lastReceiveTime = 0;

int main (int argc, char ** argv )
{
	/// first time inits.
    pollfds.events = POLLIN;
	
	
	int udpPort = SERVER_PORT_3333;

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
	
	bool waitingForLastMessage = false;
	int receivedMessages = 0;
	/// If true, will try and batch all messages to send.
	bool batchMessages = true;

//	std::cout<<"\nStart of loop.";
	Sleep(500);
	/// Format with time-stamp 
	std::string dateStr;
	while(true)
	{
	//	std::cout<<"\nReady to read?";
		// Read
		timeval t = {0, 1000};
		
		// Check time since last message.
		if (waitingForLastMessage)
		{
			long timeDiff = TimeMs() - firstReceiveTime;
			/// If sufficient time has passed since first message (4 minutes), start the long sleep (9 minutes).
//			std::cout<<"\nTimeMs(): "<<TimeMs()<<" firstReceiveTime: "<<firstReceiveTime<<" diff: "<<TimeMs() - firstReceiveTime;
			if (timeDiff > 300000) 
			{
				waitingForLastMessage = false;
				/// Send acc later
				std::cout<<"\nBeginning long sleep for 480 seconds...\n";
				ReadyToRead(sockfd, 480000);
				receivedMessages = 0;
			}
		}
		/// If final message.. send it?
		if (!ReadyToRead(sockfd))
		{
			continue;
		}

//		std::cout<<"\nBytes to read";
		int flags = 0;
//		std::cout<<"\nSize of sock Addr: "<<sizeofSockaddr<<" vs "<<sizeof(sockaddr);
		int bytesRead = recvfrom(sockfd, inputBuffer, INPUT_BUFFER_SIZE-1, flags, &fromAddress, (socklen_t*) &sizeofSockaddr);
		inputBuffer[bytesRead] = '\0'; // Add null-char so it is printable.
//		std::cout<<"\nReceived: "<<inputBuffer;
		if (bytesRead == SOCKET_ERROR)
		{
			int error = GetLastError();
			std::cout<<"\nError reading socket from given address. ErrorCode: "<<error;
			continue;
		}
		lastReceiveTime = TimeMs();
		// Flag as waiting for last message.
		waitingForLastMessage = true;
		/// First message, initialize stuff.
		if (receivedMessages == 0)
		{
			firstReceiveTime = TimeMs();
			ClearLogFile();
		}
		++receivedMessages;

		/// SERVER
		/// Replace #000 numbers with chars
		int newLength = ReplaceStringKeys(inputBuffer, bytesRead);
		std::fstream file;
		/// append for now
		file.open("/opt/gclc/gclc.log", std::ios_base::out | std::ios_base::app);
		if (file.is_open())
		{
			file.write((char*) inputBuffer, newLength);
			file.close();
		}
		else {
			std::cout<<"unable to open file D:";
		}		
		std::cout<<"|";
	}

	/// Free up resources (if we ever exit...)
	freeaddrinfo(servInfo);

	return 0;
}
