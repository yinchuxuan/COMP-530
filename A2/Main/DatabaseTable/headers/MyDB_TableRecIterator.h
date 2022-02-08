
#ifndef TABLE_REC_ITER_H
#define TABLE_REC_ITER_H

#include "MyDB_RecordIterator.h"
#include "MyDB_PageReaderWriter.h"
#include "MyDB_Record.h"
#include "MyDB_PageRecIterator.h"


class MyDB_TableRecIterator: public MyDB_RecordIterator {
private:
    MyDB_RecordPtr recordPtr;
    MyDB_TableReaderWriterPtr tableReaderWriter;
    MyDB_PageRecIterator* pageRecIterator;
    int index;
public:

    // Constructor with rec pointer and table reader writer pointer 
    // Record is an injected object, table reader writer pointer is used to refer
    // itself
    MyDB_TableRecIterator(MyDB_RecordPtr rec, MyDB_TableReaderWriterPtr trw);

    // Get current record object
    MyDB_RecordPtr getRecord();

    bool hasNext() override;

    void getNext() override;
};

#endif