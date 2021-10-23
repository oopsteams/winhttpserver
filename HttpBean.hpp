#ifndef  __HTTP_BEAN_H
#define  __HTTP_BEAN_H
#include <string>
#include <malloc.h>
#include <direct.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <memory>
#include <functional>
#include <sys/stat.h>
#include "String.hpp"
#include "SocketStream.h"
#include <time.h>
#include <regex>
#include <stdio.h>
#define GET 0
#define POST 1
namespace TimeUtils{
    std::string getHourDirName(const std::string& prefix="D");
    std::string getDayDirName(const std::string& prefix="F");
    std::string getLastDayDirName(const std::string& prefix="F");
};
namespace File {
    bool Create(const std::string& filename);
    bool Delete(const std::string& filename);
    bool Exists(const std::string& filename);
    bool Move(const std::string& oldname, const std::string& newname);
    void ReadFile(const std::string& filename, std::string& outData);
    void WriteFile(const std::stringstream& data, const std::string& filename);
    void appendFile(const std::string& data, const std::string& filename);
};
namespace Path{
    class FileWatcher {
    private:
        std::string match = "*.*";
        std::string path;
        std::function<void(const std::string& filename)> callback = NULL;
        size_t sleep;
        bool ctn = true;
        void TaskFunc();
    public:
        FileWatcher(const std::string& path, const std::string& match, const std::function<void(const std::string& filename)>& callback, size_t sleep = 500);
        virtual ~FileWatcher();
    };
    bool Create(const std::string& path);
    bool Delete(const std::string& directoryName);
    std::vector<std::string> SearchFiles(const std::string& path, const std::string& pattern);
    bool Exists(const std::string& path);
    std::string GetFileNameWithoutExtension(const std::string& _filename);
    std::string GetDirectoryName(const std::string& _filename);
    std::string GetFileName(const std::string& _filename);
    std::string GetExtension(const std::string& _filename);
    std::string StartPath();
    std::string GetModuleFileName();
};
namespace FileSystem {
    typedef enum: unsigned char{
        None,
        File,
        Directory
    }FileType;
    struct FileInfo
    {
        private:
            std::ifstream* fs = NULL;
        public:
            unsigned long long StreamPos = 0;
            struct  _stat64 __stat;
            FileType fileType = FileSystem::FileType::None;
            std::string Extension;
            std::string FullName;
            std::string FileName;
            std::string contentType;
            bool ReadOnly = false;
            size_t Read(char* _buf_, size_t _rdCount = 512) {
                size_t rdbufCount = _rdCount;
                if(StreamPos + _rdCount >= __stat.st_size){
                    rdbufCount = __stat.st_size - StreamPos;
                }
                if(rdbufCount == 0){
                    return 0;
                }
                if(fs == NULL){
                    fs = new std::ifstream(FullName, std::ios::binary);
                }
                fs->seekg(StreamPos);
                fs->read(_buf_, rdbufCount);
                StreamPos += rdbufCount;
                return rdbufCount;
            }
            FileInfo(){}
            FileInfo(const std::string& filename){
                int status = _stat64(filename.c_str(), &__stat);
                if(status == 0 && (__stat.st_mode & S_IFREG) == S_IFREG) {
                    Extension = Path::GetExtension(filename);
                    FileName = Path::GetFileName(filename);
                    FullName = filename;
                    fileType = FileType::File;
                }
            }
            void Close() {
                if (fs) {
                    fs->close();
                    delete fs;
                    fs = NULL;
                }
            }
            ~FileInfo() {
                if (fs) {
                    fs->close();
                    delete fs;
                }
            }
    };
    //void ReadFileInfoWin32(const std::string& directory, WIN32_FIND_DATAA& pNextInfo, std::vector<FileSystem::FileInfo>& result);
    size_t Find(const std::string& directory, std::vector<FileSystem::FileInfo>& result, const std::string& pattern = "*.*");
};

