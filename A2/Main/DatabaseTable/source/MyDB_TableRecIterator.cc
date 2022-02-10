
#ifndef TABLE_REC_ITER_C
#define TABLE_REC_ITER_C

#include "MyDB_TableRecIterator.h"

MyDB_TableRecIterator::MyDB_TableRecIterator(MyDB_RecordPtr rec, MyDB_TableReaderWriter* trw) {
    recordPtr = rec;
    index = -1;     // Not initialized yet
    tableReaderWriter = trw;
}

MyDB_RecordPtr MyDB_TableRecIterator::getRecord() {
    return pageRecIterator->getRecord();
}

bool MyDB_TableRecIterator::hasNext() {
    if (index == -1) {
        return (*tableReaderWriter)[0].getIterator(recordPtr)->hasNext(); 
    }

    return !(index == tableReaderWriter->getLast() && !pageRecIterator->hasNext());
}

void MyDB_TableRecIterator::getNext() {
    if (index == -1) {      // first initialize it
        pageRecIterator = dynamic_pointer_cast<MyDB_PageRecIterator>((*tableReaderWriter)[++index].getIterator(recordPtr));
    }

    if (!hasNext()) {
        cerr << "Fatal Error: There is no next record in table!" << endl;
    }

    while (!pageRecIterator->hasNext()) {
        pageRecIterator = dynamic_pointer_cast<MyDB_PageRecIterator>((*tableReaderWriter)[++index].getIterator(recordPtr));
    }

    pageRecIterator->getNext();
}

#endif