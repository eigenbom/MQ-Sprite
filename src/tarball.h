// Author: Pierre Lindenbaum (2010) (plindenbaum@yahoo.fr) 
// Modified: Ben Porter (2018)

#ifndef LINDENB_IO_TARBALL_H_
#define LINDENB_IO_TARBALL_H_

#include <iostream>
#include <map>
#include <string>

namespace lindenb { namespace io {

class TarOut
{
private:
    bool _finished;

protected:
    std::ostream& out;
    void _init(void* header);
    void _checksum(void* header);
    void _size(void* header,unsigned long fileSize);
    void _filename(void* header,const char* filename);
    void _endRecord(std::size_t len);

public:
    TarOut(std::ostream& out);
    virtual ~TarOut();
    /** writes 2 empty blocks. Should be always called before closing the Tar file */
    void finish();
    void put(const char* filename, const std::string& s);
    void put(const char* filename, const char* content);
    void put(const char* filename, const char* content, std::size_t len);
    void putFile(const char* filename, const char* nameInArchive);
};

class TarIn
{
public:
    struct Record {
        char* buffer;
        int length;
    };
private:
    std::istream& in;
    std::map<std::string, Record> mFileMap;
    bool mOK;
    bool mEndOfStream;

protected:
    void readHeader(void* header);

public:
    TarIn(std::istream& in);
    virtual ~TarIn();

    void read();
    bool ok();
    std::map<std::string,Record>& fileMap(){return mFileMap;}
};

}}

#endif
