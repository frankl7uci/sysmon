#include "gpu.h"

#include "windows.h"

#include <pdh.h>
#include <pdhmsg.h>

float get_gpu(void)
{
    static PDH_HQUERY query = NULL;
    static PDH_HCOUNTER counter;
    static int initialized = 0;

    if (!initialized) {
        if (PdhOpenQuery(NULL, 0, &query) != ERROR_SUCCESS)
            return 0.0f;

        //Total GPU utilization (seems to only work with Windows 10 and newer)

        if (PdhAddEnglishCounterA(
                query,
                "\\GPU Engine(*)\\Utilization Percentage",
                0,
                &counter) != ERROR_SUCCESS)
        {
            return 0.0f;
        }

        PdhCollectQueryData(query);

        initialized = 1;
        return 0.0f;
    }

    if (PdhCollectQueryData(query) != ERROR_SUCCESS)
        return 0.0f;

    DWORD buf_size = 0;
    DWORD item_count = 0;

    PDH_STATUS status = PdhGetFormattedCounterArrayA(
        counter,
        PDH_FMT_DOUBLE,
        &buf_size,
        &item_count,
        NULL
    );

    if (status != PDH_MORE_DATA)
        return 0.0f;

    PDH_FMT_COUNTERVALUE_ITEM_A *items =
        (PDH_FMT_COUNTERVALUE_ITEM_A*)malloc(buf_size);

    if (!items)
        return 0.0f;

    status = PdhGetFormattedCounterArrayA(
        counter,
        PDH_FMT_DOUBLE,
        &buf_size,
        &item_count,
        items
    );

    if (status != ERROR_SUCCESS) {
        free(items);
        return 0.0f;
    }

    double total = 0.0;

    for (DWORD i = 0; i < item_count; i++) {

        //Sum everything

        if (items[i].FmtValue.CStatus == ERROR_SUCCESS)
            total += items[i].FmtValue.doubleValue;
    }

    free(items);

    // Limit number to 100 for display even when usage can exceed 100%

    if (total > 100.0)
        total = 100.0;

    return (float)total;
}