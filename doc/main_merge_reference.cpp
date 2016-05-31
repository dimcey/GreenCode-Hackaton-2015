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
#define INPUT_BUFFER_SIZE 2000
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
int millisecondsToWait = 2000;
int result;
int sizeofPollfd = sizeof(pollfd);
int sizeofHints = sizeof(hints);
int sizeofSockaddr = sizeof(sockaddr);

#define PRINT_NUMBERS true

/// Returns state of a socket, if it is ready to be read or not!
bool ReadyToRead(SOCKET sock)
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

/// Attempts to connect to target host.
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
	for(server = servInfo; server != NULL; server = server->ai_next){
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
#define TOTAL_INPUT_BUFFER_SIZE 3000 
#define SEND_THRESHOLD 2000
char totalInputBuffer[TOTAL_INPUT_BUFFER_SIZE];
int totalReceivedBytes = 0;

int SendBatchToServer()
{
//	std::cout<<"Sending to server";
	int result = SendToServer(totalInputBuffer, totalReceivedBytes);
	totalReceivedBytes = 0;
}


/*
void TestThePi()
{
	// SPAM ItT
	while (true)
	{
		Sleep(1000);
		const int maxFromBuffer = 512;
		char fromBuffer[maxFromBuffer];
		strcpy(fromBuffer, "Hello, world!");
		int maxBytesToWrite = 24;
		/// Check that the socket is writable
		timeval t = {0, 1000};
		if (!ReadyToWrite(sockfd))
	        continue;

		int flags = 0;

		// Set up some settings on the connection
		memset(&hints, 0, sizeofHints);		// Make sure the struct is empty
		hints.ai_family		= AF_INET;	// Use IPv4 or IPv6
		hints.ai_socktype	= SOCK_DGRAM;	// Use selected type of socket
		hints.ai_flags		= AI_PASSIVE;		// Fill in my IP for me
		hints.ai_protocol	= IPPROTO_UDP;			// Select correct Protocol for the connection

		const char * ip = "79.98.21.68";
		char portStr[5] = "514";
//		bool ok = ToString(514, portStr);
	//	assert(ok);
//		std::cout<<"\nPortStr: "<<portStr;

		int result = getaddrinfo(ip, portStr, &hints, &servInfo);
	    if(result != 0)
		{
			std::cout<<"\nGetaddrinfo failed, "<<GetLastError();
			freeaddrinfo(servInfo);
			continue;
		}

	    /// Try start the server
		bool boundSuccessfully = false;
		int bytesWritten = 0;
		for(server = servInfo; server != NULL; server = server->ai_next){
			// Get good address.
			bytesWritten += sendto(sockfd, fromBuffer, maxBytesToWrite, flags, server->ai_addr, server->ai_addrlen);
			if (bytesWritten == -1)
			{
				// Add extra handling maybe.
				std::cout<<"Blugg";
			}
			if(bytesWritten)
			{
				std::cout<<"\nWrote successfully";
				boundSuccessfully = true;
				break;	// Success!
			}
		}
		freeaddrinfo(servInfo);
	}
}
*/

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

//End Edit

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
//	std::cout<<"\nFinal str: "<<forString;
	return newLength;

	/* /// OLD code replacing #000 with \n etc., but we needed the reverse! D:
	int newLength = length;
	std::cout<<"\nreplace length : "<<length;
	for (int i = 0; i < length - 1; ++i)
	{
		if (forString[i] == '#' && forString[i+1] == '0')
		{
			// Fix it!
			int num = atoi(forString + i + 1);
			std::cout<<"\nNum: "<<num;
			/// Convert from octal to decimal.
			int decNum = num % 10 + (num / 10 * 8);
			*(forString + i) = (char) decNum;
			int remainingLength = length - i - 2;
			/// Move end from #000badabada to 
			memmove(forString + i + 1, forString + i + 4, remainingLength);
			newLength -= 3;
		}
	}
	return newLength;
	*/
}

long lastReceiveTime = 0;

