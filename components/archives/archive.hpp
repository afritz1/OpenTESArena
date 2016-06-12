#ifndef COMPONENTS_ARCHIVES_ARCHIVE_HPP
#define COMPONENTS_ARCHIVES_ARCHIVE_HPP

#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <array>


namespace Archives
{

typedef std::shared_ptr<std::istream> IStreamPtr;

inline uint32_t read_le32(std::istream &stream)
{
    char buf[4];
    if(!stream.read(buf, sizeof(buf)) || stream.gcount() != sizeof(buf))
        return 0;
    return ((uint32_t(buf[0]    )&0x000000ff) | (uint32_t(buf[1]<< 8)&0x0000ff00) |
            (uint32_t(buf[2]<<16)&0x00ff0000) | (uint32_t(buf[3]<<24)&0xff000000));
}

inline uint16_t read_le16(std::istream &stream)
{
    char buf[2];
    if(!stream.read(buf, sizeof(buf)) || stream.gcount() != sizeof(buf))
        return 0;
    return ((uint16_t(buf[0]   )&0x00ff) | (uint16_t(buf[1]<<8)&0xff00));
}


class ConstrainedFileStreamBuf : public std::streambuf {
    std::streamsize mStart, mEnd;

    std::unique_ptr<std::istream> mFile;

    std::array<char,4096> mBuffer;

public:
    ConstrainedFileStreamBuf(std::unique_ptr<std::istream> file, std::streamsize start, std::streamsize end);
    ~ConstrainedFileStreamBuf();

    virtual int_type underflow();

    virtual pos_type seekoff(off_type offset, std::ios_base::seekdir whence, std::ios_base::openmode mode);
    virtual pos_type seekpos(pos_type pos, std::ios_base::openmode mode);
};

class ConstrainedFileStream : public std::istream {
public:
    ConstrainedFileStream(std::unique_ptr<std::istream> file, std::streamsize start, std::streamsize end)
        : std::istream(new ConstrainedFileStreamBuf(std::move(file), start, end))
    {
    }

    ~ConstrainedFileStream()
    {
        delete rdbuf();
    }
};


class Archive {
public:
    virtual ~Archive() { }
    virtual IStreamPtr open(const char *name) = 0;
    virtual bool exists(const char *name) const = 0;
    virtual const std::vector<std::string> &list() const = 0;
};

} // namespace Archives

#endif /* COMPONENTS_ARCHIVES_ARCHIVE_HPP */
