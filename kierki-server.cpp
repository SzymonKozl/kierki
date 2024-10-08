//
// Created by szymon on 15.05.24.
//

#include "server/server.h"
#include "common/utils.h"

#include "vector"
#include "string"
#include "cstdint"
#include "boost/program_options.hpp"

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
    int status;
    {
        po::options_description desc("Allowed options");
        desc.add_options()
                ("port,p", po::value<std::vector<uint16_t>>(),
                        "port to listen on")
                ("file,f", po::value<std::vector<std::string>>()->required(),
                        "configuration file")
                ("timeout,t", po::value<std::vector<int>>(),
                        "listen timeout");
        po::positional_options_description posDesc;
        po::variables_map vm;
        try {
            po::store(po::command_line_parser(argc, argv).options(desc).
                positional(posDesc).run(), vm);
        } catch (std::exception& e) {
            std::cerr << e.what() << '\n';
            std::cerr << "usage: " << argv[0] <<
                " -p <port> -f <game scenario file> [-t timeout]";
            exit(1);
        }
        po::notify(vm);
        std::vector<uint16_t> ports;
        std::string file;
        std::vector<int> timeouts;
        if (vm.contains("port")) {
            ports = vm["port"].as<std::vector<uint16_t>>();
        }
        if (vm.contains("file")) {
            file = vm["file"].as<std::vector<std::string>>()[0];
        }
        if (vm.contains("timeout")) {
            timeouts = vm["timeout"].as<std::vector<int>>();
        }
        ignoreBrokenPipe();
        Server server(parseScenario(file), (ports.empty()) ? 0 : ports[0],
                      (timeouts.empty()) ? DEFAULT_TIMEOUT : timeouts[0]);
        status = server.run();
    }
    exit(status);
}
