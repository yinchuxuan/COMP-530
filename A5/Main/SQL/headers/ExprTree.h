
#ifndef SQL_EXPRESSIONS
#define SQL_EXPRESSIONS

#include "MyDB_AttType.h"
#include "MyDB_Table.h"
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>

// create a smart pointer for database tables
using namespace std;
class ExprTree;
typedef shared_ptr <ExprTree> ExprTreePtr;

// this class encapsules a parsed SQL expression (such as "this.that > 34.5 AND 4 = 5")

enum ExprType {
	NUMBER_EXPR,
	BOOL_EXPR,
	STRING_EXPR,
	IDENTIFIER_EXPR
};

// class ExprTree is a pure virtual class... the various classes that implement it are below
class ExprTree {

protected:
	ExprType type;

public:
	virtual string toString () = 0;
	virtual ~ExprTree () {}
	virtual int check(unordered_map<string, MyDB_TablePtr>& alias_map) = 0;

	ExprTree() {
		type = NUMBER_EXPR;
	}
	
	ExprType getType() {
		return type;
	} 

	void setType(ExprType t) {
		type = t;
	}
};

class BoolLiteral : public ExprTree {

private:
	bool myVal;
public:
	
	BoolLiteral (bool fromMe) {
		myVal = fromMe;
	}

	string toString () {
		if (myVal) {
			return "bool_EXPR[true]";
		} else {
			return "bool_EXPR[false]";
		}
	}	

	int check(unordered_map<string, MyDB_TablePtr>& alias_map) {
		setType(BOOL_EXPR);
		return 0;
	} 

};

class DoubleLiteral : public ExprTree {

private:
	double myVal;
public:

	DoubleLiteral (double fromMe) {
		myVal = fromMe;
		type = NUMBER_EXPR;
	}

	string toString () {
		return "double[" + to_string (myVal) + "]";
	}	

	int check(unordered_map<string, MyDB_TablePtr>& alias_map) {
		setType(NUMBER_EXPR);
		return 0;
	}

	~DoubleLiteral () {}
};

// this implement class ExprTree
class IntLiteral : public ExprTree {

private:
	int myVal;
public:

	IntLiteral (int fromMe) {
		myVal = fromMe;
	}

	string toString () {
		return "int[" + to_string (myVal) + "]";
	}

	int check(unordered_map<string, MyDB_TablePtr>& alias_map) {
		setType(NUMBER_EXPR);
		return 0;
	}

	~IntLiteral () {}
};

class StringLiteral : public ExprTree {

private:
	string myVal;
public:

	StringLiteral (char *fromMe) {
		fromMe[strlen (fromMe) - 1] = 0;
		myVal = string (fromMe + 1);
	}

	string toString () {
		return "string[" + myVal + "]";
	}

	int check(unordered_map<string, MyDB_TablePtr>& alias_map) {
		setType(STRING_EXPR);
		return 0;
	}

	~StringLiteral () {}
};

class Identifier : public ExprTree {

private:
	string tableName;
	string attName;
public:

	Identifier (char *tableNameIn, char *attNameIn) {
		tableName = string (tableNameIn);
		attName = string (attNameIn);
	}

	string toString () {
		return "[" + tableName + "_" + attName + "]";
	}	

	int check(unordered_map<string, MyDB_TablePtr>& alias_map) {
		if (!alias_map.count(tableName)) {
			cout << "Parse Error: there is no table named " + tableName + " in " + toString() + "!" << endl;
			return 1;
		} else {
			pair<int, MyDB_AttTypePtr> atts = (alias_map[tableName])->getSchema()->getAttByName(attName);
			if (atts.first == -1) {     // not found the attribute
				cout << "Parse Error: table " + tableName + " has no attribute " + attName + "!" << endl;
				return 1;
			}

			MyDB_AttTypePtr type = atts.second;
			if (type->toString() == "int" || type->toString() == "double") {
				setType(NUMBER_EXPR);
			} else if (type->toString() == "string") {
				setType(STRING_EXPR);
			} else if (type->toString() == "bool") {
				setType(BOOL_EXPR);
			}
		}

		return 0;
	}

	~Identifier () {}
};

class MinusOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	MinusOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "- (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	int check(unordered_map<string, MyDB_TablePtr>& alias_map) {
		if (lhs->check(alias_map) || rhs->check(alias_map)) {
            return 1;
        }
		if (lhs->getType() != NUMBER_EXPR && lhs->getType() != IDENTIFIER_EXPR) {
			cout << "Parse Error: wrong type for minus formula term " + lhs->toString() << "!" << endl; 
			return 1;
		}
		if (rhs->getType() != NUMBER_EXPR && rhs->getType() != IDENTIFIER_EXPR) {
			cout << "Parse Error: wrong type for minus formula term " + rhs->toString() << "!" << endl; 
			return 1;
		}

		setType(NUMBER_EXPR);

		return 0;
	}

	~MinusOp () {}
};

class PlusOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	PlusOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "+ (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	int check(unordered_map<string, MyDB_TablePtr>& alias_map) {
		if (lhs->check(alias_map) || rhs->check(alias_map)) {
            return 1;
        }
		if (lhs->getType() == NUMBER_EXPR) {
			if (rhs->getType() != NUMBER_EXPR && rhs->getType() != IDENTIFIER_EXPR) {
				cout << "Parse Error: wrong type for plus formula term " + rhs->toString() << "!" << endl; 
				return 1;
			}

			setType(NUMBER_EXPR);
		} else if (lhs->getType() == STRING_EXPR) {
			if (rhs->getType() != STRING_EXPR && rhs->getType() != IDENTIFIER_EXPR) {
				cout << "Parse Error: wrong type for concatenation formula term " + rhs->toString() << "!" << endl; 
				return 1;
			}

			setType(STRING_EXPR);
		} else if (lhs->getType() == IDENTIFIER_EXPR) {
			if (rhs->getType() == STRING_EXPR) {
				setType(STRING_EXPR);
			} else if (rhs->getType() == NUMBER_EXPR) {
				setType(NUMBER_EXPR);
			} else if (rhs->getType() == IDENTIFIER_EXPR) {
				setType(IDENTIFIER_EXPR);
			} else {
				cout << "Parse Error: wrong type for plus formula term " + rhs->toString() << "!" << endl; 
				return 1;
			}
		} else {
			cout << "Parse Error: wrong type for plus formula term " + lhs->toString() << "!" << endl; 
			return 1;
		}

		return 0;
	}

	~PlusOp () {}
};

class TimesOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	TimesOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "* (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	int check(unordered_map<string, MyDB_TablePtr>& alias_map) {
		if (lhs->check(alias_map) || rhs->check(alias_map)) {
            return 1;
        }
		if (lhs->getType() != NUMBER_EXPR && lhs->getType() != IDENTIFIER_EXPR) {
			cout << "Parse Error: wrong type for times formula term " + lhs->toString() << "!" << endl; 
			return 1;
		}
		if (rhs->getType() != NUMBER_EXPR && rhs->getType() != IDENTIFIER_EXPR) {
			cout << "Parse Error: wrong type for times formula term " + rhs->toString() << "!" << endl; 
			return 1;
		}

		setType(NUMBER_EXPR);

		return 0;
	}

	~TimesOp () {}
};

class DivideOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	DivideOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "/ (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	int check(unordered_map<string, MyDB_TablePtr>& alias_map) {
		if (lhs->check(alias_map) || rhs->check(alias_map)) {
            return 1;
        }
		if (lhs->getType() != NUMBER_EXPR && lhs->getType() != IDENTIFIER_EXPR) {
			cout << "Parse Error: wrong type for divide formula term " + lhs->toString() << "!" << endl; 
			return 1;
		}
		if (rhs->getType() != NUMBER_EXPR && rhs->getType() != IDENTIFIER_EXPR) {
			cout << "Parse Error: wrong type for divide formula term " + rhs->toString() << "!" << endl; 
			return 1;
		}

		setType(NUMBER_EXPR);

		return 0;
	}

	~DivideOp () {}
};

class GtOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	GtOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "> (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	int check(unordered_map<string, MyDB_TablePtr>& alias_map) {
		if (lhs->check(alias_map) || rhs->check(alias_map)) {
            return 1;
        }

		if (lhs->getType() == IDENTIFIER_EXPR) {
			if (rhs->getType() != IDENTIFIER_EXPR && rhs->getType() != NUMBER_EXPR && rhs->getType() != STRING_EXPR) {
				cout << "Parse Error: wrong type for comparation term " + rhs->toString() + "!" << endl;
				return 1;
			}
		} else if (lhs->getType() == NUMBER_EXPR) {
			if (rhs->getType() != IDENTIFIER_EXPR && rhs->getType() != NUMBER_EXPR) {
				cout << "Parse Error: wrong type for comparation term " + rhs->toString() + "!" << endl;
				return 1;
			}
		} else if (lhs->getType() == STRING_EXPR) {
			if (rhs->getType() != IDENTIFIER_EXPR && rhs->getType() != STRING_EXPR) {
				cout << "Parse Error: wrong type for comparation term " + rhs->toString() + "!" << endl;
				return 1;
			}
		} else {
			cout << "Parse Error: wrong type for comparation term " + lhs->toString() + "!" << endl;
			return 1;
		}

		setType(BOOL_EXPR);

		return 0;
	}

	~GtOp () {}
};

class LtOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	LtOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "< (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	int check(unordered_map<string, MyDB_TablePtr>& alias_map) {
		if (lhs->check(alias_map) || rhs->check(alias_map)) {
            return 1;
        }

		if (lhs->getType() == IDENTIFIER_EXPR) {
			if (rhs->getType() != IDENTIFIER_EXPR && rhs->getType() != NUMBER_EXPR && rhs->getType() != STRING_EXPR) {
				cout << "Parse Error: wrong type for comparation term " + rhs->toString() + "!" << endl;
				return 1;
			}
		} else if (lhs->getType() == NUMBER_EXPR) {
			if (rhs->getType() != IDENTIFIER_EXPR && rhs->getType() != NUMBER_EXPR) {
				cout << "Parse Error: wrong type for comparation term " + rhs->toString() + "!" << endl;
				return 1;
			}
		} else if (lhs->getType() == STRING_EXPR) {
			if (rhs->getType() != IDENTIFIER_EXPR && rhs->getType() != STRING_EXPR) {
				cout << "Parse Error: wrong type for comparation term " + rhs->toString() + "!" << endl;
				return 1;
			}
		} else {
			cout << "Parse Error: wrong type for comparation term " + lhs->toString() + "!" << endl;
			return 1;
		}

		setType(BOOL_EXPR);

		return 0;
	}

	~LtOp () {}
};

class NeqOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	NeqOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "!= (" + lhs->toString () + ", " + rhs->toString () + ")";
	}

	int check(unordered_map<string, MyDB_TablePtr>& alias_map) {
		if (lhs->check(alias_map) || rhs->check(alias_map)) {
            return 1;
        }

		if (lhs->getType() == IDENTIFIER_EXPR) {
			if (rhs->getType() != IDENTIFIER_EXPR && rhs->getType() != NUMBER_EXPR && rhs->getType() != STRING_EXPR) {
				cout << "Parse Error: wrong type for comparation term " + rhs->toString() + "!" << endl;
				return 1;
			}
		} else if (lhs->getType() == NUMBER_EXPR) {
			if (rhs->getType() != IDENTIFIER_EXPR && rhs->getType() != NUMBER_EXPR) {
				cout << "Parse Error: wrong type for comparation term " + rhs->toString() + "!" << endl;
				return 1;
			}
		} else if (lhs->getType() == STRING_EXPR) {
			if (rhs->getType() != IDENTIFIER_EXPR && rhs->getType() != STRING_EXPR) {
				cout << "Parse Error: wrong type for comparation term " + rhs->toString() + "!" << endl;
				return 1;
			}
		} else {
			cout << "Parse Error: wrong type for comparation term " + lhs->toString() + "!" << endl;
			return 1;
		}

		setType(BOOL_EXPR);

		return 0;
	}

	~NeqOp () {}
};

class OrOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	OrOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "|| (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	int check(unordered_map<string, MyDB_TablePtr>& alias_map) {
		if (lhs->check(alias_map) || rhs->check(alias_map)) {
            return 1;
        }

		if (lhs->getType() != BOOL_EXPR) {
			cout << "Parse Error: wrong type for logical operation term " + lhs->toString() + "!" << endl;
			return 1;
		}	
		if (rhs->getType() != BOOL_EXPR) {
			cout << "Parse Error: wrong type for logical operation term " + lhs->toString() + "!" << endl;
			return 1;
		}

		setType(BOOL_EXPR);

		return 0;
	}

	~OrOp () {}
};

class EqOp : public ExprTree {

private:

	ExprTreePtr lhs;
	ExprTreePtr rhs;
	
public:

	EqOp (ExprTreePtr lhsIn, ExprTreePtr rhsIn) {
		lhs = lhsIn;
		rhs = rhsIn;
	}

	string toString () {
		return "== (" + lhs->toString () + ", " + rhs->toString () + ")";
	}	

	int check(unordered_map<string, MyDB_TablePtr>& alias_map) {
		if (lhs->check(alias_map) || rhs->check(alias_map)) {
            return 1;
        }

		if (lhs->getType() == IDENTIFIER_EXPR) {
			if (rhs->getType() != IDENTIFIER_EXPR && rhs->getType() != NUMBER_EXPR && rhs->getType() != STRING_EXPR) {
				cout << "Parse Error: wrong type for comparation term " + rhs->toString() + "!" << endl;
				return 1;
			}
		} else if (lhs->getType() == NUMBER_EXPR) {
			if (rhs->getType() != IDENTIFIER_EXPR && rhs->getType() != NUMBER_EXPR) {
				cout << "Parse Error: wrong type for comparation term " + rhs->toString() + "!" << endl;
				return 1;
			}
		} else if (lhs->getType() == STRING_EXPR) {
			if (rhs->getType() != IDENTIFIER_EXPR && rhs->getType() != STRING_EXPR) {
				cout << "Parse Error: wrong type for comparation term " + rhs->toString() + "!" << endl;
				return 1;
			}
		} else {
			cout << "Parse Error: wrong type for comparation term " + lhs->toString() + "!" << endl;
			return 1;
		}

		setType(BOOL_EXPR);

		return 0;
	}

	~EqOp () {}
};

class NotOp : public ExprTree {

private:

	ExprTreePtr child;
	
public:

	NotOp (ExprTreePtr childIn) {
		child = childIn;
	}

	string toString () {
		return "!(" + child->toString () + ")";
	}	

	ExprTreePtr getChild() {
		return child;
	}

	int check(unordered_map<string, MyDB_TablePtr>& alias_map) {
		if (child->check(alias_map)) {
            return 1;
        }

		if (child->getType() != BOOL_EXPR) {
			cout << "Parse Error: wrong type for logical operation term " + child->toString() + "!" << endl;
			return 1;
		}	

		setType(BOOL_EXPR);

		return 0;
	}

	~NotOp () {}
};

class SumOp : public ExprTree {

private:

	ExprTreePtr child;
	
public:

	SumOp (ExprTreePtr childIn) {
		child = childIn;
	}

	string toString () {
		return "sum(" + child->toString () + ")";
	}	

	int check(unordered_map<string, MyDB_TablePtr>& alias_map) {
		if (child->check(alias_map)) {
            return 1;
        }

		if (child->getType() != NUMBER_EXPR && child->getType() != IDENTIFIER_EXPR) {
			cout << "Parse Error: wrong type for sum operation term " + child->toString() + "!" << endl;
			return 1;
		}	

		setType(NUMBER_EXPR);

		return 0;
	}

	~SumOp () {}
};

class AvgOp : public ExprTree {

private:

	ExprTreePtr child;
	
public:

	AvgOp (ExprTreePtr childIn) {
		child = childIn;
	}

	string toString () {
		return "avg(" + child->toString () + ")";
	}	

	int check(unordered_map<string, MyDB_TablePtr>& alias_map) {
		if (child->check(alias_map)) {
            return 1;
        }

		if (child->getType() != NUMBER_EXPR && child->getType() != IDENTIFIER_EXPR) {
			cout << "Parse Error: wrong type for avg operation term " + child->toString() + "!" << endl;
			return 1;
		}	

		setType(NUMBER_EXPR);

		return 0;
	}

	~AvgOp () {}
};

#endif
