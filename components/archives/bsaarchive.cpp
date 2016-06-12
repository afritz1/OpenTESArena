
#include "bsaarchive.hpp"

#include <algorithm>
#include <sstream>
#include <fstream>


namespace Archives
{

void BsaArchive::loadNamed(size_t count, std::istream& stream)
{
    std::vector<std::string> names; names.reserve(count);
    std::vector<Entry> entries; entries.reserve(count);

    std::streamsize base = stream.tellg();
    if(!stream.seekg(std::streampos(count) * -18, std::ios_base::end))
        throw std::runtime_error("Failed to seek to archive footer ("+std::to_string(count)+" entries)");
    for(size_t i = 0;i < count;++i)
    {
        std::array<char,13> name;
        stream.read(name.data(), name.size()-1);
        name.back() = '\0'; // Ensure null termination
        std::replace(name.begin(), name.end(), '\\', '/');
        names.push_back(std::string(name.data()));

        int iscompressed = read_le16(stream);
        if(iscompressed != 0)
            throw std::runtime_error("Compressed entries not supported");

        Entry entry;
        entry.mStart = ((i == 0) ? base : entries[i-1].mEnd);
        entry.mEnd = entry.mStart + read_le32(stream);
        entries.push_back(entry);
    }
    if(!stream.good())
        throw std::runtime_error("Failed reading archive footer");

    for(const std::string &name : names)
    {
        auto iter = std::lower_bound(mLookupName.begin(), mLookupName.end(), name);
        if(iter == mLookupName.end() || *iter != name)
            mLookupName.insert(iter, name);
    }
    mEntries.resize(mLookupName.size());
    for(size_t i = 0;i < count;++i)
    {
        auto iter = std::find(mLookupName.cbegin(), mLookupName.cend(), names[i]);
        mEntries[std::distance(mLookupName.cbegin(), iter)] = entries[i];
    }
}

void BsaArchive::load(const std::string &fname)
{
    mFilename = fname;

    std::ifstream stream(mFilename.c_str(), std::ios::binary);
    if(!stream.is_open())
        throw std::runtime_error("Failed to open "+mFilename);

    size_t count = read_le16(stream);

    mEntries.reserve(count);
    loadNamed(count, stream);
}

IStreamPtr BsaArchive::open(const Entry &entry)
{
    std::unique_ptr<std::istream> stream(new std::ifstream(mFilename.c_str(), std::ios::binary));
    if(!stream->seekg(entry.mStart))
        return IStreamPtr(nullptr);
    return IStreamPtr(new ConstrainedFileStream(std::move(stream), entry.mStart, entry.mEnd));
}

IStreamPtr BsaArchive::open(const char *name)
{
    auto iter = std::lower_bound(mLookupName.begin(), mLookupName.end(), name);
    if(iter == mLookupName.end() || *iter != name)
        return IStreamPtr(nullptr);
    return open(mEntries[std::distance(mLookupName.begin(), iter)]);
}

bool BsaArchive::exists(const char *name) const
{
    return std::binary_search(mLookupName.begin(), mLookupName.end(), name);
}

} // namespace Archives
