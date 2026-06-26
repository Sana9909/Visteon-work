#ifndef TEST_LOADER_H
#define TEST_LOADER_H

#include "types.h"

/* Test case definition */
typedef struct {
    char name[64];
    int speed;
    int temperature;
    int gear;
    VehicleMode mode;
    int repeat;
} TestScenario;

/* Load test cases from file */
int load_test_cases(const char *filename,
                    TestScenario *cases,
                    int max_cases);

#endif /* TEST_LOADER_H */