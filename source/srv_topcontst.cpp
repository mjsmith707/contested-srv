#include "../header/srv_topcontst.h"

SRV_TOPCONTST::SRV_TOPCONTST(Config* config, Logger* log, SRV_DB* database) {
	runningConfig = config;
	runningLog = log;
	runningDB = database;
	running = false;
}

void SRV_TOPCONTST::run() {
	running = true;
	while (running) {
		try {
			runningLog->sendMsg("Updating Top Contests");
			std::string newTop = runningDB->getTopContests();
			runningConfig->setTopContests(newTop);
			runningLog->sendMsg("Finished Updating Top Contests");
			boost::this_thread::sleep(boost::posix_time::seconds(60));
		}
		catch (boost::thread_interrupted&) {

		}
	}
	running = false;
}

void SRV_TOPCONTST::stop() {
	running = false;
}
