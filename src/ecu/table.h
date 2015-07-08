/*
 *  Name: table.h
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
 *  merge 2d and 3d tables
 */
#ifndef TABLE_H
#define TABLE_H

/************************************************************
** 2D Tables
************************************************************/ 
/*  This is the table struct.
   It keeps track of the table's x and y values,
   the data in the table, and how wide the table is.
   We need to know how wide the table is
   to support multidimensional tables. */
typedef struct table2D_t {
   float *xVals;
   float *yVals;
   float *data;
   int xAxiswidth;
} table2D_t;

/*  This is a prototype for our tableLookup function.
   It tells programs that #include "table.h" that they can
   use a function called tableLookup which returns a float
   and takes in a pointer to a table_t, an x value, and a y value. */
float table2DLookup(table2D_t *table, float x, float y);

/*    This is a function used to get table values */
float get2DData(table2D_t *table, int x, int y);

/*    This is a function used to set table values. */
void set2DData(table2D_t *table, int x, int y, float value);


/************************************************************
** 3D Tables
************************************************************/ 
/*  This is the table struct.
   It keeps track of the table's x, y, and z values,
   the data in the table, and how wide the table is.
   We need to know how wide the table is
   to support multidimensional tables. */
typedef struct table3D_t {
   float *xVals;
   float *yVals;
   float *zVals;
   float *data;
   int xAxisWidth;
   int yAxisLength;
} table3D_t;

/*  This is a prototype for our tableLookup function.
   It tells programs that #include "table.h" that they can
   use a function called tableLookup which returns a float
   and takes in a pointer to a table_t, an x value, a y value, and a z value. */
float table3DLookup(table3D_t *table, float x, float y, float z);

/*    This is a function used to get table values */
float get3DData(table3D_t *table, int x, int y, int z);

/*    This is a function used to set table values. */
void set3DData(table3D_t *table, int x, int y, int z, float value);


#endif
