#ifndef  SOCKETSTREAM_H
#define  SOCKETSTREAM_H

#include    "package.h"
#ifdef WIN32
#	include <windows.h>
#else
#	include <unistd.h>
#	include	<errno.h>
#endif

#include    "error.h"
#include  <string>
CW_BEGIN

const int SSMAXLEN=512;
class SocketStream{
public:
	SocketStream(int sk):handle(sk)
	{
		uncnt=0;
		ssbufp=ssbuf;
		memset(ssbuf,0,sizeof(ssbuf));
	}
	~SocketStream(){}
	int Receive(char* outBuf, size_t recvLen, int flags=0);
	/* read and write without buffer */
	int readn(char* usrbuf,int n);
	int writen(const char* usrbuf,int n);
	/* read with buffer */
	int readb(char* usrbuf,int n);
	int readlineb(char* usrbuf,int maxlen);
	void close();
private:
	int handle;
	int uncnt;//unread bytes
	char* ssbufp;
	char ssbuf[SSMAXLEN];
};

CW_END



#endif  /*SOCKETSTREAM_H*/
