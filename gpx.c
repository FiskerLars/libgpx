#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <expat.h>
#include <errno.h>


#include "gpx.h"

// Using expat, include and lib missing
// http://www.xml.com/pub/a/1999/09/expat/reference.html


/* Specification  http://www.topografix.com/GPX/1/1/
 * Schema http://www.topografix.com/GPX/1/1/gpx.xsd
 */

/* <trk>
  <name>2010-04-30T13:46:52Z</name>
  <trkseg>
<trkpt lat="53.593132019" lon="13.109690666">
  <ele>317.765625</ele>
  <time>2010-04-30T13:46:52Z</time>
</trkpt>
*/


#define INIT_BUFSIZE 1024

// Tag Strings
#define TAG_GPX "gpx"
#define TAG_NAME "name" 
#define TAG_TRKPT "trkpt"
#define ATTR_LAT "lat"
#define ATTR_LON "lon"
#define TAG_ELE   "ele"
#define TAG_TIME  "time"


/** Internal enumeration of elements. */
enum gpx_id { gpx, trk, trksex, trkpt, ele, time };


// Rump-Handler forward declarations 
void start_hdl(void* usrData, const char* el, const char** attr);
void end_hdl(void* usrData, const char* el);
void char_hdl(void* usrData, const char* txt, int len);


struct input_handler {
      enum gpx_id id;
      char* tag;
      start_handler start;
      char_handler data;
      end_handler end;
};





/*********************************************************************/
// Stack of Trackpoints



struct xml_elem* xml_pop_elem(xml_stack* stack) {
      struct xml_elem* res = 0;
      if(stack)
	    if(stack->top){
		  res = stack->top;
		  if(res->prev != 0) { // top is not last elem
			res->prev->next = 0;
			stack->top = res->prev;
		  } else {
			stack->top = 0;
		  }
		  res->prev = res->next = 0;
		  stack->depth -= 1;
	    }
      return res;
}


/** init a new element. Just to know where in the tree we are. 
 */
struct xml_elem* xml_new_elem(const char* tag){
      struct xml_elem* elem =(struct xml_elem*)malloc(sizeof(struct xml_elem));
      elem->tag = strdup(tag);
      elem->prev = elem->next = 0;
}

void xml_del_elem(struct xml_elem* el) {
      free(el->tag);
      free(el);
}

xml_stack* xml_push_elem(xml_stack* stack,  struct xml_elem* elem) {
      if(stack) {
	    if(stack->top) { // stack not empty
		  stack->top->next = elem;
		  elem->prev = stack->top;
		  stack->top = elem;
	    } else { //stack empty
		  stack->first = elem;
		  elem->prev = 0;
		  stack->top = elem;
	    } 
	    stack->depth += 1;
      }
      return stack;
}


int xml_stack_depth(xml_stack* stack) {
      if(stack)
	    return stack->depth;
      return -1;
}





void dbg_print_gpx_input(struct gpx_input* input) {
      fprintf(stderr, " void* buf = 0x%08x\nvoid* buf_cur = 0x%08x\nvoid* buf_parsed = 0x%08x\nsize_t bufsize = %d\n",
	      (int)input->buf, (int)input->buf_cur, (int)input->buf_parsed, (int)input->bufsize);
      
      fprintf(stderr, "cur-parsed = %d\n",(input->buf_cur - input->buf_parsed));
      fprintf(stderr,"parser = 0x%08x\n", (int)input->parser);
}

/**********************************************************/


/** parsing a trkpt elements start.
Example: <trkpt lat="53.593132019" lon="13.109690666">
*/
int default_trkpt_start(void* usrData, const char* tag, const char** attr) {
      struct gpx_input* input = (struct gpx_input*)usrData;
      xml_push_elem(&(input->stack), xml_new_elem(TAG_TRKPT)); 
      // geopoint* v = new_point(input->ptset);  // TODO push that point
      int i; // counter
      if(attr) {
	    for(i = 0; attr[i]; i+=2) {
		  fprintf(stderr, "attr %s\n", attr[i]);
		  if(!strcmp(ATTR_LON, attr[i])) {
			fprintf(stderr, "parsing lon: %s\n", attr[i+1]);
			//parse_lon(v, attr[i+1]);
		  } else if(!strcmp(ATTR_LAT, attr[i])) {
			fprintf(stderr, "parsing lat: %s\n", attr[i+1]);
			//parse_lat(v,attr[i+1]);
		  }
	    }
	    return 0;
      }
      return 1;
}


