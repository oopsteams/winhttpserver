#include    "HttpSocket.h"
#include  <stdio.h>
USING_CW

HttpSocket::HttpSocket()
{
	threadPool = new ThreadPool(12);
	create(AF_INET,SOCK_STREAM);
}
HttpSocket::~HttpSocket()
{
	if(threadPool){
		delete threadPool;
	}
}
void HttpSocket::tryCreateTempDir(){

}
void HttpSocket::Get(const String& url, const HttpHandler& func){
	GetFunc.emplace(std::pair<String, HttpHandler>(url, func));
}
void HttpSocket::Post(const String& url, const HttpHandler& func){
	PostFunc.emplace(std::pair<String, HttpHandler>(url, func));
}
void HttpSocket::resetEnvDir(){
	std::string lastDayDirName = TimeUtils::getLastDayDirName();
	std::string lastDayDir = dataDir + "\\" + lastDayDirName;
	std::string lastDayOutDir = outDir + "\\" + lastDayDirName;
	std::string currentDayDirName = TimeUtils::getDayDirName();
	std::string currentDayDir = dataDir + "\\" + currentDayDirName;
	std::string currentDayOutDir = outDir + "\\" + currentDayDirName;
	dayDir = currentDayDir;
	outDayDir = currentDayOutDir;
	//printf("lastDayDir:%s\n", lastDayDir.c_str());
	//printf("currentDayDir:%s\n", currentDayDir.c_str());
	
	if (Path::Exists(lastDayDir)){
		Path::Delete(lastDayDir);
	}
	if (Path::Exists(lastDayOutDir)){
		Path::Delete(lastDayOutDir);
	}
	
	if (!Path::Exists(currentDayDir)){
		printf("need create currentDayDir:%s\n", currentDayDir.c_str());
		if (Path::Create(currentDayDir)){
			printf("create currentDayDir ok!\n");
		}
	}
	if (!Path::Exists(currentDayOutDir)){
		if (Path::Create(currentDayOutDir)){
			printf("create currentDayDir ok!\n");
		}
	}
}
void HttpSocket::Receive(const SocketStream client) {
	printf("%s:%d\n", Address.c_str(), Port);
	resetEnvDir();
	std::shared_ptr<Request> auto_req(new Request);
	std::shared_ptr<Response> auto_resp(new Response);
	Request &rq = *auto_req;
	Response &rp = *auto_resp;
	rq.Client = std::shared_ptr<SocketStream>(new SocketStream(client));
	if(!ReceiveHeader(rq)){
		printf("Ternimation of receiving, abnormal message\n");
		//client.close();
		rq.Client->close();
		return;
	};
	
	auto handler = HandleUrl(rq, rp);
	if(handler){
		(*handler)(rq, rp);
	}
	ResponseHeader(rq, rp);
	ResponseBody(rq, rp);
	String keep_alive;
	if(rq.GetHeader("Connection", keep_alive) && keep_alive == "keep-alive"){
		printf("keep-alive\n");
		printf("wait accepting...\n");
		//return false;
		threadPool->enqueue(&HttpSocket::Receive, this, *(rq.Client));
	} else {
		printf("Connection close\n");
		rq.Client->close();
		//return true;
	}
}
void HttpSocket::start(unsigned short port)
{
	bind(port);
	listen(256);
#ifdef linux
	Socket cli;
	for(;;)
	{
		/* here may exist a close error,the server
		 * do not close the cli.m_socket!,so I change
		 * the Socket::close() function.Not perfect yet*/
		accept(cli);
		pid_t childpid;
		if((childpid=fork())==0)
		{
			close();
			handleRequest(cli);
			cli.close();
			exit(0);
		}
		cli.close();
		printf("will call quit:%d.\n", quit);
		if(quit){
			break;
		}
	}
#else
	for(;;)
	{
		SOCKET clientSocket = Accept();
		printf("loop in connect =======>sk:%d.", clientSocket);
		
		if(quit){
			printf("will call quit:%d.\n", quit);
			threadPool = NULL;
			break;
		} else {
			if(clientSocket){
				std::shared_ptr<SocketStream> skt(new SocketStream(clientSocket));
				threadPool->enqueue(&HttpSocket::Receive, this, *skt);
			}
		}
	}
#endif

}
HttpHandler* HttpSocket::HandleUrl(Request& rq, Response& rp){
	if(rq.Method == GET){
		for(auto& it:GetFunc){
			if(it.first == rq.Url){
				return &it.second;
			}
		}
		String filename = WebRoot + "\\" + rq.Url;
		if (File::Exists(filename)){
			String ext = String(Path::GetExtension(filename)).Replace(".","");
			if(ext=="html"||ext=="htm"){
				rp.ContentType = "text/html";
			}else if(ext == "mp4"){
				rp.ContentType = "video/" + ext;
			}else if(ext == "mp3"){
				rp.ContentType = "audio/mp3";
			}else if(ext == "ogg"){
				rp.ContentType = "audio/ogg";
			}else if(ext == "jpg" || ext == "jpeg"){
				rp.ContentType = "image/jpeg";
			}else{
				rp.ContentType = "application/octet-stream";
			}
			rp.Status = 200;
			rp.fileinfo = new FileSystem::FileInfo(filename);
		} else {
			rp.Status = 404;
		}
		return NULL;
	}else if(rq.Method == POST){

		for(auto& it:PostFunc){
			if(it.first == rq.Url){
				return &it.second;
			}
		}
	}
	return NULL;
}
bool HttpSocket::ReceiveHeader(Request& rq){
	char buf[UpSize];
	bool EndHead = false;
	bool ok = false;
	int total = 0;
	for(;;){
		int len = rq.Client->Receive(buf, UpSize);
		if (len == -1 || len == 0){
			printf("Connetion disconect!\n");
			break;
		}
		total += len;
		if(rq.Header.empty()){
			String tempStr = buf;
			size_t pos1 = tempStr.find("POST /");
			size_t pos2 = tempStr.find("GET /");
			if(pos1 != 0 && pos2 != 0){
				printf("no http!\n");
				break;
			}
			rq.Method =pos1 == 0? POST: GET;
		}
		if (!EndHead){
			if (rq.Header.size()>2048){
				printf("Maximum limit exceeded 2048 byte!\n");
				break;
			}
			rq.Header.append(buf, len);
		} else {
			//rq.Temp.append(buf, len);
		}
		if(!EndHead){
			rq.HeadPos = rq.Header.find("\r\n\r\n");
		}
		printf("loop recv len:%d\n", len);
		if(!EndHead && rq.HeadPos != String::npos){
			EndHead = true;
			rq.Temp = rq.Header.substr(rq.HeadPos + 4);
			rq.Header = rq.Header.substr(0, rq.HeadPos);
			size_t pos = rq.Header.find("/");
			for (size_t i=pos;i<rq.Header.size() - pos;i++){
				if (rq.Header.at(i) == ' '){
					break;
				}
				rq.RawUrl += rq.Header.at(i);
			}
			size_t what = rq.RawUrl.find("?");
			rq.Url = what != String::npos ? rq.RawUrl.substr(0, what):rq.RawUrl;
			rq.ParamString = what != String::npos? rq.RawUrl.substr(what+1):"";
			printf("request url:%s,querystring:%s\n", rq.Url.c_str(), rq.ParamString.c_str());
			std::vector<String> vstr;
			rq.Header.Split("\r\n", vstr);
			for(auto kv = vstr.begin() + 1; vstr.size()>=2 && kv!=vstr.end();kv++){
				size_t mhPos = (*kv).find(":");
				rq.Headers.emplace(std::pair<String, String>((*kv).substr(0, mhPos), (*kv).substr(mhPos + 2)));
			}
			String contentLen;
			if (RegexValue(rq.Header.Replace(" ",""), "Content-Length:(\\d+)", contentLen)){
				try{
					rq.ContentLength = std::stoi(contentLen);
				}catch(const std::exception&){
					rq.ContentLength = -1;
				}
			}
			rq.GetHeader("Cookie", rq.Cookie);
			ok = true;
			NewReceiveMultipartFile(buf, len, rq);
			if(rq.ContentLength>0){
				if(total >= rq.ContentLength){
					break;
				}
			} else {
				break;
			}
		} else {
			bool has = NewReceiveMultipartFile(buf, len, rq);
			if(!has){
				rq.Temp.append(buf, len);
			}
			if(rq.ContentLength>0){
				if(total >= rq.ContentLength){
					break;
				}
			} else {
				break;
			}
		}
	}
	return ok;
}
void HttpSocket::ParseMultipartFile(Request& rq){
	if (rq.mForm->findFileHeader){
		std::sregex_iterator iter(rq.mForm->temp.begin(), rq.mForm->temp.end(), rq.mForm->regexTag);
		std::sregex_iterator iend;
		if(iter != iend){
			std::smatch match = *iter;
			std::string match_str = match.str();
			std::string _temp = rq.mForm->temp.substr(0, match.position());
			//printf("append file end.%s\n", rq.mForm->currentFile.FullName.c_str());
			File::appendFile(_temp, rq.mForm->currentFile.FullName);
			rq.mForm->findFileHeader = false;
			rq.mForm->temp = rq.mForm->temp.erase(0, match.position());
			//printf("append file end, remain size.%d\n", rq.mForm->temp.size());
		} else {
			//printf("append file:%s,len:%d.\n", rq.mForm->currentFile.FullName.c_str(), rq.mForm->temp.size());
			File::appendFile(rq.mForm->temp, rq.mForm->currentFile.FullName);
			rq.mForm->temp.clear();
		}
	}
}
void HttpSocket::ParseMultipartFileHeader(Request& rq){
	if (!rq.mForm->findFileHeader){
		rq.mForm->endPos = rq.mForm->temp.find("\r\n\r\n", rq.mForm->startPos);
		//printf("ParseMultipartFileHeader endPos:%d\n", rq.mForm->endPos);
		if (rq.mForm->endPos >= 0){
			rq.mForm->findFileHeader = true;
			String fileHeader = rq.mForm->temp.substr(rq.mForm->startPos, rq.mForm->endPos - rq.mForm->startPos);
			//printf("ParseMultipartFileHeader fileHeader:%s\n", fileHeader.c_str());
			std::vector<String> vstr;
			fileHeader.Split("\r\n", vstr);
			//printf("ParseMultipartFileHeader header params size:%d\n", vstr.size());
			int cnt = vstr.size();
			for(int i=0;i<cnt;i++){
				auto kv = vstr[i];
				//printf("kv len:%d\n", kv.size());
				if(kv.size() == 0){
					continue;
				}
				size_t mhPos = (kv).find(":");
				if(mhPos>0){
					String key = (kv).substr(0, mhPos);
					String val = (kv).substr(mhPos + 2);
					//printf("ParseMultipartFileHeader key:%s==>%s\n", key.c_str(), val.c_str());
					if(key == "Content-Disposition"){
						mhPos = val.find("filename=");
						if(mhPos>=0){
							String fn = val.substr(mhPos + 9);
							//printf("ParseMultipartFileHeader fn:%s\n", fn.c_str());
							int idx = fn.find("\"");
							if(idx>=0){
								int idx_ = fn.find("\"", idx+1);
								//printf("ParseMultipartFileHeader fn idx:%d,idx_:%d.\n", idx, idx_);
								rq.mForm->currentFile.FileName = fn.substr(idx+1, idx_ - idx - 1);
							} else {
								rq.mForm->currentFile.FileName = fn;
							}
							String target = rq.GetParam("t");
							//printf("parser file header target:%s\n", target.c_str());
							if(target == "out"){
								rq.mForm->currentFile.FullName = outDayDir + "\\" +  rq.mForm->currentFile.FileName;
							} else {
								rq.mForm->currentFile.FullName = dayDir + "\\" +  rq.mForm->currentFile.FileName;
							}
							//printf("currentFile.FullName:%s\n", rq.mForm->currentFile.FullName.c_str());
							File::Delete(rq.mForm->currentFile.FullName);
							//printf("Delete file %s ok.\n", rq.mForm->currentFile.FullName.c_str());
						} else {
							rq.mForm->findFileHeader = false;
						}
					} else if(key == "Content-Type"){
						rq.mForm->currentFile.contentType = val;
						//printf("ParseMultipartFileHeader contentType:%s==>%s\n", rq.mForm->currentFile.contentType.c_str(), val.c_str());
						//break;
					}
				}
			}
			
			if(rq.mForm->findFileHeader){
				String remain = rq.mForm->temp.substr(rq.mForm->endPos + 4);
				rq.mForm->temp.clear();
				rq.mForm->temp.append(remain.c_str(), remain.size());
			}
		}
	}
}
void HttpSocket::CheckMultipartFileHeader(Request& rq){
	if(!rq.mForm->findFileHeader){
		int dog = 20;
		std::sregex_iterator iter(rq.mForm->temp.begin(), rq.mForm->temp.end(), rq.mForm->regexTag);
		std::sregex_iterator iend;
		while(iter != iend && dog > 0){
			std::smatch match = *iter;
			std::string match_str = match.str();
			rq.mForm->startPos = match_str.size() + match.position();
			std::string suffix = rq.mForm->temp.substr(rq.mForm->startPos, 2);
			if(suffix == "--"){
				break;
			}
			ParseMultipartFileHeader(rq);
			if(rq.mForm->findFileHeader){
				break;
			}
			++iter;
			--dog;
		}
		if(rq.mForm->findFileHeader){
			ParseMultipartFile(rq);
			if(!rq.mForm->findFileHeader){
				printf("re find findFileHeader in.");
				CheckMultipartFileHeader(rq);
			}
		}
	} else {
		ParseMultipartFile(rq);
		if(!rq.mForm->findFileHeader){
			printf("loop next, re find findFileHeader in.");
			CheckMultipartFileHeader(rq);
		}
	}
}
bool HttpSocket::NewReceiveMultipartFile(const char* buf, size_t len, Request& rq){
	if (!rq.hasMultipart) {
		String contentType;
		rq.GetHeader("Content-Type",contentType);
		//printf("NewReceiveMultipartFile check contentType:%s\n", contentType.c_str());
		if (contentType.size()>0 && contentType.startsWith("multipart")){
			size_t pos = contentType.find("boundary=");
			//printf("NewReceiveMultipartFile boundary pos:%d\n", pos);
			rq.hasMultipart = true;
			rq.mForm = std::shared_ptr<MultipartForm>(new MultipartForm());
			rq.mForm->tag = contentType.substr(pos + 9);
			rq.mForm->findTag = true;
			rq.mForm->regexTag = std::regex("[\\-]+"+rq.mForm->tag);
			rq.mForm->findFileHeader = false;
			rq.mForm->temp.clear();
			//printf("NewReceiveMultipartFile mForm tag:%s\n", rq.mForm->tag.c_str());
		}
	}
	if (rq.hasMultipart){
		
		rq.mForm->temp.append(buf, len);
		CheckMultipartFileHeader(rq);
	}
	
	return rq.hasMultipart;
}
bool HttpSocket::RegexValue(const String& content, const String& regex, String& result){
	std::regex regexExp(regex);
	std::smatch matchResult;
	if(std::regex_search(content, matchResult, regexExp)){
		if(matchResult.size()>=2){
			result = (String)matchResult[1];
			return true;
		}
	}
	return false;
}
void HttpSocket::ResponseHeader(Request& rq, Response& rp){
	if(rp.fileinfo){
		String lstChange = std::to_string(rp.fileinfo->__stat.st_mtime);
		String inm;
		if(rq.GetHeader("If-None-Match", inm) && inm.trim() == lstChange){
			rp.Status = 304;
		}else{
			rp.Status = 200;
		}
		rp.AddHeader("ETag", lstChange);
	}
	if(!rp.Location.empty()){
		rp.Status = 302;
		rp.AddHeader("Location", rp.Location);
	} else {
		rp.Status = 200;
	}
	String header("HTTP/1.1 " + std::to_string(rp.Status) + " OK \r\n");
	rp.AddHeader("Content-Length", std::to_string(rp.fileinfo?rp.fileinfo->__stat.st_size:rp.Body.size()));
	rp.AddHeader("Accept-Ranges", "none");
	rp.AddHeader("Access-Control-Allow-Origin", "*");
	rp.AddHeader("Access-Control-Allow-Credentials", "true");
	rp.AddHeader("Access-Control-Allow-Headers", "*");
	rp.AddHeader("Connection", "close");
	rp.AddHeader("Server", "Local Server 1.0");
	if(!rp.Cookie.empty()){
		rp.AddHeader("Set-Cookie", rp.Cookie);
	}
	if(!rp.ContentType.empty()){
		rp.AddHeader("Content-Type", rp.ContentType);
	}
	for(auto& it: rp.Headers){
		header.append(it.first + ": " + it.second + "\r\n");
	}
	header.append("\r\n");
	//printf("-------------------------\n%s\n", header.c_str());
	rq.Client->writen(header.c_str(), header.size());
}
void HttpSocket::ResponseBody(Request& rq, Response& rp){
	if(!rp.fileinfo){
		if(rp.Body.size()!=0){
			rq.Client->writen(rp.Body.c_str(), rp.Body.size());
		}
	}else{
		if(rp.Status != 304){
			char buf2[DownSize];
			size_t ct = 0;
			while ((ct=rp.fileinfo->Read(buf2,DownSize))>0)
			{
				if(ct!=rq.Client->writen(buf2, ct)){
					printf("File transfer failed!!");
					break;
				}
			}
			
		}
	}
}
void HttpSocket::render(const Request& rq, const Response& rp){
	String outf = rq.GetParam("o");
	String infstr = rq.GetParam("i");
	String target = rq.GetParam("t");

	if(infstr.size()>0){
		if(target.size() == 0){
			target = "in";
		}
		String outLog = logDir + "\\render.log";
		std::vector<String> infs;
		infstr.Split("~", infs);
		String params = rq.GetParam("p");
		int probeIdx = params.find("-probe");
		String _baseDir = dayDir;
		if(target == "out"){
			_baseDir = outDayDir;
		}
		printf("render _baseDir:%s\n", _baseDir.c_str());
		if(probeIdx >=0){
			params = params.Replace("-i~"+infs[0],"\""+_baseDir+"\\"+infs[0]+"\"");
		} else {
			for(auto inf : infs){
				params = params.Replace("-i~"+inf,"-i \""+_baseDir+"\\"+inf+"\"");
			}
		}
		int ofileIdx = params.find("-ofile");
		if(ofileIdx>0 && outf.size()>0){
			params = params.Replace("-ofile","\""+outDayDir+"\\"+outf+"\"");
		}
		String _p = params.Replace("~"," ");
		String _command = WebRoot + "\\render.exe " + _p ;
		printf("_command:%s.\n", _command.c_str());
		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(sa);
		sa.lpSecurityDescriptor = NULL;
		sa.bInheritHandle = TRUE;
		//FILE_APPEND_DATA
		printf("render outLog:%s\n", outLog.c_str());
		HANDLE logFile = CreateFile(const_cast<char*>(outLog.c_str()), 
		FILE_APPEND_DATA, 
		FILE_SHARE_WRITE|FILE_SHARE_READ,
		&sa, //&sa
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
		);

		STARTUPINFOA si;
		PROCESS_INFORMATION pi;
		memset(&si,0, sizeof(si));
		memset(&pi,0, sizeof(pi));
		si.cb = sizeof(STARTUPINFOA);
		si.dwFlags |= STARTF_USESTDHANDLES;
		si.hStdInput = NULL;
		si.hStdError = logFile;
		si.hStdOutput =  logFile;
		//si.wShowWindow = SW_HIDE;
		//DWORD flags = CREATE_NO_WINDOW;
		LPSTR COMMAND = const_cast<char*>(_command.c_str());
		PCTSTR psCurDir = const_cast<char*>(_baseDir.c_str());
		bool working = ::CreateProcess(NULL, COMMAND, NULL, NULL,TRUE, CREATE_NO_WINDOW,NULL,psCurDir,&si,&pi);
		//bool working = ::CreateProcess(NULL, COMMAND, NULL, NULL,FALSE, NORMAL_PRIORITY_CLASS,NULL,psCurDir,&si,&pi);
		if(working == 0){
			printf("create process failed.\n");
		} else {
			printf("wait process deal...%d\n", working);
			WaitForSingleObject(pi.hProcess, INFINITE);
			printf("wait process deal complete.\n");
			unsigned long Result;
			GetExitCodeProcess(pi.hProcess, &Result);
			printf("process deal complete result:%d.\n", Result);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
		CloseHandle(logFile);
	}
}

void HttpSocket::handleRequest(Socket& req)
{
	/*
	char buf[MAXLINE];//,method[MAXLINE],uri[MAXLINE],version[MAXLINE];
	memset(buf, 0, sizeof(buf));
	SocketStream ss=req.getSocketStream();
	ss.readlineb(buf,MAXLINE);
	printf("handleRequest readlineb:%8x,%8x,%8x,%8x.", buf[0],buf[1],buf[2],buf[3]);
	//fputs(buf,stdout);
	if (buf[1] & 0x2a) 
	{
		printf("is ack!");
		return;
	} else if(buf[0] == 0){
		printf("is sock expired!");
		//req.close();
		return;
	} else {
		printf("handleRequest readlineb:%s.", buf);
	}
	char ret[MAXLINE];
	char body[MAXLINE];
	//send the response
	
	sprintf(body,"<html>\
			<title>test Web Server</title>\
			<body>\
				Hello World!Just a Web Server test:)\
			</body>\
			</html>\r\n");
	sprintf(ret,"HTTP/1.1 200 OK\r\n\
			Server: test Web Server\r\n\
			Content-type: text/html\r\n\
			Content-length: %d\r\n\r\n",strlen(body));
	ss.writen(ret,strlen(ret));
	ss.writen(body,strlen(body));
	*/
}
