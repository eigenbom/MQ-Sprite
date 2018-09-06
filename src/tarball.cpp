/*
 * tarball.cpp
 *
 *  Created on: Jul 28, 2010
 *      Author: Pierre Lindenbaum PhD
 *              plindenbaum@yahoo.fr
 *              http://plindenbaum.blogspot.com
 *
 */
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <ctime>
// #include <unistd.h>
#include <iostream>
#include "tarball.h"
// #include "lindenb/lang/throw.h"

#include <QDebug>

/*
// snprintf replacement
// from: http://stackoverflow.com/questions/2915672/snprintf-and-visual-studio-2010
#ifdef _MSC_VER
#include <cstdio>
#include <cstdlib>
#define snprintf c99_snprintf
inline int c99_snprintf(char* str, size_t size, const char* format, ...)
{
    int count;
    va_list ap;

    va_start(ap, format);
    count = c99_vsnprintf(str, size, format, ap);
    va_end(ap);

    return count;
}

inline int c99_vsnprintf(char* str, size_t size, const char* format, va_list ap)
{
    int count = -1;

    if (size != 0)
        count = _vsnprintf_s(str, size, _TRUNCATE, format, ap);
    if (count == -1)
        count = _vscprintf(format, ap);

    return count;
}

#endif // _MSC_VER
*/

#define LOCALNS lindenb::io
#define TARHEADER static_cast<PosixTarHeader*>(header)

struct PosixTarHeader
{
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char typeflag[1];
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char pad[12];
};

void LOCALNS::TarOut::_init(void* header)
{
    std::memset(header,0,sizeof(PosixTarHeader));
    std::sprintf(TARHEADER->magic,"ustar  ");
    std::sprintf(TARHEADER->mtime,"%011lo", (unsigned long) time(nullptr));
    std::sprintf(TARHEADER->mode,"%07o",0644);
    std::sprintf(TARHEADER->uname,"ben");
    // char * s = ::getlogin();
    // if(s!=NULL)  std::snprintf(TARHEADER,32,"%s",s);
    // std::snprintf(TARHEADER,32,"%s","ben");
    std::sprintf(TARHEADER->gname,"%s","users");
}

void LOCALNS::TarOut::_checksum(void* header)
{
    unsigned int sum = 0;
    char *p = (char *) header;
    char *q = p + sizeof(PosixTarHeader);
    while (p < TARHEADER->checksum) sum += *p++ & 0xff;
    for (int i = 0; i < 8; ++i)  {
        sum += ' ';
        ++p;
    }
    while (p < q) sum += *p++ & 0xff;

    std::sprintf(TARHEADER->checksum,"%06o",sum);
}

void LOCALNS::TarOut::_size(void* header,unsigned long fileSize)
{
    std::sprintf(TARHEADER->size,"%011llo",(long long unsigned int)fileSize);
}

void LOCALNS::TarOut::_filename(void* header,const char* filename)
{
    if(filename==NULL || filename[0]==0 || std::strlen(filename)>=100)
    {
        qDebug() << "invalid archive name \"" << filename << "\"";
        // THROW("invalid archive name \"" << filename << "\"");
    }

    // snprintf()
    _snprintf(TARHEADER->name,100,"%s",filename);
}

void LOCALNS::TarOut::_endRecord(std::size_t len)
{
    char c='\0';
    while((len%sizeof(PosixTarHeader))!=0)
    {
        out.write(&c,sizeof(char));
        ++len;
    }
}


LOCALNS::TarOut::TarOut(std::ostream& out):_finished(false),out(out)
{
    if(sizeof(PosixTarHeader)!=512)
    {
        qDebug() << sizeof(PosixTarHeader);
        // THROW(sizeof(PosixTarHeader));
    }
}

LOCALNS::TarOut::~TarOut()
{
    if(!_finished)
    {
        qWarning("[warning]tar file was not finished.");
    }
}

/** writes 2 empty blocks. Should be always called before closing the Tar file */
void LOCALNS::TarOut::finish()
{
    _finished=true;
    //The end of the archive is indicated by two blocks filled with binary zeros
    PosixTarHeader header;
    std::memset((void*)&header,0,sizeof(PosixTarHeader));
    out.write((const char*)&header,sizeof(PosixTarHeader));
    out.write((const char*)&header,sizeof(PosixTarHeader));
    out.flush();
}

void LOCALNS::TarOut::put(const char* filename,const std::string& s)
{
    put(filename,s.c_str(),s.size());
}

void LOCALNS::TarOut::put(const char* filename,const char* content)
{
    put(filename,content,std::strlen(content));
}

void LOCALNS::TarOut::put(const char* filename,const char* content,std::size_t len)
{
    PosixTarHeader header;
    _init((void*)&header);
    _filename((void*)&header,filename);
    header.typeflag[0]=0;
    _size((void*)&header,len);
    _checksum((void*)&header);
    out.write((const char*)&header,sizeof(PosixTarHeader));
    out.write(content,len);
    _endRecord(len);
}