// default handlers
int default_start(void* usrData, const char* tag, const char** attr) {
      xml_push_elem(&(((struct gpx_input*)usrData)->stack), xml_new_elem(tag));
      return 0;
} 

/** parsing character data. Handling determined by top element in parse-stack.
 */
int default_char(void* usrData, const char* txt, int len) {
      struct gpx_input* input = (struct gpx_input*)usrData;
      // FIXME ele time
      // TODO txt might be only part of element data, collect in last element of stack
      char* str = (char*)malloc((len+1)*sizeof(char));
      strncpy(str, txt, len);
      str[len] = '\0';
      xml_stack* stack = &(input->stack);
      if (stack->top) {
	    if(!strcmp(stack->top->tag, TAG_ELE)) {
		  fprintf(stderr, "parsing ele: %s\n", str);
		  //parse_ele(input->ptset->last->e, str);
	    } else if(!strcmp(stack->top->tag, TAG_ELE)) {
		  fprintf(stderr, "parsing time: %s\n", str);
		  //parse_time(input->ptset->last->e, str);		  
	    } else
		  fprintf(stderr, "Data in unknown element: ---%s---\n",str);
      }
      free(str);
}


/** Removes element from stack if tag matches the stack element. 
    Otherwise just hope that the stacked element will be found later in the stream.
 */
int default_end(void* usrData, const char* tag) {
      struct xml_elem* xelem = xml_pop_elem(&(((struct gpx_input*)usrData)->stack));
      if(xelem) 
	    if(!strcmp(xelem->tag, tag)) 
		  xml_del_elem(xelem);
	    else // element not matched on stack 
		  xml_push_elem(usrData,  xelem);
      // TODO: log error
      else // empty stack
	    return 1;
      return 0;
}


struct input_handler default_handler[3][5] = {
      {trkpt, TAG_TRKPT, default_trkpt_start, 0, default_end},
      {ele, TAG_ELE ,default_start , 0, default_end},
      {time, TAG_TIME, default_start, 0, default_end}
};



/**************************************************************************************************/



struct gpx_input* resize_in_buf(struct gpx_input* input, size_t len) {
      if(input)
	    if(input->buf != 0){
		  void* oldbuf = input->buf;
		  input->buf = (void*)realloc(input->buf, 
					      len);
		  input->bufsize = len;
		  input->buf_cur = input->buf + (input->buf_cur - oldbuf);
		  input->buf_parsed = input->buf + (input->buf_parsed - oldbuf);
	    } else {
		  // new buffer
		  input->buf =						\
			input->buf_cur =				\
			input->buf_parsed = (void*)malloc(len);
		  input->bufsize = len;
	    }
      return input;
}


struct gpx_input* incr_in_buf(struct gpx_input* input) {
      if(input)
	    if(input->buf){
		  void* oldbuf = input->buf;
		  input->bufsize += INIT_BUFSIZE;
		  input->buf = (void*)realloc(input->buf, 
					      input->bufsize);
		  input->buf_cur = input->buf + (input->buf_cur - oldbuf);
		  input->buf_parsed = input->buf + (input->buf_parsed - oldbuf);
	    } else {
		  // new buffer
		  input->buf =						\
			input->buf_cur =				\
			input->buf_parsed = (void*)malloc(INIT_BUFSIZE);
		  input->bufsize = INIT_BUFSIZE;
	    }
      return input;
} 


/** initiate a parsing structure...
 */
struct gpx_input* open_r_gpx_file(const char* const fname) {
      struct gpx_input *input = (struct gpx_input*)malloc(sizeof(struct gpx_input));
      input->stream = fopen(fname, "r");
      resize_in_buf(input, INIT_BUFSIZE);

      input->stack.first = 0;
      input->stack.top = 0;

      input->parser = XML_ParserCreate(0); //"UTF-8");
      XML_SetElementHandler(input->parser, start_hdl, end_hdl);
      XML_SetCharacterDataHandler(input->parser, char_hdl);
    
