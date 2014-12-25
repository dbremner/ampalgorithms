/*----------------------------------------------------------------------------
* Copyright © Microsoft Corp.
*
* Licensed under the Apache License, Version 2.0 (the "License"); you may not 
* use this file except in compliance with the License.  You may obtain a copy 
* of the License at http://www.apache.org/licenses/LICENSE-2.0  
* 
* THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED 
* WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE, 
* MERCHANTABLITY OR NON-INFRINGEMENT. 
*
* See the Apache Version 2.0 License for specific language governing 
* permissions and limitations under the License.
*---------------------------------------------------------------------------
* 
* C++ AMP standard algorithm library.
*
* This file contains unit tests.
*---------------------------------------------------------------------------*/

#include "stdafx.h"
#include "testtools.h"

using namespace concurrency;

bool configure_tests(int argc, char **argv);
void usage();

int main(int argc, char **argv)
{
    std::cout << std::endl << "C++ AMP Algorithms Library (";
#if defined(_DEBUG)
    std::cout << "DEBUG";
#else
    std::cout << "RELEASE";
#endif
    std::cout << " build)" << std::endl << std::endl;

    ::testing::InitGoogleTest(&argc, argv);

    if (!configure_tests(argc, argv))
    {
        return 1;
    }

    return RUN_ALL_TESTS();
}

bool configure_tests(int argc, char **argv)
{
    if (argc == 0)
    {
        return true;
    }

    auto accls = testtools::get_accelerators(false);

    bool is_quiet = false;
    bool is_verbose = false;
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = std::string(argv[i]);

        if (arg == "-h")
        {
            usage();
            return false;
        }
        if ((arg == "-v") || (arg == "--verbose"))
        {
            is_verbose = true;
            continue;
        }
        if ((arg == "-q") || (arg == "--quiet"))
        {
            is_quiet = true;
            continue;
        }

        if (((arg == "-d") || (arg == "--device")) && (i < (argc - 1)))
        {
            try
            {
                arg = std::string(argv[++i]);
                if (arg == "ref")
                {
                    testtools::set_default_accelerator(accelerator::direct3d_ref);
                    continue;
                }
                size_t d = std::stoi(argv[i]) - 1;
                if ((d >= 0) && (d < accls.size()))
                {
                    testtools::set_default_accelerator(accls[d].device_path);
                    continue;
                }
            }
            catch (std::invalid_argument)
            {
            }
            catch (std::out_of_range)
            {
            }
        }
        std::cout << "Invalid argument '" << arg << "'." << std::endl;
        usage();
        return false;
    }

    if (!is_quiet)
    {
        testtools::display_accelerators(accls, is_verbose);
    }
    return true;
}

void usage()
{
    std::wcout << std::endl
        << "Usage: amp_algorithms.exe ... googletest options ... amp_algorithms options ..." << std::endl << std::endl;

    std::wcout 
        << "  -d [d]          Run tests on a selected device. Where d is the integer device number as listed in the test output. " << std::endl
        << "  --device [d]    Setting d to ref will select the reference accelerator. " << std::endl
        << "  -q              Minimal output showing only the build configuration and test output." << std::endl
        << "  --quiet         " << std::endl
        << "  -v              Verbose output showing properties for all C++ AMP devices. The device being used for running tests" << std::endl
        << "  --verbose       is denoted with a `*`." << std::endl << std::endl;
}
