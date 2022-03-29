
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

void SortMergeJoin :: run () {
    MyDB_RecordPtr leftLhs = leftTable->getEmptyRecord();
    MyDB_RecordPtr leftRhs = leftTable->getEmptyRecord();
    MyDB_RecordPtr leftTmp = leftTable->getEmptyRecord();
    MyDB_RecordPtr rightLhs = rightTable->getEmptyRecord();
    MyDB_RecordPtr rightRhs = rightTable->getEmptyRecord();
    MyDB_RecordPtr rightTmp = rightTable->getEmptyRecord();
    vector<void*> joinLhs;
    vector<void*> joinRhs;
    MyDB_RecordPtr leftJoinRec = leftTable->getEmptyRecord();
    MyDB_RecordPtr rightJoinRec = rightTable->getEmptyRecord();
    vector<MyDB_PageReaderWriter> joinLhsPages;
    vector<MyDB_PageReaderWriter> joinRhsPages;
    MyDB_RecordIteratorAltPtr joinLeft;
    MyDB_RecordIteratorAltPtr joinRight;
    int finished = 0;

    func leftCmp = leftTmp->compileComputation(equalityCheck.first);
    func rightCmp = rightTmp->compileComputation(equalityCheck.second);

	// and get the schema that results from combining the left and right records
	MyDB_SchemaPtr mySchemaOut = make_shared <MyDB_Schema> ();
	for (auto &p : leftTable->getTable ()->getSchema ()->getAtts ())
		mySchemaOut->appendAtt (p);
	for (auto &p : rightTable->getTable ()->getSchema ()->getAtts ())
    mySchemaOut->appendAtt (p);

	// get the combined record
	MyDB_RecordPtr combinedRec = make_shared <MyDB_Record> (mySchemaOut);
	combinedRec->buildFrom (leftJoinRec, rightJoinRec);

	func finalPredicate = combinedRec->compileComputation (finalSelectionPredicate);

	// and get the final set of computatoins that will be used to buld the output record
	vector <func> finalComputations;
	for (string s : projections) {
		finalComputations.push_back (combinedRec->compileComputation (s));
	}

	// this is the output record
	MyDB_RecordPtr outputRec = output->getEmptyRecord ();

    function<bool()> leftComp = buildRecordComparator(leftLhs, leftRhs, equalityCheck.first);
    MyDB_RecordIteratorAltPtr left_iter = buildItertorOverSortedRuns(runSize, *leftTable, leftComp, leftLhs, leftRhs, leftSelectionPredicate);
    function<bool()> rightComp = buildRecordComparator(rightLhs, rightRhs, equalityCheck.second);
    MyDB_RecordIteratorAltPtr right_iter = buildItertorOverSortedRuns(runSize, *rightTable, rightComp, rightLhs, rightRhs, rightSelectionPredicate);

    if (!left_iter->advance() || !right_iter->advance()) {
        return;
    }

    while (true) {
        left_iter->getCurrent(leftTmp);
        right_iter->getCurrent(rightTmp);

        if (rightCmp()->hash() < leftCmp()->hash()) {   // right key is less then left key, skip it
            if (!right_iter->advance()) {
                finished = 1;
            }
        } else if (rightCmp()->hash() == leftCmp()->hash()) { // if equals, find all records that could join together 
            size_t k = leftCmp()->hash();

            while (leftCmp()->hash() == k) {
                if (joinLhsPages.size() == 0) {
                    MyDB_PageReaderWriter newPage(true, *output->getBufferMgr());
                    joinLhsPages.push_back(newPage);
                }
                if (!joinLhsPages[joinLhsPages.size() - 1].append(leftTmp)) {
                    MyDB_PageReaderWriter newPage(true, *output->getBufferMgr());
                    joinLhsPages.push_back(newPage);
                    joinLhsPages[joinLhsPages.size() - 1].append(leftTmp);
                }

                if (!left_iter->advance()) {
                    finished = 1;
                    break;
                }

                left_iter->getCurrent(leftTmp);
            }

            while (rightCmp()->hash() == k) {
                if (joinRhsPages.size() == 0) {
                    MyDB_PageReaderWriter newPage(true, *output->getBufferMgr());
                    joinRhsPages.push_back(newPage);
                }
                if (!joinRhsPages[joinRhsPages.size() - 1].append(rightTmp)) {
                    MyDB_PageReaderWriter newPage(true, *output->getBufferMgr());
                    joinRhsPages.push_back(newPage);
                    joinRhsPages[joinRhsPages.size() - 1].append(rightTmp);
                }

                if (!right_iter->advance()) {
                    finished = 1;
                    break;
                }

                right_iter->getCurrent(rightTmp);               
            }

            joinLeft = getIteratorAlt(joinLhsPages);

            while (joinLeft->advance()) {  // join them together
                joinLeft->getCurrent(leftJoinRec);

                joinRight = getIteratorAlt(joinRhsPages);
                while (joinRight->advance()) {
                    joinRight->getCurrent(rightJoinRec);

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

            joinLhsPages.clear();
            joinRhsPages.clear();
        } else {    // right key is greater then left key, skip the left one
            if (!left_iter->advance()) {
                finished = 1;
            }
        }

        if (finished) {
            break;
        } 
    }
}

#endif
