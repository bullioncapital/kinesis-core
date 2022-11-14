// Copyright 2014 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "crypto/CryptoError.h"
#include "invariant/InvariantDoesNotHold.h"
#include "ledger/NonSociRelatedException.h"
#include "main/CommandLine.h"
#include "util/Backtrace.h"
#include "util/FileSystemException.h"
#include "util/Logging.h"

#include "crypto/ShortHash.h"
#include "util/RandHasher.h"
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <sodium/core.h>
#include <system_error>
#include <xdrpp/marshal.h>

namespace stellar
{
static void
printCurrentException()
{
    std::exception_ptr eptr = std::current_exception();
    if (eptr)
    {
        try
        {
            std::rethrow_exception(eptr);
        }
        catch (NonSociRelatedException const& e)
        {
            fprintf(stderr,
                    "current exception: NonSociRelatedException(\"%s\")\n",
                    e.what());
        }
        catch (CryptoError const& e)
        {
            fprintf(stderr, "current exception: CryptoError(\"%s\")\n",
                    e.what());
        }
        catch (FileSystemException const& e)
        {
            fprintf(stderr, "current exception: FileSystemException(\"%s\")\n",
                    e.what());
        }
        catch (InvariantDoesNotHold const& e)
        {
            fprintf(stderr, "current exception: InvariantDoesNotHold(\"%s\")\n",
                    e.what());
        }
        catch (std::filesystem::filesystem_error const& e)
        {
            fprintf(stderr,
                    "current exception: std::filesystem::filesystem_error(%d, "
                    "\"%s\", \"%s\", \"%s\", \"%s\")\n",
                    e.code().value(), e.code().message().c_str(), e.what(),
                    e.path1().string().c_str(), e.path2().string().c_str());
        }
        catch (std::system_error const& e)
        {
            fprintf(
                stderr,
                "current exception: std::system_error(%d, \"%s\", \"%s\")\n",
                e.code().value(), e.code().message().c_str(), e.what());
        }
        catch (std::domain_error const& e)
        {
            fprintf(stderr, "current exception: std::domain_error(\"%s\")\n",
                    e.what());
        }
        catch (std::invalid_argument const& e)
        {
            fprintf(stderr,
                    "current exception: std::invalid_argument(\"%s\")\n",
                    e.what());
        }
        catch (std::length_error const& e)
        {
            fprintf(stderr, "current exception: std::length_error(\"%s\")\n",
                    e.what());
        }
        catch (std::out_of_range const& e)
        {
            fprintf(stderr, "current exception: std::out_of_range(\"%s\")\n",
                    e.what());
        }
        catch (std::range_error const& e)
        {
            fprintf(stderr, "current exception: std::range_error(\"%s\")\n",
                    e.what());
        }
        catch (std::overflow_error const& e)
        {
            fprintf(stderr, "current exception: std::overflow_error(\"%s\")\n",
                    e.what());
        }
        catch (std::underflow_error const& e)
        {
            fprintf(stderr, "current exception: std::underflow_error(\"%s\")\n",
                    e.what());
        }
        catch (std::logic_error const& e)
        {
            fprintf(stderr, "current exception: std::logic_error(\"%s\")\n",
                    e.what());
        }
        catch (std::runtime_error const& e)
        {
            fprintf(stderr, "current exception: std::runtime_error(\"%s\")\n",
                    e.what());
        }
        catch (std::exception const& e)
        {
            fprintf(stderr, "current exception: std::exception(\"%s\")\n",
                    e.what());
        }
        catch (...)
        {
            fprintf(stderr, "current exception: unknown\n");
        }
        fflush(stderr);
    }
}

static void
printBacktraceAndAbort()
{
    printCurrentException();
    printCurrentBacktrace();
    std::abort();
}

static void
outOfMemory()
{
    std::fprintf(stderr, "Unable to allocate memory\n");
    std::fflush(stderr);
    printBacktraceAndAbort();
}
}

int
main(int argc, char* const* argv)
{
    using namespace stellar;
    BacktraceManager btGuard;

    // Abort when out of memory
    std::set_new_handler(outOfMemory);
    // At least print a backtrace in any circumstance
    // that would call std::terminate
    std::set_terminate(printBacktraceAndAbort);

    Logging::init();
    if (sodium_init() != 0)
    {
        LOG_FATAL(DEFAULT_LOG, "Could not initialize crypto");
        return 1;
    }
    shortHash::initialize();
    randHash::initialize();
    xdr::marshaling_stack_limit = 1000;

    return handleCommandLine(argc, argv);
}
