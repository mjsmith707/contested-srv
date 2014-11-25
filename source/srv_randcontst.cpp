#include "../header/srv_randcontst.h"

SRV_RANDCONTST::SRV_RANDCONTST(Config* config, Logger* log, SRV_DB* database) {
	runningConfig = config;
	runningLog = log;
	runningDB = database;
	running = false;
}

void SRV_RANDCONTST::run() {
	running = true;
	while (running) {
		try {
			runningLog->sendMsg("Updating Random Contests");
			std::string newRand = runningDB->getRandomContests();
			runningConfig->setRandomContests(newRand);
			runningLog->sendMsg("Finished Updating Random Contests");
			boost::this_thread::sleep(boost::posix_time::seconds(60));
		}
		catch (boost::thread_interrupted&) {

		}
	}
	running = false;
}

void SRV_RANDCONTST::stop() {
	running = false;
}
