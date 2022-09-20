#pragma once

// Copyright 2015 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "crypto/ByteSlice.h"
#include "crypto/SHA.h"
#include "util/FileSystemException.h"
#include "util/Fs.h"
#include "util/GlobalChecks.h"
#include "util/Logging.h"
#include "xdrpp/marshal.h"
#include <Tracy.hpp>

#include <fstream>
#include <string>
#include <vector>
#ifdef _WIN32
#include <io.h>
#endif

namespace stellar
{

/**
 * Helper for loading a sequence of XDR objects from a file one at a time,
 * rather than all at once.
 */
class XDRInputFileStream
{
    std::ifstream mIn;
    std::vector<char> mBuf;
    size_t mSizeLimit;
    size_t mSize;

  public:
    XDRInputFileStream(unsigned int sizeLimit = 0)
        : mSizeLimit{sizeLimit}, mSize{0}
    {
    }

    void
    close()
    {
        ZoneScoped;
        mIn.close();
    }

    void
    open(std::string const& filename)
    {
        ZoneScoped;
        mIn.open(filename, std::ifstream::binary);
        if (!mIn)
        {
            std::string msg("failed to open XDR file: ");
            msg += filename;
            msg += ", reason: ";
            msg += std::to_string(errno);
            CLOG_ERROR(Fs, "{}", msg);
            throw FileSystemException(msg);
        }
        mIn.exceptions(std::ios::badbit);
        mSize = fs::size(mIn);
    }

    operator bool() const
    {
        return mIn.good();
    }

    size_t
    size() const
    {
        return mSize;
    }

    size_t
    pos()
    {
        releaseAssertOrThrow(!mIn.fail());

        return mIn.tellg();
    }

    template <typename T>
    bool
    readOne(T& out)
    {
        ZoneScoped;
        char szBuf[4];
        if (!mIn.read(szBuf, 4))
        {
            return false;
        }

        // Read 4 bytes of size, big-endian, with XDR 'continuation' bit cleared
        // (high bit of high byte).
        uint32_t sz = 0;
        sz |= static_cast<uint8_t>(szBuf[0] & '\x7f');
        sz <<= 8;
        sz |= static_cast<uint8_t>(szBuf[1]);
        sz <<= 8;
        sz |= static_cast<uint8_t>(szBuf[2]);
        sz <<= 8;
        sz |= static_cast<uint8_t>(szBuf[3]);

        if (mSizeLimit != 0 && sz > mSizeLimit)
        {
            return false;
        }
        if (sz > mBuf.size())
        {
            mBuf.resize(sz);
        }
        if (!mIn.read(mBuf.data(), sz))
        {
            throw xdr::xdr_runtime_error("malformed XDR file");
        }
        xdr::xdr_get g(mBuf.data(), mBuf.data() + sz);
        xdr::xdr_argpack_archive(g, out);
        return true;
    }
};

// XDROutputFileStream needs access to a file descriptor to do fsync, so we use
// asio's synchronous stream types here rather than fstreams.
class XDROutputFileStream
{
    std::vector<char> mBuf;
    const bool mFsyncOnClose;

#ifdef WIN32
    // Windows implementation assumes calls can't get interrupted
    fs::native_handle_t mHandle;
    FILE* mOut{nullptr};
#else
    // buffered stream
    asio::buffered_write_stream<stellar::fs::stream_t> mBufferedWriteStream;
#endif

  public:
    XDROutputFileStream(asio::io_context& ctx, bool fsyncOnClose)
        : mFsyncOnClose(fsyncOnClose)
#ifndef WIN32
        , mBufferedWriteStream(ctx, stellar::fs::bufsz())
#endif
    {
    }

    ~XDROutputFileStream()
    {
        if (isOpen())
        {
            close();
        }
    }

    bool
    isOpen()
    {
#ifdef WIN32
        return mOut != nullptr;
#else
        return mBufferedWriteStream.next_layer().is_open();
#endif
    }

    fs::native_handle_t
    getHandle()
    {
#ifdef WIN32
        return mHandle;
#else
        return mBufferedWriteStream.next_layer().native_handle();
#endif
    }

