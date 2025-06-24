#include <iostream>

#include <curl/curl.h>
#define CURL_STATICLIB

#pragma comment(lib, "libcurl-bcc-x64.lib")
class TestCurl
{

    private:
    CURL* curl;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    
    public:
    void FunctionTest(void);

    TestCurl(void);
    ~TestCurl(void);

};

