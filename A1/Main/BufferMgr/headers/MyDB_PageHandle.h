
#ifndef PAGE_HANDLE_H
#define PAGE_HANDLE_H

#include <memory>
#include "MyDB_Table.h"
#include "MyDB_PageIndexUtil.h"
#include "MyDB_BufferManager.h"

// page handles are basically smart pointers
using namespace std;
class MyDB_PageHandleBase;
class MyDB_BufferManager;
typedef shared_ptr <MyDB_PageHandleBase> MyDB_PageHandle;

class MyDB_PageHandleBase {

friend class MyDB_BufferManager;

public:

	// THESE METHODS MUST BE IMPLEMENTED WITHOUT CHANGING THE DEFINITION

	// access the raw bytes in this page... if the page is not currently
	// in the buffer, then the contents of the page are loaded from 
	// secondary storage. 
	void *getBytes ();

	// let the page know that we have written to the bytes.  Must always
	// be called once the page's bytes have been written.  If this is not
	// called, then the page will never be marked as dirty, and the page
	// will never be written to disk. 
	void wroteBytes ();

	// There are no more references to the handle when this is called...
	// this should decrmeent a reference count to the number of handles
	// to the particular page that it references.  If the number of 
	// references to a pinned page goes down to zero, then the page should
	// become unpinned.  
	~MyDB_PageHandleBase ();

	// Initialize page handle base
	// This method should only be called by MyDB_BufferManager
	MyDB_PageHandleBase(MyDB_PageKey key, MyDB_BufferManager* manager):pageKey(key), managerInstance(manager) {}

private:

	MyDB_PageKey pageKey;

	MyDB_BufferManager* managerInstance;

};

#endif

