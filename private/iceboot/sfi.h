#ifndef MAININCLUDE
#define MAININCLUDE

typedef const char *(*CFunc)(const char *);
typedef int (*CFunc1)(int );
typedef int (*CFunc2)(int , int );
typedef int (*CFunc3)(int , int , int);
typedef int (*CFunc4)(int , int , int , int );

void addConstantBucket(const char *nm, int constant);

void addCFunc1Bucket(const char *nm, CFunc1 cf);
void addCFunc2Bucket(const char *nm, CFunc2 cf);
void addCFunc3Bucket(const char *nm, CFunc3 cf);
void addCFunc4Bucket(const char *nm, CFunc4 cf);

#endif
