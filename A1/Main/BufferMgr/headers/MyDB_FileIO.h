
#ifndef FILE_IO_H
#define FILE_IO_H

#include <string>

using namespace std;

// Read page content from table file
// Read content in [offset, offset + pageSize) of the table file and fill it in the buffer
// If there are no more content in the slot, fill the remaining space with 0
// Return how many bytes it actually reads 
size_t readPage(const string filename, const size_t offset, const size_t pageSize, char* buffer);

// Write back page content to file
// Write the content in buffer to the slot [offset, offset + pageSize)
void writePage(const string filename, const size_t offset, const size_t pageSize, const char* buffer);

#endif