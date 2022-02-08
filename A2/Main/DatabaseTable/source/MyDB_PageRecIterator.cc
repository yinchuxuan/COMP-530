
#ifndef PAGE_REC_ITER_C
#define PAGE_REC_ITER_C
#include "MyDB_PageRecIterator.h"

MyDB_PageRecIterator ::MyDB_PageRecIterator(MyDB_RecordPtr rec, struct MyDB_PageHeader* header) {
    recordPtr = rec;
    pageHeader = header;
    startPos = pageHeader->records;
}

MyDB_RecordPtr MyDB_PageRecIterator::getRecord() {
    return recordPtr;
}

void MyDB_PageRecIterator::getNext() {
    startPos = recordPtr->fromBinary(startPos);
}

bool MyDB_PageRecIterator::hasNext() {
    size_t end = pageHeader->offSetToPageEnd;
    return pageHeader->records + end > startPos;
}
#endif