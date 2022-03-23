
#ifndef SORTMERGE_CC
#define SORTMERGE_CC

#include "Aggregate.h"
#include "MyDB_Record.h"
#include "MyDB_PageReaderWriter.h"
#include "MyDB_TableReaderWriter.h"
#include "SortMergeJoin.h"
#include "Sorting.h"

SortMergeJoin :: SortMergeJoin (MyDB_TableReaderWriterPtr leftInputIn, MyDB_TableReaderWriterPtr rightInputIn,
                MyDB_TableReaderWriterPtr outputIn, string finalSelectionPredicateIn, 
                vector <string> projectionsIn, pair <string, string> equalityChecksIn, string leftSelectionPredicateIn,
                string rightSelectionPredicateIn) {
    output = outputIn;
    finalSelectionPredicate = finalSelectionPredicateIn;
    projections = projectionsIn;
    equalityCheck = equalityChecksIn;
    leftTable = leftInputIn;
    rightTable = rightInputIn;
    leftSelectionPredicate = leftSelectionPredicateIn;
    rightSelectionPredicate = rightSelectionPredicateIn;
    runSize = leftTable->getBufferMgr()->numPages / 2;
}

void SortMergeJoin::sortIntoRuns(vector<vector<MyDB_PageReaderWriter>>& runs, vector<MyDB_RecordIteratorAltPtr>& runIterators, string side) {
	MyDB_RecordPtr leftCompareRec;
    MyDB_RecordPtr rightCompareRec;
    MyDB_TableReaderWriterPtr table;
    func leftCompareEquality;
    func rightCompareEquality;

    if (side == "left") {
        table = leftTable;
	    leftCompareRec = table->getEmptyRecord ();
        rightCompareRec = table->getEmptyRecord();

        leftCompareEquality = leftCompareRec->compileComputation(equalityCheck.first);
        rightCompareEquality = rightCompareRec->compileComputation(equalityCheck.first);
    } else {
        table = rightTable;
	    leftCompareRec = table->getEmptyRecord ();
        rightCompareRec = table->getEmptyRecord();

        leftCompareEquality = leftCompareRec->compileComputation(equalityCheck.second);
        rightCompareEquality = rightCompareRec->compileComputation(equalityCheck.second);
    }

    auto comparator = [&leftCompareEquality, &rightCompareEquality]() {
        return leftCompareEquality()->hash() < rightCompareEquality()->hash(); 
    };

    MyDB_BufferManagerPtr bufferManager = table->getBufferMgr();
	vector<vector<MyDB_PageReaderWriter>> sortedRunList;
	vector<MyDB_RecordIteratorAltPtr> runIteratorList;

	for (int i = 0; i < table->getNumPages(); i += runSize) {
		queue<vector<MyDB_PageReaderWriter>> pageListQueue;

		for (int j = 0; j < runSize; j++) {
			if (i + j >= table->getNumPages()) {
				break;
			}

			// sort each page at first
			vector<MyDB_PageReaderWriter> tmpPageList({*((*table)[i + j].sort(comparator, leftCompareRec, rightCompareRec))});

			// push each page list in queue (until now each contains a single page)
			pageListQueue.push(tmpPageList);
		}

		// repeatedly call merge into list until there is only one page list in queue
		while (pageListQueue.size() > 1) {
			vector<MyDB_PageReaderWriter> firstList = pageListQueue.front();
			pageListQueue.pop();
			vector<MyDB_PageReaderWriter> secondList = pageListQueue.front();
			pageListQueue.pop();

			vector<MyDB_PageReaderWriter> result = mergeIntoList(bufferManager, getIteratorAlt(firstList), getIteratorAlt(secondList), comparator, leftCompareRec, rightCompareRec);
			pageListQueue.push(result);
		}

		runs.push_back(pageListQueue.front());
		runIterators.push_back(getIteratorAlt(pageListQueue.front()));
	}
}