class Form{
public:
    size_t ContentLength;
    size_t DataCount;
    String Filed;
    size_t Read(char* buf, size_t count) {
        memset(buf, 0, count);
        for(size_t i=0;i<DataCount;i++){
            buf[i] = DataPos[i];
        }
        return 0;
    }
    char* DataPos = NULL;
};
class MultipartForm{
public:
    MultipartForm(){}
    virtual ~MultipartForm(){}
    String temp;
    String tag;
    std::regex regexTag;
    bool findTag = false;
    bool findStart = false;
    bool findEnd = false;
    bool findFileHeader = false;
    FileSystem::FileInfo currentFile;
    size_t startPos = 0;
    size_t endPos = 0;
    std::map<String, String> params;
    std::map<String, FileSystem::FileInfo> raws;
};
class Request {
public:
    Request(){}
    virtual ~Request(){}
    size_t HeadPos = std::string::npos;
    String Temp;
    String Cookie;
    String RawUrl;
    String Url;
    String Header;
    std::vector<Form> Forms;
    std::map<String, String> Headers;
    char Method = GET;
    std::shared_ptr<cw::SocketStream> Client;
    std::shared_ptr<MultipartForm> mForm;
    size_t ContentLength = 0;
    bool hasMultipart = false;
    String ParamString;
    String GetParam(const String& key) const{
        std::map<String, String> Params;
        if(!ParamString.empty()){
            std::vector<String> arr;
            ParamString.Split("&", arr);
            for(auto& it : arr) {
                size_t eq_ = it.find("=");
                String key_ = it.substr(0, eq_);
                String value = it.substr(eq_ + 1);
                if (key == key_){
                    return value;
                }
            }
        }
        return "";
    }
    bool GetHeader(const String& key, String& value)const{
        for (auto& it: Headers) {
            if(it.first == key){
                value = it.second;
                return true;
            }
        }
        return false;
    }
    int ReadStream(String& buf, size_t _Count) const{
        buf.clear();
        Request* ptr = (Request*)this;
        if (ptr->ReadCount >= ContentLength || ptr->ContentLength == -1) {
            return 0;
        }
        if (!Temp.empty()){
            buf.append(Temp.c_str(), Temp.size());
            ptr->ReadCount += Temp.size();
            ptr->Temp.clear();
            return buf.size();
        }
        std::shared_ptr<char> buf2(new char[_Count]);
        int len = Client->Receive(buf2.get(), _Count);
        if(len != -1 && len != 0){
            buf.append(buf2.get(), len);
            ptr->ReadCount += len;
        }
        return len == -1 ? 0:len;
    }
    size_t ReadStreamToEnd(String& body, size_t _Count)const{
        String* buf = new String;
        while (ReadStream(*buf, _Count) > 0)
        {
            body.append(buf->c_str(), buf->size());
        }
        //delete buf;
        return body.size();
    }
private:
    size_t ReadCount = 0;
};
class Response {
public:
    Response(){}
    virtual ~Response(){if(fileinfo){delete fileinfo;}}
    size_t Status = 0;
    String Cookie;
    String Body;
    String Location;
    String ContentType;
    std::map<String, String> Headers;
    FileSystem::FileInfo* fileinfo = NULL;
    void SetContent(const String& body, const String& ContentType_ = "text/plain"){
        Body = body;
        ContentType = ContentType_;
    }
    void AddHeader(const String& key, const String& value){
        bool exist = false;
        for(auto& it : Headers){
            if(it.first == key){
                it.second = value;
                exist = true;
                break;
            }
        }
        if (!exist) {
            Headers.emplace(std::pair<String, String>(key, value));
        }
    }
    void RemoveHeader(const String& key){
        Headers.erase(key);
    }
    
private:
    size_t ReadCount = 0;
};