int main (int argc, char ** argv )
{
	/// first time inits.
    pollfds.events = POLLIN;
    serverAddrInfoGotten = false;
	
/*
	// Test the replacer
	char * b = new char[500];
	strcpy(b, "Hello world!\n\t\nThinker toys.");
	unsigned char one41 = 141;
	char * buf = new char[4]; 
	buf[0] = (unsigned char) 141;
	buf[1] = '\0';
	strcat(b, buf);
	strcat(b, "Hellow world");
	strcat(b, buf);
	b[14] = 141;
	int len = strlen(b);
	std::cout<<"\n"<<b;
	ReplaceStringKeys(b, len);
	std::cout<<"\n";
	return 0;
*/
	

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
 
	 /// After opening port, try send to pi if specified
	/*
	if (argc > 1 && strstr(argv[1], "testpi"))
	{
		TestThePi();
	}*/

	/// Edited
	/// Set on receiving first message of the batch. Compared to when determining when to start the long sleep.
	long firstReceiveTime = TimeMs();
	
	/// 

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
			if (isPi && batchMessages && timeDiff > 240000)
			{
				if (totalReceivedBytes > 0)
					SendBatchToServer();
			}
			/// If sufficient time has passed since first message (4 minutes), start the long sleep (9 minutes).
//			std::cout<<"\nTimeMs(): "<<TimeMs()<<" firstReceiveTime: "<<firstReceiveTime<<" diff: "<<TimeMs() - firstReceiveTime;
			if (timeDiff > 240000) 
			{
				waitingForLastMessage = false;
				/// Send acc later
				std::cout<<"\nBeginning long sleep for 540 seconds...\n";
		//		std::cout<<"\nBeginning long sleep for 540 seconds...\n";
		//		std::cout<<"\nBeginning long sleep for 540 seconds...\n";
		//		std::cout<<"\nBeginning long sleep for 540 seconds...\n";
				Sleep(540000);
				receivedMessages = 0;
			}
		}
		/// If final message.. send it?
		if (!ReadyToRead(sockfd))
		{
			if (isPi)
				;//Sleep(80);
			else
				Sleep(2000);
			continue;
		}
		if (isPi) 
			dateStr = currentDateTime();

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
//		std::cout<<"|";
//<<inputBuffer
		// Flag as waiting for last message.
		waitingForLastMessage = true;
		/// First message, initialize stuff.
		if (receivedMessages == 0)
		{
			firstReceiveTime = TimeMs();
			if (!isPi) // Clear log file if on server and first message.
				ClearLogFile();
		}
		++receivedMessages;


		if (isPi)
		{
	//		std::cout<<"\nThis is the PI,";
			/// Move the body of the text.
			int dateStrLen = dateStr.length();

			int fullString = bytesRead + dateStrLen;
			/// Send to the server straight away, first test.
			if (batchMessages)
			{
			//	memmove(inputBuffer + dateStrLen, inputBuffer, bytesRead);
		//		memcpy(inputBuffer, dateStr.c_str(), dateStrLen);
	///			std::cout<<"\nFull string to send: "<<inputBuffer;

				/// Append into total input.
				memcpy(totalInputBuffer + totalReceivedBytes, dateStr.c_str(), dateStrLen);
				totalReceivedBytes += dateStrLen;
				memcpy(totalInputBuffer + totalReceivedBytes, inputBuffer, fullString);
				totalReceivedBytes += bytesRead;
			//	memcpy(total)

//				std::cout<<"\nBatched";
			//	memcpy(totalInputBuffer + totalReceivedBytes, inputBuffer, fullString);


//				totalReceivedBytes += fullString;
				/// Separate messages with \n
				/// Magic separator 141 between each line. 
				totalInputBuffer[totalReceivedBytes - 1] = 141;
//				++totalReceivedBytes;

				if (totalReceivedBytes > SEND_THRESHOLD)
					SendBatchToServer();
			}				
			else 
			{
				memmove(inputBuffer + dateStrLen, inputBuffer, bytesRead);
				memcpy(inputBuffer, dateStr.c_str(), dateStrLen);

///			std::cout<<"\nFull string to send: "<<inputBuffer;
				SendToServer(inputBuffer, fullString);
				std::cout<<" sending to server";
			}
		}
		/// SERVER
		else 
		{
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
			// receivedMessages
		}
//		Sleep(50);
	}

	/// Free up resources (if we ever exit...)
	freeaddrinfo(servInfo);

	return 0;
}

