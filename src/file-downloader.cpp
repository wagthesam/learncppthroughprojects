#include <filesystem>
#include <string>

#include <curl/curl.h>

namespace {
    size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
        size_t written = fwrite(ptr, size, nmemb, stream);
        return written;
    }
}

namespace NetworkMonitor {

bool DownloadFile(
    const std::string& fileUrl,
    const std::filesystem::path& destination,
    const std::filesystem::path& caCertFile
) {
    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl = curl_easy_init();
    CURLcode res;
    FILE* destinationPointer;
    if (curl) {
        destinationPointer = std::fopen(destination.string().c_str(), "wb");
        if (!destinationPointer) {
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return false;
        }
        curl_easy_setopt(curl, CURLOPT_URL, fileUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, destinationPointer);
        curl_easy_setopt(curl, CURLOPT_CAINFO, caCertFile.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        res = curl_easy_perform(curl);
        fclose(destinationPointer);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return res == CURLE_OK;
    }
    return false;
}

} // namespace NetworkMonitor