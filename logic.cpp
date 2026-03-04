#include "logic.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

int compareDataPoints(const void* a, const void* b) {
    double valueA = ((const RowData*)a)->value;
    double valueB = ((const RowData*)b)->value;
    return (valueA > valueB) - (valueA < valueB);
}

void initializeContext(AppContext* context) {
    memset(context, 0, sizeof(AppContext));
}

bool loadDataFromFile(AppContext* context) {
    FILE* file = fopen(context->currentFilePath, "r");
    if (!file) return false;

    freeResources(context);

    if (!parseHeader(context, file)) {
        fclose(file);
        return false;
    }

    if (!parseRows(context, file)) {
        fclose(file);
        return false;
    }

    fclose(file);
    context->fileLoaded = true;
    return true;
}

void filterData(AppContext* context) {
    if (!context->fileLoaded || context->filtredParams.selectedColumn > context->tableData.cols) return;

    if (context->metrics.relevantRows) {
        free(context->metrics.relevantRows);
        context->metrics.relevantRows = NULL;
    }

    int count = countMatchingRows(context);
    if (count == 0){
        context->dataFiltered = false;
        return;
    }

    context->metrics.relevantRows = (RowData*)malloc(count * sizeof(RowData));
    if (!context->metrics.relevantRows) return;

    copyAndFillRelevantRowsForMetrics(context);

    context->metrics.count = count;
    context->dataFiltered = true;

    if (count > 0) {
        qsort(context->metrics.relevantRows, count, sizeof(RowData), compareDataPoints);

        context->metrics.min = context->metrics.relevantRows[0].value;
        context->metrics.max = context->metrics.relevantRows[0].value;

        for (int i = 0; i < count; i++) {
            double val = context->metrics.relevantRows[i].value;
            if (val < context->metrics.min) context->metrics.min = val;
            if (val > context->metrics.max) context->metrics.max = val;
        }

        if (count % 2 == 1) {
            context->metrics.median = context->metrics.relevantRows[count/2].value;
        } else {
            context->metrics.median = (context->metrics.relevantRows[count/2 - 1].value +
                                            context->metrics.relevantRows[count/2].value) / 2.0;
        }
    }
}

void freeResources(AppContext* context) {
    if (!context) return;

    if (context->tableData.headers) {
        for (int i = 0; i < context->tableData.cols; i++) {
            if (context->tableData.headers[i]) {
                free(context->tableData.headers[i]);
            }
        }
        free(context->tableData.headers);
        context->tableData.headers = NULL;
    }

    if (context->tableData.matrix) {
        for (int i = 0; i < context->tableData.rows; i++) {
            if (context->tableData.matrix[i]) {
                for (int j = 0; j < context->tableData.cols; j++) {
                    if (context->tableData.matrix[i][j]) {
                        free(context->tableData.matrix[i][j]);
                    }
                }
                free(context->tableData.matrix[i]);
            }
        }
        free(context->tableData.matrix);
        context->tableData.matrix = NULL;
    }

    if (context->tableData.validRows) {
        free(context->tableData.validRows);
        context->tableData.validRows = NULL;
    }

    if (context->metrics.relevantRows) {
        free(context->metrics.relevantRows);
        context->metrics.relevantRows = NULL;
    }

    context->tableData.rows = 0;
    context->tableData.cols = 0;
    context->tableData.validRowsCount = 0;
    context->tableData.invalidRows = 0;
    context->metrics.count = 0;
    context->fileLoaded = false;
    context->dataFiltered = false;
    context->currentFilePath[0] = '\0';
}

void allocateMatrix(AppContext *context){
    context->tableData.matrix = (char***)malloc(MAX_ROWS * sizeof(char**));
}

bool parseHeader(AppContext* context, FILE* file) {
    char line[4096];
    if (!fgets(line, sizeof(line), file)) return false;

    // Подсчет количества столбцов
    char* token = strtok(line, ",");
    int colCount = 0;

    while (token) {
        colCount++;
        token = strtok(NULL, ",");
    }

    context->tableData.cols = colCount;
    rewind(file);

    context->tableData.headers = (char**)malloc(colCount * sizeof(char*));
    fgets(line, sizeof(line), file);

    token = strtok(line, ",");
    for (int i = 0; i < colCount && token; i++) {
        context->tableData.headers[i] = strdup(token);
        token = strtok(NULL, ",");
    }

    return true;
}

bool parseRows(AppContext* context, FILE* file) {
    char line[4096];
    int row = 0;

    rewind(file);
    // Пропускаем строку заголовков
    fgets(line, sizeof(line), file);

    allocateMatrix(context);
    context->tableData.validRows = (RowData*)malloc(MAX_ROWS * sizeof(RowData));

    while (fgets(line, sizeof(line), file) && row < MAX_ROWS) {
        line[strcspn(line, "\n")] = '\0';
        context->tableData.matrix[row] = (char**)malloc(context->tableData.cols * sizeof(char*));

        char* token = strtok(line, ",");
        int col = 0;
        RowData dp = {0};

        while (token && col < context->tableData.cols) {
            context->tableData.matrix[row][col] = strdup(token);

            if (col == COL_REGION) {
                strncpy(dp.region, token, MAX_LEN_REGION - 1);
            }

            token = strtok(NULL, ",");
            col++;
        }

        if (col == context->tableData.cols) {
            context->tableData.validRows[row] = dp;
            row++;
        }
         else {
            context->tableData.invalidRows++;
        }
    }

    context->tableData.rows = row;
    context->tableData.validRowsCount = row;
    return true;
}

int countMatchingRows(AppContext* context) {
    int count = 0;
    for (int i = 0; i < context->tableData.validRowsCount; i++) {
        if (strcmp(context->tableData.validRows[i].region,
                   context->filtredParams.regionFilter) == 0) {
                    count++;
        }
    }
    return count;
}

void copyAndFillRelevantRowsForMetrics(AppContext *context){
    int idx = 0;
    for (int i = 0; i < context->tableData.validRowsCount; i++) {
        RowData* src = &context->tableData.validRows[i];
        if (strcmp(src->region, context->filtredParams.regionFilter) == 0) {
            RowData* dest = &context->metrics.relevantRows[idx++];
            *dest = *src;
            switch (context->filtredParams.selectedColumn) {
            case (COL_YEAR + 1):
                dest->value = atoi(context->tableData.matrix[i][COL_YEAR]);
                break;
            case (COL_NPG + 1):
                dest->value = atof(context->tableData.matrix[i][COL_NPG]);
                break;
            case (COL_BIRTH_RATE + 1):
                dest->value = atof(context->tableData.matrix[i][COL_BIRTH_RATE]);
                break;
            case (COL_DEATH_RATE + 1):
                dest->value = atof(context->tableData.matrix[i][COL_DEATH_RATE]);
                break;
            case (COL_GDW + 1):
                dest->value = atof(context->tableData.matrix[i][COL_GDW]);
                break;
            case (COL_URBANIZATION + 1):
                dest->value = atof(context->tableData.matrix[i][COL_URBANIZATION]);
                break;
            }
        }
    }
}

