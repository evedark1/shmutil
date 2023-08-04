#include "utest.h"

UTEST_STATE();

int main(int argc, const char *const argv[])
{
    srand(time(NULL));
    return utest_main(argc, argv);
}