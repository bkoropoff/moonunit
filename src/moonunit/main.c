#include <gib/test.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>

static struct
{
    int on;
    int force;
    const char* self;
    const char* command;
} debug;

static struct
{
    const char* command;
} symbols;

typedef enum
{
    RESULT_NORMAL = 0,
    RESULT_FAILED = 1,
    RESULT_SEGFAULT = 2
} result;

static const char*
result_name(result r)
{
    switch (r)
    {
    case RESULT_NORMAL:
        return "normal";
    case RESULT_FAILED:
        return "failure";
    case RESULT_SEGFAULT:
        return "segfault";
    }
}

typedef enum
{
    POSITION_FINISHED = 0,
    POSITION_SETUP = 1,
    POSITION_TEST = 2,
    POSITION_TEARDOWN = 3,
} position;

static const char*
position_name(position p)
{
    switch (p)
    {
    case POSITION_SETUP:
        return "setup";
    case POSITION_TEST:
        return "test";
    case POSITION_TEARDOWN:
        return "teardown";
    case POSITION_FINISHED:
        return "end";
    }
}

static const char* 
va(const char* format, ...)
{
    static char buffer[2048];
    va_list ap;

    va_start(ap, format);

    vsnprintf(buffer, sizeof(buffer)-1, format, ap);
    
    buffer[sizeof(buffer)-1] = 0;

    return buffer;
}

static const char*
chomp_filename(const char* filename)
{
    char* slash;
    if ((slash = strrchr(filename, '/')))
        return slash+1;
    else
        return filename;
}

static void
config_set_defaults()
{
    debug.on = debug.force = 0;
    debug.command = 
        "echo 'break *%3$p' > .gdb-gibtest-%2$i;"
        "echo 'signal SIGCONT' >> .gdb-gibtest-%2$i;"
        "gdb '%1$s' %2$i -q -x .gdb-gibtest-%2$i;"
        "rm .gdb-gibtest-%2$i";
    symbols.command = 
        "nm %1$s | grep __gib_test_t_ | cut -d' ' -f3";   
}

static GibUnitTest**
build_test_array(const char* filename, void *handle)
{
    unsigned int capacity = 32;
    GibUnitTest** tests = malloc(capacity * sizeof(*tests));
    unsigned int i = 0;
    char buffer[1024];
    FILE* pipe;

    pipe = popen(va(symbols.command, filename), "r");
    
    while (fgets(buffer, sizeof(buffer), pipe))
    {
        GibUnitTest* test;
        unsigned int len = strlen(buffer);

        if (buffer[len-1] == '\n')
            buffer[len-1] = 0;

        test = (GibUnitTest*) dlsym(handle, buffer);
        test->library = chomp_filename(filename);

        tests[i++] = test;

        if (i == capacity-1)
        {
            capacity *= 2;
            tests = realloc(tests, sizeof(*tests) * capacity);
        }
    }

    tests[i] = NULL;

    return tests;
}

static int
compare_test (const void* _a, const void* _b)
{
    GibUnitTest* a = *(GibUnitTest**) _a;
    GibUnitTest* b = *(GibUnitTest**) _b;

    int result;

    if (b && !a)
        return -1;
    else if (a && !b)
        return 1;
    else if (!a && !b)
        return 0;
    else if ((result = strcmp(a->suite, b->suite)))
        return result;
    else if (_a < _b)
        return -1;
    else if (_b > _a)
        return 1;
    else return 0;
}

static void
sort_test_array(GibUnitTest** tests)
{
    unsigned int count;
    for (count = 0; tests[count]; count++);
    qsort(tests, count, sizeof(*tests), compare_test);
}

static position test_position;

static void segfault_handler(int signum)
{
    exit (test_position | (RESULT_SEGFAULT << 4));
}

static void
dispatch_test(GibUnitTest* test, 
              void (*fixture_setup)(void),
              void (*fixture_teardown)(void),
              result* res, position *pos)
{
    pid_t pid;
    if ((pid = fork()) == 0)
    {
        int result;
            
        signal(SIGSEGV, segfault_handler);

        test_position = POSITION_SETUP;
        if (fixture_setup) fixture_setup();
        test_position = POSITION_TEST;
        result = test->function();
        test_position = POSITION_TEARDOWN;
        if (fixture_teardown) fixture_teardown();

        exit(result ? 
             (POSITION_FINISHED | (RESULT_NORMAL << 4))
             : (POSITION_TEST | (RESULT_FAILED << 4)));
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
        
        status = WEXITSTATUS(status);

        *res = status >> 4;
        *pos = status & 0xf;
    }
}

