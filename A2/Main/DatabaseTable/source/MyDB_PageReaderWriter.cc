
#ifndef PAGE_RW_C
#define PAGE_RW_C

#include "MyDB_PageReaderWriter.h"
#include <cstring>

void MyDB_PageReaderWriter :: clear () {
	memset(pageHandle->getBytes(), 0, pageSize);
	setType(RegularPage);	// Set the page type regular
	pageHandle->wroteBytes();
}

MyDB_PageType MyDB_PageReaderWriter :: getType () {
	return ((struct MyDB_PageHeader*)pageHandle->getBytes())->pageType;
}

MyDB_RecordIteratorPtr MyDB_PageReaderWriter :: getIterator (MyDB_RecordPtr record) {
	// Constructor for page record iter
	MyDB_PageHeader* pageHeader = (struct MyDB_PageHeader*)pageHandle->getBytes();
	shared_ptr<MyDB_PageRecIterator> pageRecIterator = make_shared<MyDB_PageRecIterator>(record, pageHeader);

	return pageRecIterator;
}

void MyDB_PageReaderWriter :: setType (MyDB_PageType type) {
	MyDB_PageHeader* pageHeader = (struct MyDB_PageHeader*)pageHandle->getBytes();
	pageHeader->pageType = type; 
	pageHandle->wroteBytes();
}

bool MyDB_PageReaderWriter :: append (MyDB_RecordPtr record) {
	MyDB_PageHeader* pageHeader = (struct MyDB_PageHeader*)pageHandle->getBytes();
	void* tmp = (void*)((char*)pageHeader->records + static_cast<int>(pageHeader->offSetToPageEnd));
	void* pageTail = tmp;

	if ((size_t)pageTail + record->getBinarySize() >= (size_t)pageHandle->getBytes() + pageSize) {
		return false;	// page out of memory
	}

	// write content to the tail of this page
	// update offset to page end
	pageHeader->offSetToPageEnd = (size_t)record->toBinary(pageTail) - (size_t)pageHeader->records;
	pageHandle->wroteBytes();

	return true;
}

MyDB_PageReaderWriter:: MyDB_PageReaderWriter(MyDB_TablePtr tbl, long i, size_t size, MyDB_BufferManagerPtr bufManager) {
	table = tbl;
	index = i;
	bufferManager = bufManager;
	pageHandle = bufferManager->getPinnedPage(table, index);	// This page will automatically unpin when no handle referencing it
	pageSize = size;
}

void MyDB_PageReaderWriter:: initializePage() {
	memset(pageHandle->getBytes(), 0, pageSize);
	setType(RegularPage);		// Set regular page by default
	pageHandle->wroteBytes();
}

#endif
