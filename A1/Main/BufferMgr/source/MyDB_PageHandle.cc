
#ifndef PAGE_HANDLE_C
#define PAGE_HANDLE_C

#include <memory>
#include "MyDB_PageHandle.h"

void *MyDB_PageHandleBase :: getBytes () {
	return managerInstance->getBuffer(pageKey);
}

void MyDB_PageHandleBase :: wroteBytes () {
	managerInstance->writeBuffer(pageKey);
}

MyDB_PageHandleBase :: ~MyDB_PageHandleBase () {
	managerInstance->decreaseReference(pageKey);
}

#endif

