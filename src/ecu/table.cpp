//table.c
//Written by Ivan Pachev
#include "table.h"

/* This is a helper function used to calculate
   between which table axis values our desired input values fall. */
static int findIndex(const float *vals, float in) {
   int i;
   for (i = 0; in >= vals[i]; i++);
   return i - 1;
}

/*    This is a function used to get table values */
float getData(table_t *table, int x, int y) {
   return *(table->data + y * table->width + x);
}

/*    This is a function used to set table values. */
void setData(table_t *table, int x, int y, float value) {
   *(table->data + y * table->width + x) = value;
}

/*    This is the main function used to access table data. */
float tableLookup(table_t *table, float x, float y) {
   float x_1, x_2, y_1, y_2;
   int xIndex, yIndex;

   if (x > table->xVals[table->width - 1]) {
      //Serial.print("TABLE LOOKUP OUT Of BOUNDS: ");
      //Serial.println(x);
   }
   //Find the indices for each axis between which our desired values fall.
   xIndex = findIndex(table->xVals, x);
   yIndex = findIndex(table->yVals, y);

   //Find the real values of each axis based on the calculated indices.
   x_1 = table->xVals[xIndex];
   y_1 = table->yVals[yIndex];
   x_2 = table->xVals[xIndex + 1];
   y_2 = table->yVals[yIndex + 1];

   //Return a bilinear interpolation of the data.
   return (
      1 / (x_2 - x_1) / (y_2 - y_1) * (
         getData(table, xIndex, yIndex) * (x_2 - x) * (y_2 - y) +
         getData(table, xIndex + 1, yIndex) * (x - x_1) * (y_2 - y) +
         getData(table, xIndex, yIndex + 1) * (x_2 - x) * (y - y_1) +
         getData(table, xIndex + 1, yIndex + 1) * (x - x_1) * (y - y_1)
      )
   );
}