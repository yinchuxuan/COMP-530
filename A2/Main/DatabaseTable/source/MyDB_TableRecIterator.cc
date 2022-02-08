
#ifndef TABLE_REC_ITER_C
#define TABLE_REC_ITER_C

#include "MyDB_TableRecIterator.h"


MyDB_TableRecIterator::MyDB_TableRecIterator(MyDB_RecordPtr rec, MyDB_TableReaderWriterPtr trw) {
    recordPtr = rec;
    index = 0;
    tableReaderWriter = trw;
    pageRecIterator = dynamic_cast<MyDB_PageRecIterator*>((*trw)[index]);
}

MyDB_RecordPtr MyDB_TableRecIterator::getRecord() {
    if (hasNext()) {
        return pageRecIterator->getRecord();
    } else {
        return nullptr;
    }
}

bool MyDB_TableRecIterator::hasNext() {
    return index <= tableReaderWriter->getLast() || pageRecIterator->hasNext();
}

void MyDB_TableRecIterator::getNext() {
    if (pageRecIterator -> hasNext()) {
        pageRecIterator->getNext();
    } else {
        index ++;
        // TODO: should move to next page and refresh the pageRecIterator
    }
}


#endif