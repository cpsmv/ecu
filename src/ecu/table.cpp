/*
 *  Name: table.cpp
 *  Author: Ivan Pachev & Alex Pink
 *
 *  Description:
 *  These are the table lookup functions (and associated helper functions) for the engine 
 *  fuel maps. 
 *
 *  Note to the programmer:
 *  Please use Arduino IDE to compile / upload sketches! To use the built-in
 *  Arduino libraries, the IDE must be used. The Makefile and code directory
 *  structure is too convoluted to use.
 *
 *  TODO LIST:
 *  Finish converting to 3D tables!
 *  merge 2d and 3d tables
 */
#include "table.h"
#include <Arduino.h>

/***********************************************************
** Helper Functions
***********************************************************/ 
/* This is a helper function used to calculate
   between which table axis values our desired input values fall. */
static int findIndex(const float *vals, float in) {
   int i;
   for (i = 0; in >= vals[i]; i++);
   return i - 1;
}

/***********************************************************
** 2D Tables
***********************************************************/ 
/*    This is a function used to get table values */
float get2DData(table2D_t *table, int x, int y) {
   return *(table->data + y * table->xAxiswidth + x);
}

/*    This is a function used to set table values. */
void set2DData(table2D_t *table, int x, int y, float value) {
   *(table->data + y * table->xAxiswidth + x) = value;
}

/*    This is the main function used to access table data. 
		Assume all x,y,z values have already been end-condition checked */
float table2DLookup(table2D_t *table, float x, float y) {
   //Find the indices for each axis between which our desired values fall.
   int xIndex = findIndex(table->xVals, x);
   int yIndex = findIndex(table->yVals, y);

   //Find the real values of each axis based on the calculated indices.
   float x_1 = table->xVals[xIndex];
   float y_1 = table->yVals[yIndex];
   float x_2 = table->xVals[xIndex + 1];
   float y_2 = table->yVals[yIndex + 1];

   //Return a bilinear interpolation of the data.
   return (
      1 / ((x_2 - x_1) * (y_2 - y_1)) * (
         get2DData(table, xIndex, yIndex)         * (x_2 - x) * (y_2 - y) +
         get2DData(table, xIndex + 1, yIndex)     * (x - x_1) * (y_2 - y) +
         get2DData(table, xIndex, yIndex + 1)     * (x_2 - x) * (y - y_1) +
         get2DData(table, xIndex + 1, yIndex + 1) * (x - x_1) * (y - y_1)
      )
   );
}

/***********************************************************
** 3D Tables
***********************************************************/ 
/*    This is a function used to get table values */
float get3DData(table3D_t *table, int x, int y, int z) {
   return *(table->data + z * (table->yAxisLength * table->xAxisWidth) + y * table->xAxisWidth + x);
}

/*    This is a function used to set table values. */
void set3DData(table3D_t *table, int x, int y, int z, float value) {
   *(table->data + z * (table->yAxisLength * table->xAxisWidth) + y * table->xAxisWidth + x) = value;
}

/*    This is the main function used to access table data. 
		Assume all x,y,z values have already been end-condition checked */
float table3DLookup(table3D_t *table, float x, float y, float z) {
   //Find the indices for each axis between which our desired values fall.
   int xIndex = findIndex(table->xVals, x);
   int yIndex = findIndex(table->yVals, y);
   int zIndex = findIndex(table->zVals, z);

   //Find the real values of each axis based on the calculated indices.
   float x_0 = table->xVals[xIndex];
   float y_0 = table->yVals[yIndex];
   float z_0 = table->zVals[zIndex];
   float x_1 = table->xVals[xIndex + 1];
   float y_1 = table->yVals[yIndex + 1];
   float z_1 = table->zVals[zIndex + 1];

   //Return a trilinear interpolation of the data.
   return (
      1 / ( (x_1 - x_0) * (y_1 - y_0) * (z_1 - z_0) ) 
        * (
           get3DData(table, xIndex, yIndex, zIndex)             * (x_1 - x) * (y_1 - y) * (z_1 - z) +
           get3DData(table, xIndex, yIndex, zIndex + 1)         * (x_1 - x) * (y_1 - y) * (z - z_0) +
           get3DData(table, xIndex, yIndex + 1, zIndex)         * (x_1 - x) * (y - y_0) * (z_1 - z) +
           get3DData(table, xIndex, yIndex + 1, zIndex + 1)     * (x_1 - x) * (y - y_0) * (z - z_0) +
           get3DData(table, xIndex + 1, yIndex, zIndex)         * (x - x_0) * (y_1 - y) * (z_1 - z) +
           get3DData(table, xIndex + 1, yIndex, zIndex + 1)     * (x - x_0) * (y_1 - y) * (z - z_0) +
           get3DData(table, xIndex + 1, yIndex + 1, zIndex)     * (x - x_0) * (y - y_0) * (z_1 - z) +
           get3DData(table, xIndex + 1, yIndex + 1, zIndex + 1) * (x - x_0) * (y - y_0) * (z - z_0)
      )
   );
}