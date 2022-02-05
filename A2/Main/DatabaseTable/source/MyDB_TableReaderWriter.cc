
#ifndef TABLE_RW_C
#define TABLE_RW_C

#include <fstream>
#include "MyDB_PageReaderWriter.h"
#include "MyDB_TableReaderWriter.h"

using namespace std;

MyDB_TableReaderWriter :: MyDB_TableReaderWriter (MyDB_TablePtr tbl, MyDB_BufferManagerPtr bufManager) {
	table = tbl;
	bufferManager = bufManager;
}

MyDB_PageReaderWriter MyDB_TableReaderWriter :: operator [] (size_t index) {
	return MyDB_PageReaderWriter(table, index, bufferManager->getPageSize(), bufferManager);	
}

MyDB_RecordPtr MyDB_TableReaderWriter :: getEmptyRecord () {
	return make_shared<MyDB_Record>(table->getSchema());
}

MyDB_PageReaderWriter MyDB_TableReaderWriter :: last () {
	// What if the page's last is -1?
	return MyDB_PageReaderWriter(table, table->lastPage(), bufferManager->getPageSize(), bufferManager);	
}

void MyDB_TableReaderWriter :: append (MyDB_RecordPtr record) {
	if (record->getBinarySize() > bufferManager->getPageSize()) {
		cerr << "MyDB_TableReaderWriter :: append: Record size is greater than buffer size. Cannot insert record!" << endl;
	}

	MyDB_PageReaderWriter lastPageReaderWriter = last();

	if (!lastPageReaderWriter.append(record)) {		// need to append a new page
		lastPageReaderWriter = MyDB_PageReaderWriter(table, table->lastPage() + 1, bufferManager->getPageSize(), bufferManager);
		lastPageReaderWriter.setType(RegularPage);		// Is this reasonable?
		lastPageReaderWriter.append(record);
		table->setLastPage(table->lastPage() + 1);		// Increment last page's index
	}
}

void MyDB_TableReaderWriter :: loadFromTextFile (string text) {
	// Behavior of this method?
	int pageSize = bufferManager->getPageSize();
	int pageIndex = 0;

`	// First clear all pages in this table
	for (int i = 0; i < table->lastPage) {
		(*this)[i].clear();
	}

	for (int i = 0; i < text.size(); i += bufferManager->getPageSize() / 2) {
		MyDB_RecordPtr tmpRecord = make_shared<MyDB_Record>(table->getSchema());
		tmpRecord->fromText(text.substr(i, min(pageSize / 2, text.Size() - i));		// Size of each record?

		if (i <= table->lastPage()) {	// If page already exists, append to it
			(*this)[i].append(tmpRecord);
		} else {						// else append to the table
			append(tmprecord);
		}
	}
}

MyDB_RecordIteratorPtr MyDB_TableReaderWriter :: getIterator (MyDB_RecordPtr record) {
	return MyDB_TableRecIterator(record, this);
}

void MyDB_TableReaderWriter :: writeIntoTextFile (string text) {
	MyDB_TableRecIterator iterator = dynamic_cast<MyDB_TableRecIterator>(getIterator());

	while (1) {
		MyDB_RecordPtr tmpRecord = iterator.getRecord();
		char buffer[tmpRecord->getBinarySize()];
		tmpRecord->toBinary(buffer);
		text = text + string(buffer);

		if (!iterator->hasNext()) {
			break;
		}

		iterator->getNext();
	}
}

#endif

