
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
	if (index > table->lastPage()) {	// require uninitialized page, initialize them first
		for (int i = table->lastPage() + 1; i <= index; i++) {
			MyDB_PageReaderWriter rw = MyDB_PageReaderWriter(table, i, bufferManager->getPageSize(), bufferManager);
			rw.setType(RegularPage);		
		}

		table->setLastPage(index);
	}

	return MyDB_PageReaderWriter(table, index, bufferManager->getPageSize(), bufferManager);	
}

MyDB_RecordPtr MyDB_TableReaderWriter :: getEmptyRecord () {
	return make_shared<MyDB_Record>(table->getSchema());
}

MyDB_PageReaderWriter MyDB_TableReaderWriter :: last () {
	if (table->lastPage() == -1) {		// this table has never been writen, return the first empty page
		MyDB_PageReaderWriter rw = MyDB_PageReaderWriter(table, 0, bufferManager->getPageSize(), bufferManager);
		rw.setType(RegularPage);
		table->setLastPage(0);
		return rw;
	}

	return MyDB_PageReaderWriter(table, table->lastPage(), bufferManager->getPageSize(), bufferManager);	
}

void MyDB_TableReaderWriter :: append (MyDB_RecordPtr record) {
	if (record->getBinarySize() > bufferManager->getPageSize()) {
		cerr << "MyDB_TableReaderWriter :: append: Record size is greater than buffer size. Cannot insert record!" << endl;
	}

	MyDB_PageReaderWriter lastPageReaderWriter = last();

	if (!lastPageReaderWriter.append(record)) {		// need to append a new page
		lastPageReaderWriter = MyDB_PageReaderWriter(table, table->lastPage() + 1, bufferManager->getPageSize(), bufferManager);
		lastPageReaderWriter.setType(RegularPage);		// append regular page by default 
		lastPageReaderWriter.append(record);
		table->setLastPage(table->lastPage() + 1);		// increment last page's index
	}
}

void MyDB_TableReaderWriter :: loadFromTextFile (string text) {
	fstream textFile(text);
	string recordContent;

	while (getline(textFile, recordContent)) {
		if (recordContent.size() > bufferManager->getPageSize()) {
			// Fatal Error: the record size is greater than the page size
			cerr << "Fatal Error: the record size is greater than the page size" << endl;
		}

		MyDB_RecordPtr tmpRecord = make_shared<MyDB_Record>(table->getSchema());
		tmpRecord->fromText(recordContent);
		append(tmpRecord);
	}

	textFile.close();
}

MyDB_RecordIteratorPtr MyDB_TableReaderWriter :: getIterator (MyDB_RecordPtr record) {
	return MyDB_TableRecIterator(record, this);
}

void MyDB_TableReaderWriter :: writeIntoTextFile (string text) {
	fstream testFile(text);
	MyDB_TableRecIterator iterator = dynamic_cast<MyDB_TableRecIterator>(getIterator());

	while (1) {
		MyDB_RecordPtr tmpRecord = iterator.getRecord();
		char buffer[tmpRecord->getBinarySize()];
		tmpRecord->toBinary(buffer);
		textFile << string(buffer) << endl;

		if (!iterator->hasNext()) {
			break;
		}

		iterator->getNext();
	}

	textFile.close();
}

int MyDB_TableReaderWriter::getLast() {
    return table->lastPage();
}

#endif