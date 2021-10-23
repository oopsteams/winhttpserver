#ifndef  __STRING_H
#define  __STRING_H
#include <string>
#include <vector>
class String : public std::string {
public:
    String(){}
    String(const char* cstr): std::string(cstr){}
    String(const std::string& str):std::string(str){}
    size_t Split(const String& ch_, std::vector<String>& result) const {
        String buf = *this;
        size_t pos = buf.find(ch_);
        if (pos == size_t(-1)) {
            result.push_back(*this);
            return result.size();
        }
        for(;pos != size_t(-1);) {
            if (pos == 0){
                result.push_back("");
            } else {
                result.push_back(buf.substr(0,pos));
            }
            buf = buf.erase(0, pos+ch_.size());
            pos = buf.find(ch_);
            if (pos == size_t(-1)){
                result.push_back(buf);
            }
        }
        return result.size();
    }
    String toLower() const {
        const std::string& str = *this;
        char* cStr = (char*)malloc(str.size() + 1);
        size_t pos = 0;
        for(auto ch: str){
            char newCh = ch;
            if(ch>=65 && ch<=90){
                newCh = ch+32;
            }
            cStr[pos] = newCh;
            pos++;
        }
        cStr[str.size()] = 0;
        std::string newStr = cStr;
        free(cStr);
        return newStr;
    }
    String toUpper() const {
        const std::string& str = *this;
        char* cStr = (char*)malloc(str.size()+1);
        size_t pos = 0;
        for (auto ch : str){
            char newCh = ch;
            if(ch >=97 && ch<= 122){
                newCh = ch - 32;
            }
            cStr[pos] = newCh;
            pos++;
        }
        cStr[str.size()] = 0;
        std::string newStr = cStr;
        free(cStr);
        return newStr;
    }
    String& Replace(const String& oldText, const String& newText) {
        size_t pos;
        pos = (*this).find(oldText);
        size_t count = 0;
        for(; pos != std::string::npos;){
            (*this).replace(pos, oldText.size(), newText);
            count ++;
            pos = (*this).find(oldText);
        }
        return (*this);
    }
    String& trim() {
        char* buf = new char[size() + 1]{0};
        int i = 0;
        for(char it : *this){
            if(it != 32){
                buf[i] = it;
                i++;
            }
        }
        this->erase(0, size());
        this->append(buf);
        return (*this);
    }
    int startsWith(const std::string& prefix){
        return this->find(prefix)==0?1:0;
    }
    int endsWith(const std::string& suffix){
        size_t idx = this->rfind(suffix);
        return (this->size()-suffix.size() == idx)?1:0;
    }
};
#endif