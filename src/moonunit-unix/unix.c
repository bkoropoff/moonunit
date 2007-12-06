#include <moonunit/plugin.h>

#include <stdlib.h>

extern struct MoonUnitLoader mu_unixloader;
extern struct MoonUnitHarness mu_unixharness;
extern struct MoonUnitRunner* Mu_UnixRunner_Create(const char*, 
                                                   struct MoonUnitLoader*, 
                                                   struct MoonUnitHarness*, 
                                                   struct MoonUnitLogger*);

static struct MoonUnitLoader*
create_unixloader()
{
    return &mu_unixloader;
}

static struct MoonUnitHarness*
create_unixharness()
{
    return &mu_unixharness;
}

static struct MoonUnitRunner*
create_unixrunner(const char* self, struct MoonUnitLoader* loader, 
                  struct MoonUnitHarness* harness, struct MoonUnitLogger* logger)
{
    return Mu_UnixRunner_Create(self ,loader, harness, logger);
}

static MoonUnitPlugin plugin =
{
    .name = "unix",
    .create_loader = create_unixloader,
    .create_harness = create_unixharness,
    .create_runner = create_unixrunner,
    .create_logger = NULL
};

MU_PLUGIN_INIT
{
    return &plugin;
}
