
#ifndef FILE_IO_C
#define FILE_IO_C

#include <string>
#include <stdio.h>
#include <iostream>
#include "MyDB_FileIO.h"
#include <unistd.h>
#include <fcntl.h>

using namespace std;

size_t readPage(const string filename, const size_t offset, const size_t pageSize, char* buffer) {
    if (buffer == nullptr) {
        return 0;
    }
    int fd;
    int readBytes;
    fd = open(filename.c_str() , O_RDONLY | O_CREAT, 0666);
    if (fd == -1) {
        printf("failed to open file...\n");
        return 0;
    }
    if (lseek(fd, offset, SEEK_SET) == -1 ) {
        printf("read offset error..\n");
        return 0;
    }
    readBytes = read(fd, buffer, pageSize);
//    for(int i = 0; i < readBytes; i++) {
//        cout << buffer[i];
//    }
    close(fd);
    return readBytes;
}

void writePage(const string filename, const size_t offset, const size_t pageSize, const char* buffer) {
    if (buffer == nullptr) {
        return;
    }
    int fd;
    fd = open(filename.c_str(), O_RDWR | O_FSYNC);
    if (fd == -1) {
        printf("file not exists...\n");
        return;
    }
    if (lseek(fd, offset, SEEK_SET) == -1 ) {
        printf("read offset error...\n");
        return;
    }
    write(fd, buffer, pageSize);
    close(fd);
    return;
}

//int main (int argc, char *argv[]) {
//    char buffer[1024];
//    int read;
//    string filename ="/Users/duoxu/Desktop/Spring 2022/comp530/COMP-530/A1/Main/BufferMgr/source/test";
////    read = readPage(filename, 2, 5, buffer);
//    char drawback[] = "Test write \n";
//    writePage("/Users/duoxu/Desktop/Spring 2022/comp530/COMP-530/A1/Main/BufferMgr/source/test", 4, strlen(drawback), drawback);
//    return 0;
//}


#endif