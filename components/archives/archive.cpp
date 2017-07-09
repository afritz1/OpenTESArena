
#include "archive.hpp"


namespace Archives
{

ConstrainedFileStreamBuf::ConstrainedFileStreamBuf(std::unique_ptr<std::istream> file, std::streamsize start, std::streamsize end)
  : mStart(start), mEnd(end), mFile(std::move(file))
{
}
ConstrainedFileStreamBuf::~ConstrainedFileStreamBuf()
{
}

ConstrainedFileStreamBuf::int_type ConstrainedFileStreamBuf::underflow()
{
    if(gptr() == egptr())
    {
        std::streamsize toread = std::min<std::streamsize>(mEnd-mFile->tellg(), mBuffer.size());
        mFile->read(mBuffer.data(), toread);
        setg(mBuffer.data(), mBuffer.data(), mBuffer.data()+mFile->gcount());
    }
    if(gptr() == egptr())
        return traits_type::eof();

    return traits_type::to_int_type(*gptr());
}

ConstrainedFileStreamBuf::pos_type ConstrainedFileStreamBuf::seekoff(off_type offset, std::ios_base::seekdir whence, std::ios_base::openmode mode)
{
    if((mode&std::ios_base::out) || !(mode&std::ios_base::in))
        return traits_type::eof();

    // new file position, relative to mOrigin
    std::streampos newPos = traits_type::eof();
    switch(whence)
    {
        case std::ios_base::beg:
            newPos = offset + mStart;
            break;
        case std::ios_base::cur:
            newPos = offset + mFile->tellg() - (egptr()-gptr());
            break;
        case std::ios_base::end:
            newPos = offset + mEnd;
            break;
        default:
            return traits_type::eof();
    }

    if(newPos < mStart || newPos > mEnd)
        return traits_type::eof();

    if(!mFile->seekg(newPos))
        return traits_type::eof();

    // Clear read pointers so underflow() gets called on the next read attempt.
    setg(0, 0, 0);

    // Type conversion is required for macOS + Clang compatibility
    return static_cast<long long>(newPos) - mStart;
}

ConstrainedFileStreamBuf::pos_type ConstrainedFileStreamBuf::seekpos(pos_type pos, std::ios_base::openmode mode)
{
    if((mode&std::ios_base::out) || !(mode&std::ios_base::in))
        return traits_type::eof();

    if(pos < 0 || pos > (mEnd-mStart))
        return traits_type::eof();

    // Type conversion is required for macOS + Clang compatibility
    if(!mFile->seekg(static_cast<long long>(pos) + mStart))
        return traits_type::eof();

    // Clear read pointers so underflow() gets called on the next read attempt.
    setg(0, 0, 0);

    return pos;
}


} // namespace Archives
