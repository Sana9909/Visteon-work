#include <stdio.h>
#include <string.h>
#include "test_loader.h"
#include "log.h"

/* Convert mode string to enum */
static VehicleMode parse_mode(const char *str)
{
    if (strcmp(str, "OFF") == 0) return MODE_OFF;
    if (strcmp(str, "ACC") == 0) return MODE_ACC;
    if (strcmp(str, "IGNITION_ON") == 0) return MODE_IGNITION_ON;
    return MODE_FAULT;
}

int load_test_cases(const char *filename,
                    TestScenario *cases,
                    int max_cases)
{
    FILE *fp;
    char line[256];
    int count = 0;

    fp = fopen(filename, "r");
    if (!fp) {
        log_warning("Failed to open test case file.");
        return 0;
    }

    while (fgets(line, sizeof(line), fp) && count < max_cases) {

        /* Skip empty lines and comments */
        if (line[0] == '#' || line[0] == '\n')
            continue;

        char mode_str[32];

        if (sscanf(line,
                   "%63[^,],%d,%d,%d,%31[^,],%d",
                   cases[count].name,
                   &cases[count].speed,
                   &cases[count].temperature,
                   &cases[count].gear,
                   mode_str,
                   &cases[count].repeat) != 6) {
            log_warning("Malformed test case line skipped.");
            continue;
        }

        cases[count].mode = parse_mode(mode_str);
        count++;
    }

    fclose(fp);

    log_info("Test cases loaded successfully.");
    return count;
}