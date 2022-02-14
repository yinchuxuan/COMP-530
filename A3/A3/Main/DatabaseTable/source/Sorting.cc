
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

void mergeIntoFile (MyDB_TableReaderWriter &outTable, vector <MyDB_RecordIteratorAltPtr> &runIteratorList, function <bool ()> comparator, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs) {
	RecordComparator recordComparator(comparator, lhs, rhs);

	auto cmp = [&recordComparator](MyDB_RecordIteratorAltPtr& ptr1, MyDB_RecordIteratorAltPtr& ptr2) {
		return recordComparator(ptr1->getCurrentPointer(), ptr2->getCurrentPointer());
	};

	priority_queue<MyDB_RecordIteratorAltPtr, vector<MyDB_RecordIteratorAltPtr>, decltype(cmp)> minHeap(cmp);

	// get min record from each run
	while (!minHeap.empty()) {
		MyDB_RecordPtr tmpRecord = outTable.getEmptyRecord();
		MyDB_RecordIteratorAltPtr ptr = minHeap.top();
		minHeap.pop();
		ptr->getCurrent(tmpRecord);
		outTable.append(tmpRecord);

		if (ptr->advance()) {
			minHeap.push(ptr);
		}
	}
}

vector <MyDB_PageReaderWriter> mergeIntoList (MyDB_BufferManagerPtr bufferManager, MyDB_RecordIteratorAltPtr lhs, MyDB_RecordIteratorAltPtr rhs, function <bool ()> comparator,
	MyDB_RecordPtr leftRecordPtr, MyDB_RecordPtr rightRecordPtr) {
    vector<MyDB_PageReaderWriter> res;
    RecordComparator recordComparator(comparator, leftRecordPtr, rightRecordPtr);
    MyDB_PageReaderWriter* pageReaderWriter = new MyDB_PageReaderWriter(*bufferManager);
    while(true) {
        // get 2 rec iterator
        if (recordComparator(lhs->getCurrentPointer(), rhs->getCurrentPointer())) {
            lhs->getCurrent(leftRecordPtr);
            // check whether tmp page is full
            // write the current one
            if (!pageReaderWriter->append(leftRecordPtr)) {
                res.push_back(*pageReaderWriter);
                pageReaderWriter = new MyDB_PageReaderWriter(*bufferManager);
            }
            if (!lhs->advance()) {
                // append the rest of right ptr
                rhs->getCurrent(rightRecordPtr);
                while (rhs->advance()) {
                    if (!pageReaderWriter->append(rightRecordPtr)) {
                        res.push_back(*pageReaderWriter);
                        pageReaderWriter = new MyDB_PageReaderWriter(*bufferManager);
                    }
                    rhs->getCurrent(rightRecordPtr);
                }
                break;
            }
        } else {
            rhs->getCurrent(rightRecordPtr);
            if (!pageReaderWriter->append(rightRecordPtr)) {
                res.push_back(*pageReaderWriter);
                pageReaderWriter = new MyDB_PageReaderWriter(*bufferManager);
            }
            if (!rhs->advance()) {
                // append the rest of left iter
                lhs->getCurrent(leftRecordPtr);
                while (lhs->advance()) {
                    if (!pageReaderWriter->append(leftRecordPtr)) {
                        res.push_back(*pageReaderWriter);
                        pageReaderWriter = new MyDB_PageReaderWriter(*bufferManager);
                    }
                    lhs->getCurrent(leftRecordPtr);
                }
                break;
            }
        }
    }
    return res;
}

	
void sort (int runSize, MyDB_TableReaderWriter &inTable, MyDB_TableReaderWriter &outTable, function <bool ()> comparator, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs) {
	MyDB_BufferManagerPtr bufferManager = inTable.getBufferMgr();
	vector<vector<MyDB_PageReaderWriter>> sortedRunList;
	vector<MyDB_RecordIteratorAltPtr> runIteratorList;

	for (int i = 0; i < inTable.getNumPages(); i += runSize) {
		queue<vector<MyDB_PageReaderWriter>> pageListQueue;

		for (int j = 0; j < runSize; j++) {
			if (j >= inTable.getNumPages()) {
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

			pageListQueue.push(mergeIntoList(bufferManager, getIteratorAlt(firstList), getIteratorAlt(secondList), comparator, lhs, rhs));
		}

		sortedRunList.push_back(pageListQueue.front());
		runIteratorList.push_back(getIteratorAlt(pageListQueue.front()));
	}

	mergeIntoFile(outTable, runIteratorList, comparator, lhs, rhs);
}

#endif
