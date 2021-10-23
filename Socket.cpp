#include    "Socket.h"
#include  <stdio.h>

CW_BEGIN

// windows socket init

#ifdef WIN32
static class sock_init{
public:
	sock_init();
	~sock_init();
}_init;

sock_init::sock_init()
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD(2,0);

	err=WSAStartup(wVersionRequested,&wsaData);
	printf("sock_init in...");
	if(err) throw Error("Winsock init error");

}

sock_init::~sock_init()
{
	WSACleanup();
	printf("sock_init release in...");
}
#endif


/* create an unconnected socket,or reference to sk */
Socket::Socket(SOCKET sk)
{
	m_socket=sk;
	socketClosed=false;
}
/* Socket constructor,create a new socket */
Socket::Socket(int af,int type,int protocol)
{
	create(af,type,protocol);
}
Socket::~Socket()
{
	/* close the socket if it was not closed yet */
	if(!socketClosed) close();
}
/* create a socket */
void Socket::create(int af,int type,int protocol)
{
	m_socket=socket(af,type,protocol);
	if(m_socket<0)//socket create error
	{
		socketClosed=true;
		throw Error("Socket Create Error!");
	}
	else socketClosed=false;//make the socket mark as unclosed

}
void Socket::setRcvBuf(){
	int bufLen;
	int optLen = sizeof(bufLen);
	if (getsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (char*)&bufLen, &optLen) == SOCKET_ERROR) 
	{
		throw Error("Socket Create Error!");
	} else {
		printf("setopt before setRcvBuf bufLen:%d.", bufLen);
		
		//bufLen = 1024*1024;
		bufLen = 65535;
		if(setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (char*)&bufLen, optLen) == SOCKET_ERROR)
		{
			throw Error("Socket Create Error!");
		} else {
			if (getsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (char*)&bufLen, &optLen) == SOCKET_ERROR) 
			{
				throw Error("Socket Create Error!");
			} else {
				printf("setopt after setRcvBuf bufLen:%d.", bufLen);
			}
		}
		
	}
}
void Socket::setKeepAlive()
{
	bool iKeepAlive = false;
	int iOptLen = sizeof(iKeepAlive);
	if (getsockopt(m_socket, SOL_SOCKET, SO_KEEPALIVE, (char*)&iKeepAlive, &iOptLen) == SOCKET_ERROR) 
	{
		throw Error("Socket Create Error!");
	} else 
	{
		printf("setopt before iKeepAlive:%d.", iKeepAlive);
		iKeepAlive = true;
		if(setsockopt(m_socket, SOL_SOCKET, SO_KEEPALIVE, (char*)&iKeepAlive, iOptLen) == SOCKET_ERROR)
		{
			throw Error("Socket Create Error!");
		} else 
		{
			iKeepAlive = false;
			if (getsockopt(m_socket, SOL_SOCKET, SO_KEEPALIVE, (char*)&iKeepAlive, &iOptLen) == SOCKET_ERROR) 
			{
				throw Error("Socket Create Error!");
			} else if (iKeepAlive) 
			{
				printf("setopt after iKeepAlive:%d.", iKeepAlive);
				TCP_KEEPALIVE inKeepAlive = {0};
				unsigned long ulInLen = sizeof(TCP_KEEPALIVE);
				TCP_KEEPALIVE outKeepAlive = {0};
				unsigned long ulOutLen = sizeof(TCP_KEEPALIVE);
				unsigned long ulBytesReturn = 0;

				inKeepAlive.onoff = 1;
				inKeepAlive.keepaliveinterval = 15000;
				inKeepAlive.keepalivetime = 3;
				/*
				if(WSAIoctl(m_socket, SIO_KEEPALIVE_VALS,(LPVOID)&inKeepAlive, ulInLen, (LPVOID)&outKeepAlive, ulOutLen, &ulBytesReturn, NULL, NULL) == SOCKET_ERROR)
				{
					throw Error("Socket Create Error!");

				}
				*/
			}
			
		}
	}
	
}
/* connect to ipAddr:port */
void Socket::connect(const string& ipAddr,unsigned short port)
{
	struct sockaddr_in servAddr;
	memset(&servAddr,0,sizeof(servAddr));
	servAddr.sin_family=AF_INET;
	servAddr.sin_port=htons(port);
	if(inet_pton(AF_INET,ipAddr.c_str(),&servAddr.sin_addr)<=0)
		throw Error("inet_pton error for "+ipAddr);
	if(::connect(m_socket,(struct sockaddr*)&servAddr,sizeof(servAddr))<0)
		throw Error("connect error");
}
/* bind to a port,server action */
void Socket::bind(unsigned short port)
{
	struct sockaddr_in servAddr;
	memset(&servAddr,0,sizeof(servAddr));
	servAddr.sin_family=AF_INET;
	servAddr.sin_addr.s_addr=INADDR_ANY;
	servAddr.sin_port=htons(port);
	setRcvBuf();
	setKeepAlive();
	/* eliminates "Addr already in use" error from bind */
	int optval=1;
	if(setsockopt(m_socket,SOL_SOCKET,SO_REUSEADDR,
				(optval_t)&optval,sizeof(int))<0)
		throw Error("setsockopt error");
	if(::bind(m_socket,(struct sockaddr*)&servAddr,sizeof(servAddr))<0)
		throw Error("bind error,usually permission denied");
	
}
/* make it a listening socket ready to accept connection requests */
void Socket::listen(int backlog)
{
	if(::listen(m_socket,backlog)<0)
		throw Error("socket listen error");
}
/* accept a connection request */
//Socket Socket::accept(struct sockaddr_in* cliAddr)
void Socket::accept(Socket& SK,struct sockaddr_in* cliAddr)
{
	SOCKET sk=-1;
	if(cliAddr==0)//don't care the request socket address
	{
		struct sockaddr_in cliaddr;
		memset(&cliaddr,0,sizeof(cliaddr));
		socklen_t socklen=sizeof(cliaddr);
		if((sk=::accept(m_socket,(struct sockaddr*)&cliaddr,&socklen))<0)
			throw Error("socket accept error");
		char* c_address = new char[15]{0};
		sprintf_s(c_address, 14, "%d.%d.%d.%d",cliaddr.sin_addr.S_un.S_un_b.s_b1,cliaddr.sin_addr.S_un.S_un_b.s_b2,cliaddr.sin_addr.S_un.S_un_b.s_b3,cliaddr.sin_addr.S_un.S_un_b.s_b4);
		Port = cliaddr.sin_port;
		Address = c_address;
		delete[] c_address;
	}
	else
	{
		socklen_t socklen=sizeof(*cliAddr);
		if((sk=::accept(m_socket,(struct sockaddr*)cliAddr,&socklen))<0)
			throw Error("socket accept error");
	}
	SK.m_socket=sk;

}
SOCKET Socket::Accept()
{
	struct sockaddr_in cliaddr;
	memset(&cliaddr,0,sizeof(cliaddr));
	socklen_t socklen=sizeof(cliaddr);
	SOCKET sk =::accept(m_socket,(struct sockaddr*)&cliaddr,&socklen);
	if(sk < 0)
	{
		throw Error("socket accept error");
	}
	char* c_address = new char[15]{0};
	sprintf_s(c_address, 14, "%d.%d.%d.%d",cliaddr.sin_addr.S_un.S_un_b.s_b1,cliaddr.sin_addr.S_un.S_un_b.s_b2,cliaddr.sin_addr.S_un.S_un_b.s_b3,cliaddr.sin_addr.S_un.S_un_b.s_b4);
	Port = cliaddr.sin_port;
	Address = c_address;
	delete[] c_address;
	return sk;
}
/* close socket */
void Socket::close()
{
	//if(socketClosed) return;
#ifdef WIN32
	printf("server Socket::close try in...");
	closesocket(m_socket);
    // if(shutdown(m_socket, SD_BOTH) == SOCKET_ERROR){
	// }
	//if(closesocket(m_socket));// throw Error("close socket error");
	// else socketClosed=true;
	
#else
	::close(m_socket);
	// if(::close(m_socket)<0); //throw Error("close socket error");
	// else socketClosed=true;
#endif
}

SocketStream Socket::getSocketStream()
{
	return SocketStream(m_socket);
}
CW_END
