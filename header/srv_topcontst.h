#ifndef SRV_TOPCONTST_H_
#define SRV_TOPCONTST_H_

#include "../header/constants.h"
#include "../header/config.h"
#include "../header/logger.h"
#include "../header/srv_db.h"
#include <boost/thread.hpp>
#include <atomic>

class SRV_TOPCONTST {
	public:
		SRV_TOPCONTST(Config*, Logger*, SRV_DB*);
		void run();
		void stop();
	private:
		Config* runningConfig;
		Logger* runningLog;
		SRV_DB* runningDB;
		std::atomic<bool> running;
};

#endif
