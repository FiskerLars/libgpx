#ifndef _GTO_GPX_H
#define _GTO_GPX_H

#include <expat.h> 
#include <stdio.h>

#define TRKPT void*



typedef struct GEOPoint {
      double x;
      double y;
      double z;
} GEOPoint;


struct GSet {
      struct GEOPoint** pts; //zero-terminated for easy iteration
      struct GEOPoint* top;
      unsigned int n;
      unsigned int n_alloc;
};


struct xml_elem {
      struct xml_elem *prev, *next;
      char *tag;
};

typedef struct xml_stack {
      struct xml_elem* first;
      struct xml_elem* top;
      int depth;
} xml_stack;


// int (*start_handler)(void* usrData, const char* elem, const char** attr)
typedef int (*start_handler)(void* , const char*, const char**);
// int (*start_handler)(void* usrData, const char* data, int len)
typedef int (*char_handler)(void* , const char*, int);
// int (*start_handler)(void* usrData, const char* elem)
typedef int (*end_handler)(void* , const char*);


struct gpx_input {
      FILE* stream;

      void* buf; // the buffer
      void* buf_cur;  // current write-to buffer-position
      void* buf_parsed; // buffer-position up to which is parsed
      size_t bufsize;
      
      XML_Parser parser;
      struct xml_stack stack;

      // TODO general (user defined) output structure
      struct GSet* ptset;
      void* usrData; // store everything needed by the user-defined handlers
};



struct gpx_input* open_r_gpx_file(const char* const fname);
int parse(struct gpx_input* input);


#endif
