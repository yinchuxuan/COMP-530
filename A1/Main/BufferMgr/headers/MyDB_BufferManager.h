
#ifndef BUFFER_MGR_H
#define BUFFER_MGR_H

#include "MyDB_PageHandle.h"
#include "MyDB_Table.h"
#include "MyDB_PageIndexUtil.h"
#include "MyDB_FileIO.h"
#include <unordered_map>
#include <stack>
#include <iostream>

using namespace std;

class MyDB_PageHandleBase;
typedef shared_ptr <MyDB_PageHandleBase> MyDB_PageHandle;

// Buffer's status
// EMPTY: 				This buffer is not used
// UNPINNED:			This buffer is not pinned 
// PINNED:				This buffer is pinned in memory
enum MyDB_BufferStatus {
	EMPTY,
	UNPINNED,
	PINNED
};

// Page Type
// NORMAL:				This page is used by user
// ANONYMOUS:			This page is used as temporary page
enum MyDB_PageType {
	NORMAL,
	ANONYMOUS
};

struct MyDB_Page;

// Buffer node's definition
// This supports a double linked structure
struct MyDB_BufferNode {
	char* buffer;
	MyDB_BufferNode* prev;
	MyDB_BufferNode* next;
	MyDB_Page* page;
	MyDB_BufferStatus status;

	MyDB_BufferNode(size_t pageSize):status(EMPTY) {
		buffer = static_cast<char*>(malloc(pageSize));
	}

	~MyDB_BufferNode() {
		free(static_cast<char*>(buffer));
	}
};

// Page's definition
struct MyDB_Page {
	MyDB_TablePtr table;
	long index;
	MyDB_PageType type;
	bool isWritten;
	int handleRefrence;
	MyDB_BufferNode* bufferNode;

	MyDB_Page(MyDB_TablePtr tbl, long i, MyDB_PageType pageType):table(tbl), index(i), type(pageType), isWritten(false), handleRefrence(0), bufferNode(nullptr) {}

	MyDB_Page(long i, MyDB_PageType pageType):table(nullptr), index(i), type(pageType), isWritten(false), handleRefrence(0), bufferNode(nullptr) {}

};

class MyDB_BufferManager {

public:

	// THESE METHODS MUST APPEAR AND THE PROTOTYPES CANNOT CHANGE!

	// gets the i^th page in the table whichTable... note that if the page
	// is currently being used (that is, the page is current buffered) a handle 
	// to that already-buffered page should be returned
	MyDB_PageHandle getPage (MyDB_TablePtr whichTable, long i);

	// gets a temporary page that will no longer exist (1) after the buffer manager
	// has been destroyed, or (2) there are no more references to it anywhere in the
	// program.  Typically such a temporary page will be used as buffer memory.
	// since it is just a temp page, it is not associated with any particular 
	// table
	MyDB_PageHandle getPage ();

	// gets the i^th page in the table whichTable... the only difference 
	// between this method and getPage (whicTable, i) is that the page will be 
	// pinned in RAM; it cannot be written out to the file
	MyDB_PageHandle getPinnedPage (MyDB_TablePtr whichTable, long i);

	// gets a temporary page, like getPage (), except that this one is pinned
	MyDB_PageHandle getPinnedPage ();

	// un-pins the specified page
	void unpin (MyDB_PageHandle unpinMe);

	// creates an LRU buffer manager... params are as follows:
	// 1) the size of each page is pageSize 
	// 2) the number of pages managed by the buffer manager is numPages;
	// 3) temporary pages are written to the file tempFile
	MyDB_BufferManager (size_t pageSize, size_t numPages, string tempFile);
	
	// when the buffer manager is destroyed, all of the dirty pages need to be
	// written back to disk, any necessary data needs to be written to the catalog,
	// and any temporary files need to be deleted
	~MyDB_BufferManager ();

	// FEEL FREE TO ADD ADDITIONAL PUBLIC METHODS 

private:

	unordered_map<MyDB_PageKey, MyDB_Page*> pageHashMap;

	stack<MyDB_BufferNode*> emptyBufferNodes;
	
	MyDB_BufferNode* bufferListHead;
	
	MyDB_BufferNode* bufferListTail;

	size_t pageSize;

	size_t numPages;

	string tempFile;

	long anonymous_index;

	void* getBuffer(MyDB_PageKey pageKey);

	void writeBuffer(MyDB_PageKey pageKey);

	void decreaseReference(MyDB_PageKey pageKey);

	void pinBufferNode(MyDB_BufferNode* bufferNode);

	void unpinBufferNode(MyDB_BufferNode* bufferNode);

	void loadPage(MyDB_Page* page);

	void evictPage(MyDB_Page* page);

	friend class MyDB_PageHandleBase;
};

#endif