namespace FileSystem {
    inline size_t Find(const std::string& directory, std::vector<FileSystem::FileInfo>& result, const std::string& pattern)
    {
        _finddata_t fd;
        intptr_t handle;
       String dir(directory);
       if (!dir.endsWith("\\")){
           dir = dir + "\\";
       }
       String patternDir = dir + pattern;
        handle = _findfirst(patternDir.c_str(), &fd);
        if(handle == -1){
            return handle;
        } else {
           do{
               bool skip = false;
               String fname = fd.name;
               if(!skip){
                if(fd.attrib & _A_SUBDIR){
                    if (strcmp(fd.name,".") != 0 && strcmp(fd.name,"..") != 0){
                        String fullname = dir + fname;
                        //printf("is sub dir:%s,fn:%s.\n", fd.name, fullname.c_str());
                        FileSystem::FileInfo fi(fd.name);
                        fi.fileType = FileSystem::FileType::Directory;
                        fi.FullName = fullname;
                        result.push_back(fi);
                    }
                } else {
                   String fullname = dir + fname;
                    //printf("is file dir:%s,fn:%s.\n", fd.name, fullname.c_str());
                    FileSystem::FileInfo fi(fd.name);
                    fi.fileType = FileSystem::FileType::File;
                    fi.FullName = fullname;
                    result.push_back(fi);
                }
               }
           } while (_findnext(handle,&fd) == 0);
           _findclose(handle);
        }
        return 0;
    }
};
namespace File {
#ifdef _WINDEF_
    inline bool Exists(const std::string& filename) {
        DWORD dwAttr = GetFileAttributesA(filename.c_str());
        if (dwAttr == DWORD(-1)) {
            return false;
        }
        if (dwAttr & FILE_ATTRIBUTE_ARCHIVE) {
            return true;
        }
        return false;
    }
#else
    inline bool Exists(const std::string& filename) {
        struct stat buf;
        int status = stat(filename.c_str(), &buf);
        if (status == 0 && (buf.st_mode & S_IFREG) == S_IFREG){
            return true;
        }
        return false;
    }
#endif
    inline bool Create(const std::string& filename) {
        File::Delete(filename);
        std::ofstream ofs(filename.c_str(), std::ios::app);
        ofs.flush();
        ofs.close();
        return true;
    }
    inline std::string ReadFile(const std::string& filename){
        std::stringstream ss;
        std::ifstream ifs(filename, std::ios::binary);
        ss << ifs.rdbuf();
        return ss.str();
    }
    inline bool Delete(const std::string& filename){
        printf("will Delete file %s.\n", filename.c_str());
        ::remove(filename.c_str());
        return !File::Exists(filename);
    }
    inline bool Move(const std::string& oldname, const std::string& newname) {
        if (!File::Delete(newname)){
            printf("Move Failed! The target file is in use\n");
            return false;
        }
        int code = ::rename(oldname.c_str(), newname.c_str());
        if(File::Exists(oldname)){
            return false;
        }
        return true;
    }
    inline void ReadFile(const std::string& filename, std::string& outData){
        outData.clear();
        std::ifstream* ifs = new std::ifstream(filename.c_str(), std::ios::binary);
        std::stringstream ss;
        ss << ifs->rdbuf();
        ifs->close();
        outData = ss.str();
        delete ifs;
    }
    inline void WriteFile(const std::stringstream& data, const std::string& filename){
        std::string buf = data.str();
        File::Delete(filename);
        std::ofstream* ofs = new std::ofstream(filename, std::ios::binary);
        ofs->write(buf.c_str(), buf.size());
        ofs->flush();
        ofs->close();
        delete ofs;
    }
    inline void appendFile(const std::string& data, const std::string& filename){
        std::ofstream ofs(filename.c_str(), std::ios::out|std::ios::binary|std::ios::app);
        ofs.write(data.c_str(), data.size());
        ofs.flush();
        ofs.close();
    }
};
namespace Path{
    inline void FileWatcher::TaskFunc()
    {
        /*
        std::vector<std::string> files;
        for(;exit;)
        {
            for(auto it = files.begin();it !=files.end();){
                if(!File::Exists(it)){
                    it = files.erase(it);
                } else {
                    ++it;
                }
            }
            std::vector<std::string> tmp = Path::SearchFiles(path, match.c_str());
        }
        */
    }
    inline FileWatcher::FileWatcher(const std::string& path, const std::string& match, const std::function<void(const std::string& filename)>& callback, size_t sleep)
    {
        this->sleep = sleep;
        this->callback = callback;
        this->path = path;
        this->match = match;
    }
    inline FileWatcher::~FileWatcher()
    {
        ctn = false;
    }

#ifdef _WINDEF_
    inline bool Exists(const std::string& path) {
        DWORD dwAttr = GetFileAttributesA(path.c_str());
        if (dwAttr == DWORD(-1)) {
            return false;
        }
        if (dwAttr & FILE_ATTRIBUTE_DIRECTORY) {
            return true;
        }
        return false;
    }
#else
    inline bool Exists(const std::string& path) {
        struct stat buf;
        int status = stat(path.c_str(), &buf);
        if (status == 0){
            return true;
        }
        return false;
    }
#endif
    inline bool Create(const std::string& path){
        if(Path::Exists(path)){
            return true;
        }
        LPSTR _path = const_cast<char*>(path.c_str());
        return ::CreateDirectory(path.c_str(), NULL);
    }
    inline bool Delete(const std::string& directoryName) {
        std::vector<FileSystem::FileInfo>result;
        FileSystem::Find(directoryName, result);
        for(auto& it: result){
            if(it.fileType == FileSystem::FileType::File){
                File::Delete(it.FullName);
                printf("del file:%s\n", it.FullName.c_str());
            } else if(it.fileType == FileSystem::FileType::Directory){
                Path::Delete(it.FullName);
                printf("del dir:%s\n", it.FullName.c_str());
            }

        }
        /*
        bool bRet = RemoveDirectoryA(directoryName.c_str());
        if(bRet == 0){
            printf("Del %s Failed!\n", directoryName.c_str());
            return false;
        } else {
            printf("Del %s Success!\n", directoryName.c_str());
        }
        */
        return true;
    }
    inline std::string GetFileNameWithoutExtension(const std::string& _filename) {
        std::string newDir = String(_filename).Replace("\\","/");
        int bPos = newDir.rfind("/");
        int ePos = newDir.rfind(".");
        newDir = newDir.substr(bPos+1, ePos-bPos-1);
        return newDir;
    }
    inline std::string GetExtension(const std::string& _filename) {
        size_t pos = _filename.rfind(".");
        return pos == size_t(-1)?"":_filename.substr(pos);
    }
    inline std::string GetDirectoryName(const std::string& _filename){
        int pos = String(_filename).Replace("\\", "/").rfind("/");
        return _filename.substr(0, pos);
    }
    inline std::string GetFileName(const std::string& filename){
        return Path::GetFileNameWithoutExtension(filename) + Path::GetExtension(filename);
    }
    inline std::string StartPath() {
        return Path::GetDirectoryName(GetModuleFileName());
    }
    inline std::string GetModuleFileName() {
        CHAR exeFullPath[MAX_PATH];
        ::GetModuleFileNameA(NULL, exeFullPath, MAX_PATH);
        std::string filename = exeFullPath;
        return filename;
    }
};
namespace TimeUtils{
    inline std::string getHourDirName(const std::string& prefix) {
        std::time_t timeT = time(NULL);
        struct tm* nowLocalTime;
        nowLocalTime = localtime(&timeT);
        char tstr[12];
        memset(tstr, 0, sizeof(tstr));
        sprintf(tstr, "%02d_%02d_%02d_%02d", nowLocalTime->tm_year, nowLocalTime->tm_mon, nowLocalTime->tm_mday, nowLocalTime->tm_hour);
        return (prefix+tstr);
    }
    inline std::string getDayDirName(const std::string& prefix){
        std::time_t timeT = time(NULL);
        struct tm* nowLocalTime;
        nowLocalTime = localtime(&timeT);
        char tstr[12];
        memset(tstr, 0, sizeof(tstr));
        sprintf(tstr, "%02d", nowLocalTime->tm_mday);
        return (prefix+tstr);
    }
    inline std::string getLastDayDirName(const std::string& prefix){
        std::time_t timeT = time(NULL) - 24 * 60 * 60;
        struct tm* nowLocalTime;
        nowLocalTime = localtime(&timeT);
        char tstr[12];
        memset(tstr, 0, sizeof(tstr));
        sprintf(tstr, "%02d", nowLocalTime->tm_mday);
        return (prefix+tstr);
    }
    inline std::string getDateDirName(const std::string& prefix){
        std::time_t timeT = time(NULL);
        struct tm* nowLocalTime;
        nowLocalTime = localtime(&timeT);
        char tstr[12];
        memset(tstr, 0, sizeof(tstr));
        sprintf(tstr, "%02d_%02d_%02d", nowLocalTime->tm_year, nowLocalTime->tm_mon, nowLocalTime->tm_mday);
        return (prefix+tstr);
    }
    inline std::string getLastDateDirName(const std::string& prefix){
        std::time_t timeT = time(NULL) - 24 * 60 * 60;
        struct tm* nowLocalTime;
        nowLocalTime = localtime(&timeT);
        char tstr[12];
        memset(tstr, 0, sizeof(tstr));
        sprintf(tstr, "%02d_%02d_%02d", nowLocalTime->tm_year, nowLocalTime->tm_mon, nowLocalTime->tm_mday);
        return (prefix+tstr);
    }
};
#endif