
#ifndef SORT_C
#define SORT_C

#include "MyDB_PageReaderWriter.h"
#include "MyDB_TableRecIterator.h"
#include "MyDB_TableRecIteratorAlt.h"
#include "MyDB_TableReaderWriter.h"
#include "Sorting.h"
#include "RecordComparator.h"
#include <queue>
#include "MyDB_PageReaderWriter.h"

using namespace std;

int getPageListSize(MyDB_RecordIteratorAltPtr ptr, MyDB_RecordPtr record) {
	int size = 0;
	ptr->getCurrent(record);
	size++;

	while (ptr->advance()) {
		ptr->getCurrent(record);
		size++;
	}

	return size;
}

void mergeIntoFile (MyDB_TableReaderWriter &outTable, vector <MyDB_RecordIteratorAltPtr> &runIteratorList, function <bool ()> comparator, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs) {
	RecordComparator recordComparator(comparator, lhs, rhs);
	MyDB_RecordPtr tmpRecord = outTable.getEmptyRecord();

	auto cmp = [&recordComparator](MyDB_RecordIteratorAltPtr& ptr1, MyDB_RecordIteratorAltPtr& ptr2) {
		return !recordComparator(ptr1->getCurrentPointer(), ptr2->getCurrentPointer());
	};

	priority_queue<MyDB_RecordIteratorAltPtr, vector<MyDB_RecordIteratorAltPtr>, decltype(cmp)> minHeap(cmp);

	for (auto iterator : runIteratorList) {
		minHeap.push(iterator);
	}

	// get min record from each run
	while (!minHeap.empty()) {
		MyDB_RecordIteratorAltPtr ptr = minHeap.top();
		minHeap.pop();
		ptr->getCurrent(tmpRecord);
		outTable.append(tmpRecord);

		if (ptr->advance()) {
			minHeap.push(ptr);
		}
	}
}

// check whether tmp page is full
// write the current one
inline void appendRecord(MyDB_PageReaderWriter** prw, MyDB_RecordPtr record, vector<MyDB_PageReaderWriter>& res, MyDB_BufferManagerPtr bufferManager) {
	if (record->getBinarySize() > bufferManager->getPageSize()) {
		cerr << "Error: the record size is greater then buffer size!" << endl;
	}

	while (!(*prw)->append(record)) {
		res.push_back(**prw);
		*prw = new MyDB_PageReaderWriter(*bufferManager);
	}
}

vector <MyDB_PageReaderWriter> mergeIntoList (MyDB_BufferManagerPtr bufferManager, MyDB_RecordIteratorAltPtr lhs, MyDB_RecordIteratorAltPtr rhs, function <bool ()> comparator,
	MyDB_RecordPtr leftRecordPtr, MyDB_RecordPtr rightRecordPtr) {
    vector<MyDB_PageReaderWriter> res;
    RecordComparator recordComparator(comparator, leftRecordPtr, rightRecordPtr);
    MyDB_PageReaderWriter* pageReaderWriter = new MyDB_PageReaderWriter(*bufferManager);
	int index = 0;

    while(true) {
        // get 2 rec iterator
        if (recordComparator(lhs->getCurrentPointer(), rhs->getCurrentPointer())) {
            lhs->getCurrent(leftRecordPtr);
			appendRecord(&pageReaderWriter, leftRecordPtr, res, bufferManager);
			index++;

            if (!lhs->advance()) {
                // append the rest of right ptr
                rhs->getCurrent(rightRecordPtr);
				appendRecord(&pageReaderWriter, rightRecordPtr, res, bufferManager);
				index++;

                while (rhs->advance()) {
                    rhs->getCurrent(rightRecordPtr);
                    appendRecord(&pageReaderWriter, rightRecordPtr, res, bufferManager);
					index++;
                }

                break;
            }
        } else {
            rhs->getCurrent(rightRecordPtr);
			appendRecord(&pageReaderWriter, rightRecordPtr, res, bufferManager);
			index++;

            if (!rhs->advance()) {
                // append the rest of left iter
                lhs->getCurrent(leftRecordPtr);
				appendRecord(&pageReaderWriter, leftRecordPtr, res, bufferManager);
				index++;

                while (lhs->advance()) {
                    lhs->getCurrent(leftRecordPtr);
					appendRecord(&pageReaderWriter, leftRecordPtr, res, bufferManager);
					index++;
                }

                break;
            }
        }
    }

	res.push_back(*pageReaderWriter);	// should add the last page!
	
    return res;
}
	
void sort (int runSize, MyDB_TableReaderWriter &inTable, MyDB_TableReaderWriter &outTable, function <bool ()> comparator, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs) {
	MyDB_BufferManagerPtr bufferManager = inTable.getBufferMgr();
	vector<vector<MyDB_PageReaderWriter>> sortedRunList;
	vector<MyDB_RecordIteratorAltPtr> runIteratorList;

	for (int i = 0; i < inTable.getNumPages(); i += runSize) {
		queue<vector<MyDB_PageReaderWriter>> pageListQueue;

		for (int j = 0; j < runSize; j++) {
			if (i + j >= inTable.getNumPages()) {
				break;
			}

			// sort each page at first
			vector<MyDB_PageReaderWriter> tmpPageList({*(inTable[i + j].sort(comparator, lhs, rhs))});

			// push each page list in queue (until now each contains a single page)
			pageListQueue.push(tmpPageList);
		}

		// repeatedly call merge into list until there is only one page list in queue
		while (pageListQueue.size() > 1) {
			vector<MyDB_PageReaderWriter> firstList = pageListQueue.front();
			pageListQueue.pop();
			vector<MyDB_PageReaderWriter> secondList = pageListQueue.front();
			pageListQueue.pop();

			vector<MyDB_PageReaderWriter> result = mergeIntoList(bufferManager, getIteratorAlt(firstList), getIteratorAlt(secondList), comparator, lhs, rhs);


			MyDB_RecordIteratorAltPtr ptr1 = getIteratorAlt(firstList);
			MyDB_RecordIteratorAltPtr ptr2 = getIteratorAlt(secondList);
			MyDB_RecordIteratorAltPtr ptr3 = getIteratorAlt(result);
			MyDB_RecordPtr tmpRecord = inTable.getEmptyRecord();

			pageListQueue.push(result);
		}

		sortedRunList.push_back(pageListQueue.front());
		runIteratorList.push_back(getIteratorAlt(pageListQueue.front()));
	}

	mergeIntoFile(outTable, runIteratorList, comparator, lhs, rhs);
}

#endif
