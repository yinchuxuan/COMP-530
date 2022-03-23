
#ifndef REG_SELECTION_C                                        
#define REG_SELECTION_C

#include "RegularSelection.h"

RegularSelection :: RegularSelection (MyDB_TableReaderWriterPtr inputIn, MyDB_TableReaderWriterPtr outputIn,
                string selectionPredicateIn, vector <string> projectionsIn) {
    input = inputIn;
    output = outputIn;
    selectionPredicate = selectionPredicateIn;
    projections = projectionsIn;
}

void RegularSelection :: run () {
    MyDB_RecordPtr inputRec = input->getEmptyRecord();
    MyDB_RecordPtr outputRec = output->getEmptyRecord();
    func inputPred = inputRec->compileComputation (selectionPredicate);
    // and get the final set of computations that will be used to build the output record
    vector <func> finalComputations;
    for (string s : projections) {
        finalComputations.push_back (combinedRec->compileComputation (s));
    }
    // now, iterate through the input table
    MyDB_RecordIteratorPtr myIter = rightTable->getIterator (input);
    while(myIter->hasNext()) {
        myIter->getNext ();
        // see if it is accepted by the preicate
        if (myIter ()->toBool ()) {
            // run all of the computations
            int i = 0;
            for (auto &f : finalComputations) {
                outputRec->getAtt (i++)->set (f());
            }
            outputRec->recordContentHasChanged ();
            output->append (outputRec);
        }
    }
}

#endif
