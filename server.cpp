#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#include    "HttpSocket.h"
#include <stdio.h>
#include <windows.h>
#define _WIN32_WINNT 0x0601

int main(int argc, char *argv[])
{
	try{
		HttpSocket server;
		server.WebRoot = Path::StartPath();
		String dataPath =  server.WebRoot + "\\data";
		String outPath = server.WebRoot + "\\out";
		String logPath = server.WebRoot + "\\log";
		printf("webroot:%s.",server.WebRoot.c_str());
		if (!Path::Exists(dataPath)){
			Path::Create(dataPath);
		}
		if (!Path::Exists(outPath)){
			Path::Create(outPath);
		}
		if (!Path::Exists(logPath)){
			Path::Create(logPath);
		}
		server.dataDir = dataPath;
		server.outDir = outPath;
		server.logDir = logPath;
		server.Get("/api", [&](const Request& rq, Response& rp){
			String cmd = rq.GetParam("cmd");
			
			rp.SetContent("{\"rs\":0}", "application/json");
			if(cmd.size()>0){
				printf("cmd:%s\n",cmd.c_str());
				if(cmd == "quit"){
					printf("will call Quit.\n");
					server.Quit();
				} else if(cmd == "render"){
					//String outf = rq.GetParam("o");
					//String infstr = rq.GetParam("i");
					server.render(rq, rp);
					/*
					if(infstr.size()>0){
						
						std::vector<String> infs;
						infstr.Split("~", infs);
						String params = rq.GetParam("p");
						int probeIdx = params.find("-probe");
						if(probeIdx >=0){
							params = params.Replace("-i~"+infs[0],"\""+server.dayDir+"\\"+infs[0]+"\"");
						} else {
							for(auto inf : infs){
								params = params.Replace("-i~"+inf,"-i \""+server.dayDir+"\\"+inf+"\"");
							}
						}
						int ofileIdx = params.find("-ofile");
						if(ofileIdx>0 && outf.size()>0){
							params = params.Replace("-ofile",server.outDayDir+"\\"+outf);
						}
						String _p = params.Replace("~"," ");
						String _command = server.WebRoot + "\\render.exe " + _p ;
						printf("_command:%s.\n", _command.c_str());
						STARTUPINFOA si;
						PROCESS_INFORMATION pi;
						memset(&si,0, sizeof(si));
						memset(&pi,0, sizeof(pi));
						LPSTR COMMAND = const_cast<char*>(_command.c_str());
						bool working = ::CreateProcess(NULL, COMMAND, NULL, NULL,FALSE, NORMAL_PRIORITY_CLASS,NULL,NULL,&si,&pi);
						if(working == 0){
							printf("create process failed.\n");
						} else {
							printf("wait process deal...\n");
							WaitForSingleObject(pi.hProcess, INFINITE);
							printf("wait process deal complete.\n");
							unsigned long Result;
							GetExitCodeProcess(pi.hProcess, &Result);
						}
						
					}
					*/
				} else if(cmd == "check"){
					
				} else if(cmd == "clean"){
					if (Path::Exists(server.dayDir)){
						Path::Delete(server.dayDir);
					}
				}
			}
		});
		server.Post("/api", [&](const Request& rq, Response& rp){
			String cmd = rq.GetParam("cmd");
			if(cmd == "upload"){
				String fn = rq.GetParam("fn");
				printf("fn:%s.\n", fn.c_str());
			}
			rp.SetContent("{\"rs\":0}", "application/json");
			//Path::Delete(server.WebRoot);
		});
		server.start(8080);
		printf("server quit!");
	}catch(Error x)
	{
		x.showError();
	}
	return 0;
}
