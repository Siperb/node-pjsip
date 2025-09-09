#include <pjsua2.hpp>
#include <iostream>

int main() {
    try {
        pj::Endpoint ep;
        ep.libCreate();
        std::cout << "libCreate OK" << std::endl;

        pj::EpConfig cfg;
        cfg.logConfig.level = 4;
        cfg.logConfig.consoleLevel = 4;
        cfg.uaConfig.maxCalls = 4;
        cfg.uaConfig.threadCnt = 0;
        cfg.uaConfig.mainThreadOnly = true;

        ep.libInit(cfg);
        std::cout << "libInit OK" << std::endl;

        ep.libDestroy();
        std::cout << "libDestroy OK" << std::endl;
    } catch (pj::Error& err) {
        std::cerr << "PJSUA2 ERROR: " << err.info() << std::endl;
        return 1;
    }

    return 0;
}