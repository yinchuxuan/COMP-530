
#ifndef PAGE_REC_ITER_H
#define PAGE_REC_ITER_H

#include "MyDB_RecordIterator.h"
#include "MyDB_PageReaderWriter.h"
#include "MyDB_Record.h"


class MyDB_PageRecIterator: public MyDB_RecordIterator {
    
public:

    // Constructor with rec pointer and page header
    // Record is an injected object, page header is used for locating page address
    // and getting offset to the end
    MyDB_PageRecIterator(MyDB_RecordPtr rec, struct MyDB_PageHeader* header);

    // Get current record object
    MyDB_RecordPtr getRecord();
};

#endif