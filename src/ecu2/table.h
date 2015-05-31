//table.h
//Written by Ivan Pachev
#ifndef TABLE_H
#define TABLE_H

/*  This is the table struct.
   It keeps track of the table's x and y values,
   the data in the table, and how wide the table is.
   We need to know how wide the table is
   to support multidimensional tables. */
typedef struct table_t {
   float *xVals;
   float *yVals;
   float *data;
   int width;
   float defaultVal;
} table_t;

/*  This is a prototype for our tableLookup function.
   It tells programs that #include "table.h" that they can
   use a function called tableLookup which returns a float
   and takes in a pointer to a table_t, an x value, and a y value. */
float tableLookup(table_t *table, float x, float y);

/*    This is a function used to get table values */
float getData(table_t *table, int x, int y);

/*    This is a function used to set table values. */
void setData(table_t *table, int x, int y, float value);


#endif