static void
debug_test(GibUnitTest* test, 
           void (*fixture_setup)(void),
           void (*fixture_teardown)(void),
           position pos)
{
    pid_t test_pid;

    if ((test_pid = fork()) == 0)
    {
        sleep(999);

        if (fixture_setup) fixture_setup();
        test->function();
        if (fixture_teardown) fixture_teardown();

        exit(0);
    }
    else
    {
        void* breakpoint;

        switch (pos)
        {
        case POSITION_SETUP:
            breakpoint = (void*) fixture_setup;
            break;
        case POSITION_TEST:
        default:
            breakpoint = (void*) test->function;
            break;
        case POSITION_TEARDOWN:
            breakpoint = (void*) fixture_teardown;
            break;
        }

        system(va(debug.command, debug.self, test_pid, breakpoint));
    }
}

int get_answer()
{
    char buffer[1024];
    if ((fgets(buffer, sizeof(buffer), stdin)))
    {
        char c;

        sscanf(buffer, " %c ", &c);

        return (c == 'y' || c == 'Y');
    }

    return 0;
}

#define die(...)                                \
    do {                                        \
        fprintf(stderr, __VA_ARGS__);           \
        exit(1);                                \
    } while (0);                                \

int main (int argc, char** argv)
{
    void* handle;
    void (*library_setup)(void);
    void (*library_teardown)(void);
    GibUnitTest** tests;
    int i, n;
    int c;
    const char* last_suite = "";
    struct 
    {
        unsigned int libraries;
        unsigned int suites;
        unsigned int tests;
        unsigned int failed;
    } summary = {0,0,0,0};
           

    config_set_defaults();

    while ((c = getopt(argc, argv, "gf")) != -1)
        switch (c)
        {
        case 'g':
            debug.on = 1;
            debug.self = argv[0];
            break;
        case 'f':
            debug.force = 1;
            break;
        default:
            abort();
        }

    if (argc - optind < 1)
        die("No libraries specified");

    for (i = optind; i < argc; i++)
    {
        summary.libraries++;
        handle = dlopen(argv[i], RTLD_LAZY);

        if (!handle)
            die("Could not dlopen() %s\n", argv[1]);

        if ((library_setup = dlsym(handle, "__gib_test_ls")))
            library_setup();

        tests = build_test_array(argv[i], handle);
        sort_test_array(tests);

        printf("- Library: %s\n", chomp_filename(argv[i]));
        
        for (n = 0; tests[n]; n++)
        {
            GibUnitTest* test = tests[n];
            if (test)
            {
                pid_t pid;
                int new_suite = 0;
                void (*fixture_setup)(void);
                void (*fixture_teardown)(void);
                result res;
                position pos;

                summary.tests++;

                if (strcmp(last_suite, test->suite))
                {
                    summary.suites++;
                    new_suite = 1;
                    last_suite = test->suite;
                }
                
                if (new_suite)
                {
                    printf("  -- Suite: %s\n", test->suite);
                    fixture_setup = dlsym(handle, va("__gib_test_fs_%s", test->suite));
                    fixture_teardown = dlsym(handle, va("__gib_test_ft_%s", test->suite));
                }
                
                dispatch_test(test, fixture_setup, fixture_teardown, &res, &pos);
                
                printf("     --- %s (%s:%i): %s\n",
                       test->name, chomp_filename(test->file), test->line,
                       (res == RESULT_NORMAL ? 
                        "PASSED" : 
                        va("FAILED (%s in %s)", result_name(res), position_name(pos))));
                
                if (res)
                    summary.failed++;

                if (debug.on && (res || debug.force))
                {
                    char answer;
                    printf("Do you wish to debug this test? (y/n): ");
                    if (get_answer())
                        debug_test(test, fixture_setup, fixture_teardown, pos);
                }
            }
        }
        
        if ((library_teardown = dlsym(handle, "__gib_test_lt")))
            library_teardown();
        
        dlclose(handle);
    }

    printf("\n");
    printf("%u suites(s) with %u test(s) in %u library(s), %u failures\n",
           summary.suites, summary.tests, summary.libraries, summary.failed);

    return summary.failed;
}
