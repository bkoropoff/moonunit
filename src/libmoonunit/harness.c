#include <moonunit/harness.h>

const char*
Mu_TestResultToString(MoonTestResult result)
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
