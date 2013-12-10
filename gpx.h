#ifndef _GTO_GPX_H
#define _GTO_GPX_H

#include <expat.h> 
#include <stdio.h>
#include <time.h>

#define TRKPT void*


/* 
<...
lat="latitudeType [1] ?"
lon="longitudeType [1] ?">
<ele> xsd:decimal </ele> [0..1] ?
<time> xsd:dateTime </time> [0..1] ?
<magvar> degreesType </magvar> [0..1] ?
<geoidheight> xsd:decimal </geoidheight> [0..1] ?
<name> xsd:string </name> [0..1] ?
<cmt> xsd:string </cmt> [0..1] ?
<desc> xsd:string </desc> [0..1] ?
<src> xsd:string </src> [0..1] ?
<link> linkType </link> [0..*] ?
<sym> xsd:string </sym> [0..1] ?
<type> xsd:string </type> [0..1] ?
<fix> fixType </fix> [0..1] ?
<sat> xsd:nonNegativeInteger </sat> [0..1] ?
<hdop> xsd:decimal </hdop> [0..1] ?
<vdop> xsd:decimal </vdop> [0..1] ?
<pdop> xsd:decimal </pdop> [0..1] ?
<ageofdgpsdata> xsd:decimal </ageofdgpsdata> [0..1] ?
<dgpsid> dgpsStationType </dgpsid> [0..1] ?
<extensions> extensionsType </extensions> [0..1] ?
</...>
*/

typedef GPXextension {
      wchar_t *key;
      wchar_t *value;
}

typedef struct GPXPoint {
      double lat;
      double lon;
      /** using pointer for optional elements */
      double *ele;
      time_t *time;
      double *magvar;
      double *geoidheight;
      wchar_t* name; 
      wchar_t* cmt; 
      wchar_t* desc; 
      wchar_t* src; 
      wchar_t* link; 
      wchar_t* sym; 
      wchar_t* type; 
      // fix
      uint *sat;
      double *hdop;
      double *vdop;
      double *pdop;
      double *ageofgpsdata;
      //dgpsid;
      struct GPXextension **extensions; // zero terminated list
} GPXPoint;


struct GSet {
      struct GPXPoint** pts; //zero-terminated for easy iteration
      struct GPXPoint* top;
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
