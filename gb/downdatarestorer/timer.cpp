#include "timer.h"


Timer::Timer() :
	m_isStarted(false), m_logger("clib.timer") {
}

Timer::~Timer() {
	cancel();
}

bool Timer::resetIntervalTime(int32_t intervalSeconds)
{
	if(intervalSeconds>0){
		m_logger.info("resetIntervalTime from[%d] to [%d]",m_intervalSeconds,intervalSeconds);
		m_intervalSeconds=intervalSeconds;
		return true;
	}else{
		m_logger.info("resetIntervalTime: the intervalSeconds is[%d]",intervalSeconds);
		return false;
	}
}

void Timer::start(int32_t intervalSeconds, int32_t delay, int32_t repeatTime) {
	if (!m_isStarted) {
		if (delay >= 0)
			m_delay = delay;
		if (intervalSeconds <= 0)
			return;
		m_intervalSeconds = intervalSeconds;
		if (repeatTime <= 0) {
			m_repeatTime = -1;
		} else {
			m_repeatTime = repeatTime;
		}
		startThd(1);
		m_isStarted = true;
	}
}

void Timer::svc() {
	m_repeatCount = 0;
	if (m_delay > 0)
		sleep(m_delay);
		//usleep(m_delay*1000);
	m_logger.debug("a timer started");
	while (true) {
		if (m_repeatTime == -1 || m_repeatCount < m_repeatTime) {
//			int64_t start = System::currentTimeMillis();
			int64_t start=time(NULL);
			try {
				//LOCK_GUARD guard(m_lockOperation);
				run();
			} catch(std::exception &e) {
				m_logger.error("%s\n%s", e.what());
			}
//			int64_t executeTime = System::currentTimeMillis() - start; // ms
			int64_t executeTime = time(NULL) - start; // s
//			int64_t intervalMillis = m_intervalSeconds*1000; // ms
			
			if(executeTime > m_intervalSeconds/*intervalMillis*/){
				m_logger.warn("timer executed too slow, m_intervalSeconds is %llu, but use time:%llu ms", m_intervalSeconds, executeTime);
				executeTime = executeTime % m_intervalSeconds;
			}
			m_repeatCount ++;
			if ( m_intervalSeconds < 300){
//				int64_t intervalMills = m_intervalSeconds*1000;
//				usleep((intervalMills- executeTime) * 1000);
				sleep(m_intervalSeconds- executeTime);
			} else{
				//sleep(m_intervalSeconds - executeTime/1000);
				sleep(m_intervalSeconds);
			}
		} else {
			break;
		}
	}
	m_isStarted = false;
	m_logger.debug("a timer stoped, repeat count: %d", m_repeatCount);
}
