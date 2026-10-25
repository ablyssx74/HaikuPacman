#ifndef STUB_STRING_H
#define STUB_STRING_H
#include <string>
class BString {
public:
    BString() {}
    BString(const char* s) : fText(s) {}
    void Trim() {
        const size_t a = fText.find_first_not_of(" \t\r\n");
        const size_t b = fText.find_last_not_of(" \t\r\n");
        fText = (a == std::string::npos) ? std::string() : fText.substr(a, b - a + 1);
    }
    int Length() const { return (int)fText.size(); }
    const char* String() const { return fText.c_str(); }
    BString& operator<<(const char* s) { fText += s; return *this; }
    BString& operator<<(const BString& s) { fText += s.fText; return *this; }
private:
    std::string fText;
};
#endif
