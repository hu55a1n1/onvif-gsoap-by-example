#include <stdarg.h>  // For va_start, etc.
#include <memory>    // For std::unique_ptr
#include <ctime>
#include <cstring>	// For strcpy
#include <sstream>	// For stringstream
#include <sys/stat.h>
#include "Snapshot.hpp"


Snapshot::Snapshot(std::string path, std::string name)
		: Media(){	// Call base class constructor
	// Create a name for the snapshot
    this->_snapshotPath = path;
    this->_snapshotName = name;
}

Snapshot::~Snapshot(void){
    // Delete the snapshot file from disk.
    this->deleteFromDisk();
    
}

std::string Snapshot::getDownloadUri(const std::string snapshotUri){
	return Media::string_format("curl --create-dirs -o %s %s", this->_snapshotName.c_str(), snapshotUri.c_str());
}

/*
 * Get the FTP shell cmd for uploading this snapshot to the FTP server.
 */
std::string Snapshot::getUploadUri(const std::string ftpIP, const std::string ftpUser, const std::string ftpPwd){
	return Media::string_format("curl -C - --ftp-create-dirs -u %s:%s -T %s/%s ftp://%s/",
								ftpUser.c_str(), 
                                ftpPwd.c_str(), 
                                this->_snapshotPath.c_str(), 
                                this->_snapshotName.c_str(), 
                                ftpIP.c_str());
}

size_t Snapshot::saveLocally(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written;
    written = fwrite(ptr, size, nmemb, stream);
    return written;
}

void Snapshot::deleteFromDisk(void) {
    std::string deleteCmd = Media::string_format("rm %s/%s", this->_snapshotPath.c_str(), this->_snapshotName.c_str());
    system(deleteCmd.c_str());
}

CURLcode Snapshot::download(const std::string downloadUri) {
	CURL *curl;
    FILE *fp;
    CURLcode res;
    char const *url = downloadUri.c_str();
    char outfilename[FILENAME_MAX];
    std::strcpy(outfilename, Media::string_format("%s/%s", this->_snapshotPath.c_str(), this->_snapshotName.c_str()).c_str());

    char error[CURL_ERROR_SIZE] = "";
    int timeoutSeconds = 10;
    curl = curl_easy_init();
    if (curl) {
        struct stat st = {0};
        if (stat(this->_snapshotPath.c_str(), &st) == -1) {
            mkdir(this->_snapshotPath.c_str(), 0700);
        }

        fp = fopen(outfilename,"wb");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER , error);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT , timeoutSeconds);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Snapshot::saveLocally);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        res = curl_easy_perform(curl);
        if(res) {
            printf("Error: %s\n", error);
        }
        curl_easy_cleanup(curl);
        fclose(fp);
    }   
    // else {
    //     return -1;
    // }
    return res;
}