    void
    close()
    {
        ZoneScoped;
        if (!isOpen())
        {
            FileSystemException::failWith(
                "XDROutputFileStream::close() on non-open FILE*");
        }
        flush();
        if (mFsyncOnClose)
        {
            fs::flushFileChanges(getHandle());
        }
#ifdef WIN32
        fclose(mOut);
        mOut = nullptr;
#else
        mBufferedWriteStream.close();
#endif
    }

    void
    fdopen(int fd)
    {
#ifdef WIN32
        FileSystemException::failWith(
            "XDROutputFileStream::fdopen() not supported on windows");
#else
        if (isOpen())
        {
            FileSystemException::failWith(
                "XDROutputFileStream::fdopen() on already-open stream");
        }
        mBufferedWriteStream.next_layer().assign(fd);
        if (!isOpen())
        {
            FileSystemException::failWith(
                "XDROutputFileStream::fdopen() failed");
        }
#endif
    }

    void
    flush()
    {
        ZoneScoped;
        if (!isOpen())
        {
            FileSystemException::failWith(
                "XDROutputFileStream::flush() on non-open stream");
        }
#ifdef WIN32
        fflush(mOut);
#else
        asio::error_code ec;
        do
        {
            mBufferedWriteStream.flush(ec);
            if (ec && ec != asio::error::interrupted)
            {
                FileSystemException::failWith(
                    std::string("XDROutputFileStream::flush() failed: ") +
                    ec.message());
            }
        } while (ec);
#endif
    }

    void
    open(std::string const& filename)
    {
        ZoneScoped;
        if (isOpen())
        {
            FileSystemException::failWith(
                "XDROutputFileStream::open() on already-open stream");
        }
        fs::native_handle_t handle = fs::openFileToWrite(filename);
#ifdef WIN32
        mOut = fs::fdOpen(handle);
        if (mOut != NULL)
        {
            mHandle = handle;
        }
        else
        {
            FileSystemException::failWith(
                "XDROutputFileStream::open() could not open file");
        }
#else
        mBufferedWriteStream.next_layer().assign(handle);
#endif
    }

    operator bool()
    {
        return isOpen();
    }

    template <typename T>
    void
    writeOne(T const& t, SHA256* hasher = nullptr, size_t* bytesPut = nullptr)
    {
        ZoneScoped;
        if (!isOpen())
        {
            FileSystemException::failWith(
                "XDROutputFileStream::writeOne() on non-open stream");
        }

        uint32_t sz = (uint32_t)xdr::xdr_size(t);
        releaseAssertOrThrow(sz < 0x80000000);

        if (mBuf.size() < sz + 4)
        {
            mBuf.resize(sz + 4);
        }

        // Write 4 bytes of size, big-endian, with XDR 'continuation' bit set on
        // high bit of high byte.
        mBuf[0] = static_cast<char>((sz >> 24) & 0xFF) | '\x80';
        mBuf[1] = static_cast<char>((sz >> 16) & 0xFF);
        mBuf[2] = static_cast<char>((sz >> 8) & 0xFF);
        mBuf[3] = static_cast<char>(sz & 0xFF);
        xdr::xdr_put p(mBuf.data() + 4, mBuf.data() + 4 + sz);
        xdr_argpack_archive(p, t);

        size_t const to_write = sz + 4;
        size_t written = 0;
        while (written < to_write)
        {
#ifdef WIN32
            auto w = fwrite(mBuf.data() + written, 1, to_write - written, mOut);
            if (w == 0)
            {
                FileSystemException::failWith(
                    std::string("XDROutputFileStream::writeOne() failed"));
            }
            written += w;
#else
            asio::error_code ec;
            auto buf = asio::buffer(mBuf.data() + written, to_write - written);
            written += asio::write(mBufferedWriteStream, buf, ec);
            if (ec)
            {
                if (ec == asio::error::interrupted)
                {
                    continue;
                }
                else
                {
                    FileSystemException::failWith(
                        std::string(
                            "XDROutputFileStream::writeOne() failed: ") +
                        ec.message());
                }
            }
#endif
        }
        if (hasher)
        {
            hasher->add(ByteSlice(mBuf.data(), sz + 4));
        }
        if (bytesPut)
        {
            *bytesPut += (sz + 4);
        }
    }
};
}
