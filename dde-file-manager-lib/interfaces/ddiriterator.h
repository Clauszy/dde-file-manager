#ifndef DDIRITERATOR_H
#define DDIRITERATOR_H

#include "dabstractfileinfo.h"

class DDirIterator
{
public:
    virtual ~DDirIterator() {}

    virtual DUrl next() = 0;
    virtual bool hasNext() const = 0;
    virtual void close() {}

    virtual QString fileName() const = 0;
    virtual QString filePath() const = 0;
    virtual const DAbstractFileInfoPointer fileInfo() const = 0;
    virtual QString path() const = 0;

    virtual bool hasIteratorOfSubdir() const {return false;}
};

typedef QSharedPointer<DDirIterator> DDirIteratorPointer;

#endif // DDIRITERATOR_H
