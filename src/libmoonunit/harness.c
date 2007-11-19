#include <moonunit/harness.h>

const char*
Mu_TestResultToString(MoonUnitTestResult result)
{
	switch (result)
	{
		case MOON_RESULT_SUCCESS:
			return "success";
		case MOON_RESULT_FAILURE:
			return "failure";
    	case MOON_RESULT_ASSERTION:
    		return "assertion failure";
    	case MOON_RESULT_CRASH:
    		return "crash";
    	default:
    		return "unknown";
	}
}

const char*
Mu_TestStageToString(MoonUnitTestStage stage)
{
	switch (stage)
	{
		case MOON_STAGE_SETUP:
			return "setup";
		case MOON_STAGE_TEST:
			return "test";
		case MOON_STAGE_TEARDOWN:
			return "teardown";
		case MOON_STAGE_UNKNOWN:	
		default:
			return "unknown stage";
	}
}