void LOCALNS::TarOut::putFile(const char* filename,const char* nameInArchive)
{
    char buff[BUFSIZ];
    std::FILE* in=std::fopen(filename,"rb");
    if(in==NULL)
    {
        qDebug() << "Cannot open " << filename << " "<< std::strerror(errno);
        // THROW("Cannot open " << filename << " "<< std::strerror(errno));
    }
    std::fseek(in, 0L, SEEK_END);
    long int len= std::ftell(in);
    std::fseek(in,0L,SEEK_SET);

    PosixTarHeader header;
    _init((void*)&header);
    _filename((void*)&header,nameInArchive);
    header.typeflag[0]=0;
    _size((void*)&header,len);
    _checksum((void*)&header);
    out.write((const char*)&header,sizeof(PosixTarHeader));

    std::size_t nRead=0;
    while((nRead=std::fread(buff,sizeof(char),BUFSIZ,in))>0)
    {
        out.write(buff,nRead);
    }
    std::fclose(in);

    _endRecord(len);
}

// Ban's additions
LOCALNS::TarIn::TarIn(std::istream& in):in(in),mOK(true),mEndOfStream(false)
{
    if(sizeof(PosixTarHeader)!=512)
    {
        Q_ASSERT(sizeof(PosixTarHeader)!=512);
        mOK = false;
        // , "sizeof(PosixTarHeader)!=512");
    }
}

LOCALNS::TarIn::~TarIn()
{
    // delete all buffers..
    std::map<std::string, Record>::iterator it = mFileMap.begin();
    for(;it!=mFileMap.end();it++){
        delete it->second.buffer;
    }
}

void PrintHeader(PosixTarHeader* header){
    qDebug() << "[TAR HEADER]";
    qDebug() << " name" << QString::fromUtf8(header->name, std::min((int)strlen(header->name),100)) << "\n"
             << "mode" << QString::fromUtf8(header->mode, std::min((int)strlen(header->mode),8)) << "\n"
             << "uid" << QString::fromUtf8(header->uid, std::min((int)strlen(header->uid),8)) << "\n"
             << "gid" << QString::fromUtf8(header->gid , std::min((int)strlen(header->gid),8)) << "\n"
             << "size" << QString::fromUtf8(header->size, std::min((int)strlen(header->size),12)) << "\n"
             << "mtime" << QString::fromUtf8(header->mtime , std::min((int)strlen(header->mtime),12)) << "\n"
             << "checksum" << QString::fromUtf8(header->checksum, std::min((int)strlen(header->checksum),8)) << "\n"
             << "typeflag" << (int)(header->typeflag[0]) << "\n"
             << "linkname" <<  QString::fromUtf8(header->linkname, std::min((int)strlen(header->linkname),100)) << "\n"
             << "magic" << QString::fromUtf8(header->magic, std::min((int)strlen(header->magic),6)) << "\n"
             << "version" << QString::fromUtf8(header->version, std::min((int)strlen(header->version),2));
    qDebug() << "[-----------]";
}

void LOCALNS::TarIn::readHeader(void* pvheader){
    PosixTarHeader* header = (PosixTarHeader*)pvheader;
    in.read((char*)header, sizeof(PosixTarHeader));
    if (!in.good()){
        mOK = false;
        return;
    }
    // PrintHeader(header);

    // Check for end of stream
    mEndOfStream = true;
    for(int i=0;i<(int)sizeof(PosixTarHeader);i++){
        if (((char*)header)[i]!=0) mEndOfStream = false;
    }
    if (mEndOfStream){
        in.read((char*)header, sizeof(PosixTarHeader));
        for(int i=0;i<(int)sizeof(PosixTarHeader);i++){
            if (((char*)header)[i]!=0) mEndOfStream = false;
        }
        if (mEndOfStream) return;
        else {
            qDebug() << "Empty block found";
            mOK = false;
            return;
        }
    }

    // Check magic/tar type
    bool magicValid = (strcmp(header->magic,"ustar  ")==0);
    if (!magicValid){
        static bool hasWarned = false;
        if (!hasWarned){
            qWarning() << "Tar doesn't look right, continuing with caution..";
            hasWarned = true;
        }
    }
    char* filename = header->name;
    if(filename==NULL || filename[0]==0 || std::strlen(filename)>=100)
    {
        qDebug() << "invalid archive name, nulling";
        memset(header->name, 0, 100);
    }
    else {
        // qDebug() << "filename: " << filename;
    }
}

void LOCALNS::TarIn::read(){
    PosixTarHeader header;
    readHeader((void*)&header);

    while (mOK && !mEndOfStream){
        long long unsigned int fileSize = 0;
        std::sscanf(header.size, "%011llo", &fileSize);
        // qDebug() << "Found file: " << header.name << "[" << fileSize << "b]";

        Record record;
        record.length = fileSize+1;
        record.buffer = new char[record.length+1]; // deleted in ~TarIn
        memset(record.buffer, 0, record.length+1);
        in.read(record.buffer, fileSize);

        mFileMap[std::string(header.name)] = record;

        // End of record
        char c='\0';
        while((fileSize%sizeof(PosixTarHeader))!=0)
        {
            in.read(&c,sizeof(char));
            ++fileSize;
        }

        // Read next header..
        readHeader((void*)&header);
    }

    /*
    while (mOK){
        // TODO: read segments
        PosixTarHeader header;
        readHeader((void*)&header);

        if (mOK){
            //long long unsigned int fileSize = 0;
            //std::sscanf(header.size, "%011llo", &fileSize);
            //qDebug() << "fileSize: " << fileSize;
        }
    }
    */
}


bool LOCALNS::TarIn::ok(){
    return mOK;
}