      return input;      
}

void gpx_free(struct gpx_input * input) {
      XML_ParserFree(input->parser);
      gs_free(input->ptset);
      free(input->buf);
      free(input);
}

void close_gpx_file(struct gpx_input* input) {
      // TODO check if input->stream is open
      if(input) 
	    fclose(input->stream);
}


/**********************************************************/
/*   parsing stuff                                        */



struct GSet* gs_new_set(){
      struct GSet* set = malloc(sizeof(struct GSet));
      set->pts = malloc(sizeof(struct GEOPoint*));
      set->pts[0] = 0;
      set->top = 0;
      set->n = 0;
      set->n_alloc = 0 ;
}


struct GSet* gs_push_new_elem(struct GSet* set,  struct GEOPoint* v) {
      if(set->n >= set->n_alloc) {
	    // alloc new chunk
	    int chunk = 20;
	    struct GEOPoint** tmp = realloc(set->pts, sizeof(GEOPoint*) * (set->n_alloc + 1 + chunk));
	    if(!tmp) {
		  perror("failed to allocate memory");
		  exit(1);
	    }
	    set->pts = tmp;
	    set->n_alloc = set->n_alloc + chunk;
      } 
      set->pts[set->n] = v;
      set->top = v;
      set->n++;
      set->pts[set->n] = 0;
}

GEOPoint* new_point(struct GSet* ptset){ 	    
      GEOPoint* v = (GEOPoint*)malloc(sizeof(GEOPoint));
      v->x = v->y = v->z = 0;
      // TODO gs_push_new_elem(ptset,  v);
      //      dbg_print_gset(ptset);
      return v;
}

void parse_lon(GEOPoint* v, const char* str) {
      v->x = atof(str);
}
void parse_lat(GEOPoint* v, const char* str) {
      v->y = atof(str);
}
void parse_ele(GEOPoint* v, const char* str) {
      v->z = atof(str);
}
void parse_time(GEOPoint* v, const char* str) {
}


/** FIXME: String operations.
for every new tag put onto the stack, be cautious to pop it at the end_hdl.
@param usrData is gpx_input
*/
void start_hdl(void* usrData, const char* el, const char** attr) {
      int i = 0;
      struct gpx_input* input = (struct gpx_input*)usrData;
      if(!input) {
	    errno = ENXIO;
	    perror("input gpx-stack not initialized!");
      }
	    
      fprintf(stderr, "Starttag %s\n", el);
      if(0 != input->stack.top) { // stack has element
	    // accept TAG_TIME, TAG_ELE
	    if(!strcmp(input->stack.top->tag, TAG_TRKPT))
		  if(!strcmp(el,TAG_ELE)) { // if it is elevetion element
			xml_push_elem(&(input->stack), xml_new_elem(TAG_ELE)); 
			fprintf(stderr," Start of elem %s\n", TAG_ELE);
		  } else if (!strcmp(el,TAG_TIME)) {// if it is time element
			xml_push_elem(&(input->stack), xml_new_elem(TAG_TIME)); 			
			fprintf(stderr," Start of elem %s\n", TAG_TIME);
		  }
      } else { // stack has no element yet
	    // accept TAG_TRKPT
	    if(0 == strcmp(el, TAG_TRKPT)){
		  fprintf(stderr, "startt_hdl: new trackpt\n");
		  xml_push_elem(&(input->stack),xml_new_elem(TAG_TRKPT)); 
		  GEOPoint* v = new_point(input->ptset);
		  if(attr) {
			for(i = 0; attr[i]; i+=2) {
			      fprintf(stderr, "attr %s\n", attr[i]);
			      if(!strcmp(ATTR_LON, attr[i])) {
				    fprintf(stderr, "parsing lon: %s\n", attr[i+1]);
				    parse_lon(v, attr[i+1]);
			      } else if(!strcmp(ATTR_LAT, attr[i])) {
				    fprintf(stderr, "parsing lat: %s\n", attr[i+1]);
				    parse_lat(v,attr[i+1]);
			      }
			}
			fprintf(stderr, "parsed %d attributes\n", i/2);
			dbg_print_point(v);
		  }
	    }	    
      }
      fprintf(stderr, "stack depth: %d\n",  xml_stack_depth(&(input->stack)));
}

