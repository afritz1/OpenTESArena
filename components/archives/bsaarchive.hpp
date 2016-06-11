#ifndef COMPONENTS_ARCHIVES_BSAARCHIVE_HPP
#define COMPONENTS_ARCHIVES_BSAARCHIVE_HPP

#include <iostream>
#include <string>
#include <vector>
#include <set>

#include "archive.hpp"


namespace Archives
{

class BsaArchive : public Archive {
    std::vector<std::string> mLookupName;

    struct Entry {
        std::streamsize mStart;
        std::streamsize mEnd;
    };
    std::vector<Entry> mEntries;

    std::string mFilename;

    void loadNamed(size_t count, std::istream &stream);

    IStreamPtr open(const Entry &entry);

public:
    void load(const std::string &fname);

    virtual IStreamPtr open(const char *name);

    virtual bool exists(const char *name) const;

    virtual const std::vector<std::string> &list() const final
    { return mLookupName; };
};

} // namespace Archives

#endif /* COMPONENTS_ARCHIVES_BSAARCHIVE_HPP */
