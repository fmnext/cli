#include <iostream>
#include <sstream>
#include <fstream>
#include "DCCManager.hpp"
#include "SQLManager.hpp"
#include "core.h"
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main(uint32_t argc, char* argv[])
{
	std::cout << "\nForzaTech CLI Toolkit v" << DCCManager::GetVersionString() << "-release+" << FT_TOOLKIT_SCM_DATETIME << "\n\n";

    if (argc <= 1)
	{
		std::cout << "See --help for usage and available parameters. \n\n";
	}

    try {

        po::options_description desc("\nOptions");
		desc.add_options()
			("help, h", "produce this help message")
			("input, i", po::value<std::string>(), "set a valid input zip file")
			("output, o", po::value<std::string>(), "set a valid output folder")
			("lod, l", po::value<uint32_t>(), "set level of detail (0 or 1, 2, 3, 4, 5)")
			("geo, g", po::value<uint32_t>(), "set geometry type (0 = tris, 1 = quads)")
			("opt, p", po::value<uint32_t>(), "remove isolated vertices (0 = false, 1 = true)");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) 
		{
            std::cout << "Usage: ";
            std::cout << ".\\cli --input \"path\\to\\NUL_Car_00.zip\" --output \"path\\to\\output\\folder\" --lod 0 --geo 0 --opt 0 \n";
            std::cout << desc << "\n";

            return 0;
        }

        if (vm.count("input") && vm.count("output")) 
		{
            std::filesystem::path input_file(vm["input"].as<std::string>());
			std::filesystem::path output_file(vm["output"].as<std::string>());

			uint32_t lod = vm.count("lod") ? vm["lod"].as<uint32_t>() : 0;
			uint32_t geo = vm.count("geo") ? vm["geo"].as<uint32_t>() : 0;
			uint32_t opt = vm.count("opt") ? vm["opt"].as<uint32_t>() : 0;

			if (!std::filesystem::exists(input_file))
			{
				std::cout << "ERROR: Failed to proceed, the file on input does not exist.\n\n";

				return 1;
			}

			if (input_file.has_extension() && input_file.extension() == ".zip")
			{
				if (!vm.count("lod") && !vm.count("geo") && !vm.count("opt") || !vm.count("lod") || !vm.count("geo") || !vm.count("opt"))
				{
					std::cout << "WARNING: --lod or --geo or --opt or all these parameters are not here, using default --lod 0 --geo 0 --opt 0\n\n";
				}

				if (!vm.count("output")) {
					std::cout << "ERROR: Failed to convert, you must provide a output folder.\n\n";

					return 1;
				}

				if (lod > 6 && geo > 1 && opt > 1 || lod > 6 || geo > 1 || opt > 1)
				{
					std::cout << "ERROR: Failed to convert, a parameter with an invalid range was detected. \n\n";

					return 1;
				}

				const auto start{ std::chrono::steady_clock::now() };

				DCCManager manager = DCCManager(input_file.u8string(), output_file.u8string(), lod, geo, opt);
				if (manager.Init())
				{
					const auto finish{ std::chrono::steady_clock::now() };
					const std::chrono::duration<double> elapsed_seconds{ finish - start };

					std::cout << "\nElapsed time: " << elapsed_seconds.count() << "s \n";
				}

				return 0;
			}
			else {
				std::cout << "ERROR: Input file must be the original game file with '.zip' extension.\n\n";
			}

        }
    }
    catch (std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";

        return 1;
    }
	/**/
	return 1;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
