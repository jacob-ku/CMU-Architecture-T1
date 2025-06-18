#include "TestCurl.h"

using namespace std;

TestCurl::TestCurl()
{

}

TestCurl::~TestCurl()
{

}
void TestCurl::FunctionTest(){
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    std::string response;

    if (!curl) {
        std::cerr << "CURL initialization failed." << std::endl;
        return;
    }

    curl_easy_setopt(curl, CURLOPT_URL, "https://example.com");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, TestCurl::WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "Request failed: " << curl_easy_strerror(res) << std::endl;
    }
        
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return ;

}

size_t TestCurl::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
 {
    size_t totalSize = size * nmemb;
    return totalSize;
}