int strsetcmp(const char* str, const char* set[]){
      int csel = 0; // character-selecto
      int ssel = 0; // string selector
      int nset;
      int* inset; //strings still in set;
      
      for(ssel = 0; set[ssel] != 0; ssel++);
      nset = ssel-1;
      inset = (int*)malloc((ssel-1) * sizeof(int));
      for(ssel = 0; set[ssel] !=0; ssel++) inset[ssel]=1;
      
      for(csel = 0; str[csel] != 0 && nset > 0; csel++){
	    for(ssel = 0; ssel != 0; ssel++){
		  if(inset[ssel])
			if (!(set[ssel][csel] != 0 
			      && set[ssel][csel] == str[csel])) {
			      inset[ssel] = 0;
			      nset--;
			} 
	    }
      } 
      if(nset > 0)
	    for(ssel=0; set[ssel]; ssel++)
		  if(inset[ssel])
			return ssel;
      return -1;
}

void end_hdl(void* usrData, const char* el) {
      const char* tags[] = {TAG_TRKPT, TAG_TIME, TAG_ELE};
      int match = strsetcmp(el, tags);
      if (match >=0) {
	    struct xml_elem* xelem = xml_pop_elem((xml_stack*)usrData);
	    if(xelem) {
		  if(!strcmp(xelem->tag, el))
			xml_del_elem(xelem);
	    }
      }
}

void char_hdl(void* usrData, const char* txt, int len) {
      // FIXME ele time
      // TODO txt might be only part of element data, collect in last element of stack
      xml_stack* stack = (xml_stack*)usrData;
      struct gpx_input* input = (struct gpx_input*)usrData;
      char* str = (char*)malloc((len+1)*sizeof(char));
      strncpy(str, txt, len);
      str[len] = '\0';

      if (stack->top) {
	    if(!strcmp(stack->top->tag, TAG_ELE)) {
		  parse_ele(input->ptset->top, str);
	    } else if(!strcmp(stack->top->tag, TAG_ELE)) {
		  parse_time(input->ptset->top, str);		  
	    } else
		  fprintf(stderr, "Data in unknown element: ---%s---\n",str);
      }
      free(str);
}



/********************************************************************/

/** read a number of bytes into buffer (at buf_cur), len 0 means "read until end" */
size_t read_input(struct gpx_input* input, size_t len) { 
      size_t bytes_read  = 0;
      if(len != 0) {
	    resize_in_buf(input, len + input->bufsize);
	    bytes_read = fread(input->buf_cur, 1, len, input->stream);
	    input->buf_cur += bytes_read;
      } else { // read INIT_BUFSIZE chunks until finished
	    size_t r = 0;
	    size_t toRead = input->bufsize - (input->buf_cur-input->buf);
	    
	    while(0 < (r = fread(input->buf_cur, 
					1, toRead,
					input->stream))) {
		  input->buf_cur += r;
		  bytes_read += r;
		  incr_in_buf(input);
		  toRead = input->bufsize - (input->buf_cur - input->buf);
	    }
      }    
      fprintf(stderr, "read %d bytes\n",bytes_read);
      return bytes_read;
}


/** just for documentation purposes.
 */
int parse(struct gpx_input* input){
      size_t bytes_read  = 0;
      TRKPT trkpt=0;
      /*      TRKPT trkpt = XML_Parse(input->parser, 
			      input->buf_parsed, 
			      input->buf_cur - input->buf_parsed,
			      );
      */
      fprintf(stderr, "reading all the document\n");
      read_input(input, 0);
      input->ptset = gs_new_set();

      //      write(2, input->buf_parsed, input->buf_cur - input->buf_parsed);
      //      dbg_print_gpx_input(input);
      //fprintf(stderr, "parsing (%d characters)...\n",input->buf_cur - input->buf_parsed);

      XML_SetUserData(input->parser, input);
      bytes_read = XML_Parse(input->parser, 
			     (char*)input->buf_parsed,
			     (int)(input->buf_cur - input->buf_parsed),
			     1);
      

      dbg_print_gset(input->ptset);
      
      return bytes_read;
}


