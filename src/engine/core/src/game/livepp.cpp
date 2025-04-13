#include "halley/game/livepp.h"

#ifdef HAS_LIVEPP

#include "LPP_API_x64_CPP.h"

static lpp::LppSynchronizedAgent lppAgent;

void Halley::setupLivePP()
{
    lppAgent = lpp::LppCreateSynchronizedAgentANSI(nullptr, LIVEPP_PATH);
    if (LppIsValidSynchronizedAgent(&lppAgent)) {
		lppAgent.EnableModule(lpp::LppGetCurrentModulePath(), lpp::LPP_MODULES_OPTION_NONE, nullptr, nullptr);
	}
}

void Halley::shutdownLivePP()
{
	LppDestroySynchronizedAgent(&lppAgent);
}

void Halley::updateLivePP()
{
    if (LppIsValidSynchronizedAgent(&lppAgent)) {
	    if (lppAgent.WantsReload(lpp::LPP_RELOAD_OPTION_SYNCHRONIZE_WITH_RELOAD)) {
	        lppAgent.Reload(lpp::LPP_RELOAD_BEHAVIOUR_WAIT_UNTIL_CHANGES_ARE_APPLIED);
	    }

	    if (lppAgent.WantsRestart()) {
	        lppAgent.Restart(lpp::LPP_RESTART_BEHAVIOUR_INSTANT_TERMINATION, 0u, nullptr);
	    }
    }
}

void Halley::requestRecompileLivePP()
{
    if (LppIsValidSynchronizedAgent(&lppAgent)) {
		lppAgent.ScheduleReload();
	}
}

#else

void Halley::setupLivePP()
{
}

void Halley::shutdownLivePP()
{
}

void Halley::updateLivePP()
{
}

void Halley::requestRecompileLivePP()
{
	
}

#endif
