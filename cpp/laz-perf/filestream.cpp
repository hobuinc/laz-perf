#include "filestream.hpp"
#include "excepts.hpp"

namespace lazperf
{

OutFileStream::OutFileStream(std::ostream& out) : f_(out)
{}

// Should probably write to memory to avoid all these calls to a file that can't
// be seen by the compiler.
void OutFileStream::putBytes(const unsigned char *c, size_t len)
{
    f_.write(reinterpret_cast<const char *>(c), len);
}

OutputCb OutFileStream::cb()
{
    using namespace std::placeholders;

    return std::bind(&OutFileStream::putBytes, this, _1, _2);
}

// InFileStream

struct InFileStream::Private
{
    // Setting the offset_ to the buffer size will force a fill on the first read.
    Private(std::istream& in) : f_(in), buf_(1 << 20), offset_(buf_.size())
    {}

    void getBytes(unsigned char *buf, size_t request);
    void fillit();

    std::istream& f_;
    std::vector<unsigned char> buf_;
    size_t offset_;
};

InFileStream::InFileStream(std::istream& in) : p_(new Private(in))
{}

InFileStream::~InFileStream()
{}

// This will force a fill on the next fetch.
void InFileStream::reset()
{
    p_->offset_ = p_->buf_.size();
}

InputCb InFileStream::cb()
{
    using namespace std::placeholders;

    return std::bind(&InFileStream::Private::getBytes, p_.get(), _1, _2);
}

void InFileStream::Private::getBytes(unsigned char *buf, size_t request)
{
    // Almost all requests are size 1.
    if (request == 1)
    {
        if (offset_ >= buf_.size())
            fillit();
        *buf = buf_[offset_++];
    }
    else
    {
        // Use what's left in the buffer, if anything.
        size_t fetchable = (std::min)(buf_.size() - offset_, request);
        std::copy(buf_.data() + offset_, buf_.data() + offset_ + fetchable, buf);
        offset_ += fetchable;
        request -= fetchable;
        if (request)
        {
            fillit();
            std::copy(buf_.data() + offset_, buf_.data() + offset_ + request, buf + fetchable);
            offset_ += request;
        }
    }
}

void InFileStream::Private::fillit()
{
    offset_ = 0;
    f_.read(reinterpret_cast<char *>(buf_.data()), buf_.size());
    buf_.resize(f_.gcount());

    if (buf_.size() == 0)
        throw error("Unexpected end of file.");
}

} // namespace lazperf
