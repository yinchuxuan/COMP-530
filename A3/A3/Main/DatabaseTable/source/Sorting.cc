
#ifndef SORT_C
#define SORT_C

#include "MyDB_PageReaderWriter.h"
#include "MyDB_TableRecIterator.h"
#include "MyDB_TableRecIteratorAlt.h"
#include "MyDB_TableReaderWriter.h"
#include "Sorting.h"

using namespace std;

void mergeIntoFile (MyDB_TableReaderWriter &outTable, vector <MyDB_RecordIteratorAltPtr> &runIteratorList, function <bool ()> comparator, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs) {
	RecordComparator recordComparator(comparator, lhs, rhs);

	auto cmp = [](MyDB_RecordIteratorAltPtr& ptr1, MyDB_RecordIteratorAltPtr& ptr2) {
		return RecordComparator(ptr1->getCurrentPointer(), ptr2->getCurrentPointer);
	}

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

vector <MyDB_PageReaderWriter> mergeIntoList (MyDB_BufferManagerPtr, MyDB_RecordIteratorAltPtr, MyDB_RecordIteratorAltPtr, function <bool ()>, 
	MyDB_RecordPtr, MyDB_RecordPtr) {return vector <MyDB_PageReaderWriter> (); } 
	
void sort (int runSize, MyDB_TableReaderWriter &inTable, MyDB_TableReaderWriter &outTable, function <bool ()> comparator, MyDB_RecordPtr lhs, MyDB_RecordPtr rhs) {
	MyDB_BufferManagerPtr bufferManager = inTable.getBufferMgr();
	vector<vector<MyDB_PageReaderWriter>> sortedRunList;
	vector<MyDB_RecordIteratorAltPtrer> runIteratorList;

	for (int i = 0; i < inTable.getNumPages(); i += runSize) {
		queue<vector<MyDB_PageReaderWriter> pageListQueue;

		for (int j = 0; j < runSize; j++) {
			if (j >= inTable.inTable.getNumPages()) {
				break;
			}

			// sort each page at first
			vector<MyDB_PageReaderWriter> tmpPageList(move(*(inTable[i + j].sort(comparator, lhs, rhs))));

			// push each page list in queue (until now each contains a single page)
			pageListQueue.push_back(tmpPageList);
		}

		// repeatedly call merge into list until there is only one page list in queue
		while (queue.size() > 1) {
			vector<MyDB_PageReaderWriter> firstList = queue.top();
			queue.pop();
			vector<MyDB_PageReaderWriter> secondList = queue.top();
			queue.pop();

			queue.push(bufferManager, mergeIntoList(firstList[0].getIteratorAlt(firstList), firstList[0].getIteratorAlt(secondList)), lhs, rhs);
		}

		sortedRunList.push_back(queue.top());
		runIteratorList.push_back(queue.top()[0].getIterator.getIteratorAlt(queue.top()));
	}

	mergeIntoFile(outTable, runIteratorList, comparator, lhs, rhs);
} 

#endif
