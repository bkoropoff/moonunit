#ifndef __MU_RUNNER_H__
#define __MU_RUNNER_H__

struct MoonUnitRunner;
struct MoonUnitHarness;
struct MoonUnitLoader;
struct MoonUnitLogger;

typedef struct MoonUnitRunner MoonUnitRunner;

MoonUnitRunner* Mu_Runner_New(struct MoonUnitLoader* loader, struct MoonUnitHarness* harness, struct MoonUnitLogger* logger);
void Mu_Runner_RunTests(MoonUnitRunner* runner, const char* path);

#endif