void SortMergeJoin :: run () {
    vector<vector<MyDB_PageReaderWriter>> leftRuns;
    vector<vector<MyDB_PageReaderWriter>> rightRuns;
    vector<MyDB_RecordIteratorAltPtr> leftRunIterators;
    vector<MyDB_RecordIteratorAltPtr> rightRunIterators;
    string leftEqualityCheck = equalityCheck.first;
    string rightEqualityCheck = equalityCheck.second;
    MyDB_RecordPtr leftLhs = leftTable->getEmptyRecord();
    MyDB_RecordPtr leftRhs = leftTable->getEmptyRecord();
    MyDB_RecordPtr leftTmp = leftTable->getEmptyRecord();
    MyDB_RecordPtr rightLhs = rightTable->getEmptyRecord();
    MyDB_RecordPtr rightRhs = rightTable->getEmptyRecord();
    MyDB_RecordPtr rightTmp = rightTable->getEmptyRecord();
    func leftCmpLhs = leftLhs->compileComputation(leftEqualityCheck);
    func leftCmpRhs = leftRhs->compileComputation(leftEqualityCheck);
    func rightCmpLhs = rightLhs->compileComputation(rightEqualityCheck);
    func rightCmpRhs = rightRhs->compileComputation(rightEqualityCheck);
    vector<MyDB_RecordIteratorAltPtr> joinLhs;
    vector<MyDB_RecordIteratorAltPtr> joinRhs;

	auto leftCmp = [&leftCmpLhs, &leftCmpRhs, &leftLhs, &leftRhs](MyDB_RecordIteratorAltPtr& ptr1, MyDB_RecordIteratorAltPtr& ptr2) {
        ptr1->getCurrent(leftLhs);
        ptr2->getCurrent(leftRhs);
		return leftCmpLhs()->hash() < leftCmpRhs()->hash(); 
	};

	auto rightCmp = [&rightCmpLhs, &rightCmpRhs, &rightLhs, &rightRhs](MyDB_RecordIteratorAltPtr& ptr1, MyDB_RecordIteratorAltPtr& ptr2) {
        ptr1->getCurrent(rightLhs);
        ptr2->getCurrent(rightRhs);
		return rightCmpLhs()->hash() < rightCmpRhs()->hash(); 
	};

    priority_queue<MyDB_RecordIteratorAltPtr, vector<MyDB_RecordIteratorAltPtr>, decltype(leftCmp)> leftMinHeap(leftCmp);
    priority_queue<MyDB_RecordIteratorAltPtr, vector<MyDB_RecordIteratorAltPtr>, decltype(rightCmp)> rightMinHeap(rightCmp);

    // run TPMMS "sort" phase
    sortIntoRuns(leftRuns, leftRunIterators, "left");
    sortIntoRuns(rightRuns, rightRunIterators, "right");

    func leftPred = leftTmp->compileComputation(leftSelectionPredicate);
    func rightPred = rightTmp->compileComputation(rightSelectionPredicate);
    func leftCmp = leftTmp->compileComputation(leftEqualityCheck);
    func rightCmp = rightTmp->compileComputation(rightEqualityCheck);

	// and get the schema that results from combining the left and right records
-	MyDB_SchemaPtr mySchemaOut = make_shared <MyDB_Schema> ();
	for (auto &p : leftTable->getTable ()->getSchema ()->getAtts ())
		mySchemaOut->appendAtt (p);
	for (auto &p : rightTable->getTable ()->getSchema ()->getAtts ())
		mySchemaOut->appendAtt (p);

	// get the combined record
	MyDB_RecordPtr combinedRec = make_shared <MyDB_Record> (mySchemaOut);
	combinedRec->buildFrom (leftTmp, rightTmp);

	func finalPredicate = combinedRec->compileComputation (finalSelectionPredicate);

	// and get the final set of computatoins that will be used to buld the output record
	vector <func> finalComputations;
	for (string s : projections) {
		finalComputations.push_back (combinedRec->compileComputation (s));
	}

	// this is the output record
	MyDB_RecordPtr outputRec = output->getEmptyRecord ();

    for (auto iterator : leftRunIterators) {
		leftMinHeap.push(iterator);
	}
    for (auto iterator : rightRunIterators) {
		rightMinHeap.push(iterator);
	}

    while (!leftMinHeap.empty()) {
        MyDB_RecordIteratorAltPtr leftTop = leftMinHeap.top();
        leftTop->getCurrent(leftTmp);

        while (!rightMinHeap.empty()) {
            MyDB_RecordIteratorAltPtr rightTop = rightMinHeap.top();
            rightTop->getCurrent(rightTmp);

            if (rightCmp()->hash() < leftCmp()->hash()) {   // right key is less then left key, skip it
                rightMinHeap.pop();
                if (rightTop->advance()) {
                    rightMinHeap.push(rightTop);
                }

                continue; 
            } else if (rightCmp()->hash() == leftCmp()->hash()) { // if equals, find all records that could join together 
                size_t k = leftCmp()->hash();

                while (!leftMinHeap.empty() && leftCmp()->hash() == k) {
                    if (leftPred()->toBool()) {
                        joinLhs.push_back(leftTop);
                    }

                    leftMinHeap.pop();
                    if (leftTop->advance()) {
                        leftMinHeap.push(leftTop);
                    }

                    MyDB_RecordIteratorAltPtr leftTop = leftMinHeap.top();
                    leftTop->getCurrent(leftTmp);
                }

                while (!rightMinHeap.empty() && rightCmp()->hash() == k) {
                    if (rightPred()->toBool()) {
                        joinRhs.push_back(rightTop);
                    }

                    rightMinHeap.pop();
                    if (rightTop->advance()) {
                        rightMinHeap.push(rightTop);
                    }

                    MyDB_RecordIteratorAltPtr rightTop = rightMinHeap.top();
                    rightTop->getCurrent(rightTmp);
                }

                for (auto leftRec : joinLhs) {  // join them together
                    leftRec->getCurrent(leftTmp);

                    for (auto rightRec : joinRhs) {
                        rightRec->getCurrent(rightTmp);

                        if (finalPredicate()->toBool()) {
                            int i = 0;
                            for (auto &f : finalComputations) {
                                outputRec->getAtt (i++)->set (f());
                            }

                            outputRec->recordContentHasChanged ();
                            output->append (outputRec);
                        }
                    }
                }

                joinLhs.clear();
                joinRhs.clear();
            } else {    // right key is greater then left key, skip the left one
                leftMinHeap.pop();
                if (leftTop->advance()) {
                    leftMinHeap.push(leftTop);
                }

                break;
            }
        }

        if (rightMinHeap.empty()) {
            break;
        } 
    }
}

#endif
