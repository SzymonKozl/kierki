//
// Created by szymon on 15.05.24.
//

#include "server.h"
#include "utils.h"

#include "unordered_map"

int main() {
    Server server(parseScenario("/home/szymon/CLionProjects/kierki/scenarios/simple.scn"));
    server.run();
}
