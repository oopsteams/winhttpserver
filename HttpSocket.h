#ifndef  HTTPSOCKET_H
#define  HTTPSOCKET_H

#include <map>
#include <regex>
#include    "Socket.h"
#include    "error.h"
#include "HttpBean.hpp"
#include "ThreadPool.hpp"
USING_CW

const int MAXLINE=512;
typedef std::function<void(const Request&, Response&)> HttpHandler;
/* HttpSocket handle the Http web,
 * maybe there should be a TcpSocket between
 * HttpSocket and Socket. But for now,just 
 * inheritance Socket.*/
class HttpSocket:public Socket{
public:
	size_t UpSize = 512 * 1024 * 1;
	size_t DownSize = 512 * 1024 * 1;
	String WebRoot;
	String dataDir;
	String logDir;
	String outDir;
	String dayDir;
	String outDayDir;
	void Get(const String&, const HttpHandler&);
	void Post(const String&, const HttpHandler&);
	HttpSocket();
	void start(unsigned short port);
	void render(const Request& rq, const Response& rp);
	void handleRequest(Socket& req);
	void Quit() {quit = true;close();}
	bool quit = false;
	void tryCreateTempDir();
	// void Receive(SocketStream& client);
	void Receive(const SocketStream client);
	
	virtual ~HttpSocket();
private:
	std::map<String, HttpHandler> PostFunc;
	std::map<String, HttpHandler> GetFunc;
	ThreadPool* threadPool = NULL;
	HttpHandler* HandleUrl(Request& rq, Response& rp);
	
	bool ReceiveHeader(Request& rq);
	void ParseMultipartFile(Request& rq);
	void ParseMultipartFileHeader(Request& rq);
	void CheckMultipartFileHeader(Request& rq);
	bool NewReceiveMultipartFile(const char* buf, size_t len, Request& rq);
	void resetEnvDir();
	bool RegexValue(const String& content, const String& regex, String& result);
	void ResponseHeader(Request& rq, Response& rp);
	void ResponseBody(Request& rq, Response& rp);
};

#endif  /*HTTPSOCKET_H*/
