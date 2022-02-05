
#ifndef PAGE_REC_ITER_H
#define PAGE_REC_ITER_H

#include "MyDB_RecordIterator.h"
#include "MyDB_PageReaderWriter.h"
#include "MyDB_Record.h"


class MyDB_PageRecIterator: public MyDB_RecordIterator {
    
public:

    // Constructor with rec pointer and table reader writer pointer 
    // Record is an injected object, table reader writer pointer is used to refer
    // itself
    MyDB_PageRecIterator(MyDB_RecordPtr rec, MyDB_TableReaderWriterPtr trw);

    // Get current record object
    MyDB_RecordPtr getRecord();
};

#endif