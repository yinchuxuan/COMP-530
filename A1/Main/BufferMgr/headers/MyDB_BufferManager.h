
#ifndef BUFFER_MGR_H
#define BUFFER_MGR_H

#include "MyDB_PageHandle.h"
#include "MyDB_Table.h"

using namespace std;

// Page node's status
// EMPTY: 				This page is not used
// NORMAL:				This page is used as normal page
// ANONYMOUS:			This page is used as anonymous page
// NORMAL_PINNNED:		This page is used as normal page and is pinned
// ANONYMOUS_PINNED: 	This page is user as anonymous page and is pinned 
enum Status {
	EMPTY,
	NORMAL,
	ANONYMOUS,
	NORMAL_PINNED,
	ANONYMOUS_PINNED
};

// Page node's definition
// This supports a double linked list structure
struct MyDB_PageNode {
	MyDB_TablePtr table;
	long index;
	Status status;
	bool isWritten;
	char* pageBuffer;
	MyDB_PageNode* prev;
	MyDB_PageNode* next;

	MyDB_PageNode():status(EMPTY), isWritten(false) {} 
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

	// YOUR STUFF HERE

};

#endif


