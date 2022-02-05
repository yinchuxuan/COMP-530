
#ifndef PAGE_RW_C
#define PAGE_RW_C

#include "MyDB_PageReaderWriter.h"

void MyDB_PageReaderWriter :: clear () {
	memset(pageAddress, 0, pageSize);
	setType(RegularPage);	// Set the page type regular
}

MyDB_PageType MyDB_PageReaderWriter :: getType () {
	return ((struct MyDB_PageHeader*)pageHandle->getBytes())->pageType;
}

MyDB_RecordIteratorPtr MyDB_PageReaderWriter :: getIterator (MyDB_RecordPtr record) {
	// Constructor for page record iter
	MyDB_PageHeader* pageHeader = (struct MyDB_PageHeader*)pageHandle->getBytes();
	MyDB_PageRecIterator pageRecIterator = make_shared<MyDB_PageRecIterator>(record, pageHeader);

	return pageRecIterator;
}

void MyDB_PageReaderWriter :: setType (MyDB_PageType type) {
	MyDB_PageHeader* pageHeader = (struct MyDB_PageHeader*)pageHandle->getBytes();
	pageHeader->pageType = type; 
}

bool MyDB_PageReaderWriter :: append (MyDB_RecordPtr record) {
	MyDB_PageHeader* pageHeader = (struct MyDB_PageHeader*)pageHandle->getBytes();
	void* pageTail = (void*)((uint32_t)pageHeader->records + pageHeader->offSetToPageEnd);

	if (((uint32_t)pageTail) + record->getBinarySize > (uint32_t)pageHandle->getBytes() + pageSize) {
		return false;	// page out of memory
	}

	// write content to the tail of this page
	// update offset to page end
	pageHeader->offSetToPageEnd = (size_t)(record->toBinary(pageTail) - pageHeader->records);

	return true;
}

MyDB_PageReaderWriter:: MyDB_PageReaderWriter(MyDB_TablePtr tbl, long i, size_t size, MyDB_BufferManagerPtr bufManager) {
	table = tbl;
	index = i;
	bufferManager = bufManager;
	pageHandle = bufferManager->getPinnedPage(table, index);
	pageSize = size;
}

#endif
