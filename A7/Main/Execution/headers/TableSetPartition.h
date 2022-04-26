
#ifndef TABLESETPARTITION_H
#define TABLESETPARTITION_H

#include "SFWQuery.h"
#include <math.h>

class TableSetPartition {
    private:
        vector<pair<string, string>> tableSet;
        int index;
        int limit;

    public:
        TableSetPartition(vector<pair<string, string>>& tables): tableSet(tables), index(1), limit(pow(2, tables.size()) - 1) {}

        bool hasNext() {
            return index < limit;
        }

        pair<vector<pair<string, string>>, vector<pair<string, string>>> getNextPartition() {
            vector<pair<string, string>> leftTables;
			vector<pair<string, string>> rightTables;

			for (int j = 0; j < tableSet.size(); j++) {
				if ((index & (1 << j)) != 0) {
					leftTables.push_back(tableSet[j]);
				} else {
					rightTables.push_back(tableSet[j]);
				}
			}
            index++;

            return make_pair(leftTables, rightTables);
        }
};

#endif