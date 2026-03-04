#ifndef LOGIC_H
#define LOGIC_H

#include "appcontext.h"
#include <stdio.h>

void initializeContext(AppContext* context);
bool loadDataFromFile(AppContext* context);
bool parseHeader(AppContext* context, FILE* file);
bool parseRows(AppContext* context, FILE* file);
void filterData(AppContext* context);
void freeResources(AppContext* context);
void allocateMatrix(AppContext* context);
void copyAndFillRelevantRowsForMetrics(AppContext* context);
int countMatchingRows(AppContext* context);


#endif // LOGIC_H
