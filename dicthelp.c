
/* Includes
***************/   
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "gnrcheap.h"

/* constants
***************/

#define EXITCODE_SUCCESS    0
#define EXITCODE_FAIL_USAGE 1
#define EXITCODE_FAIL_FILE  2
#define EXITCODE_FAIL_MEM   3

#define MAX_DICTWORD_LEN 100

#define UNKNOWN_EDIT_DISTANCE (-1)

// Default settings
#define DEFAULT_DICT_FILE "/usr/share/dict/words"
#define DEFAULT_EDITDIST_THRESHOLD 2


/* macros
***********/
#define min(a,b)  (((a) <= (b)) ? (a) : (b))

#if DBG || DEBUG
 #define DBG_PRINTF printf
#else
 #define DBG_PRINTF
#endif


/* structures
***************/

typedef struct dictword_editdist {
    int    edit_dist;
    char  *dict_word;
} EDITDIST, *P_EDITDIST;
 
typedef struct {
    P_EDITDIST pwordarray;
    int        max_word;
    int        curr_size;   //Number of words currently held
} VECTOR_DICTWORD, *P_VECTOR_DICTWORD;    

typedef struct {
    BOOL   help;
    BOOL   verbose;
    int    editdist_threshold; 
    char  *dict_file;
    char   output_sort_order;
    BOOL   stop_on_match;
} PROGRAM_SETTINGS;


/* routines
****************/   
void usage()
{
    fprintf(stdout,"\n");
    fprintf(stdout,"Usage: dicthelp [OPTION]... [word]\n");
    fprintf(stdout,"Refers the dictionary and suggests correctly spelled "
                   "words(s) for a given (misspelled) word.\n");
    fprintf(stdout,"\n");
    fprintf(stdout,"If the word is not supplied on the command-line, it is "
                   "read through stdin\n");
    fprintf(stdout,"Options:\n");
    fprintf(stdout,"        -d <dictionary>\n");  
    fprintf(stdout,"           Specify name of the dictionary to look up the\n");
    fprintf(stdout,"           word/suggestions into\n");
    fprintf(stdout,"           *Default dictionary = %s\n",DEFAULT_DICT_FILE);
    fprintf(stdout,"        -s [r|a]\n");  
    fprintf(stdout,"           Set sort order of the output\n");
    fprintf(stdout,"           r  sorts suggested words according to the "
                               "relevancy (edit-distance)\n");
    fprintf(stdout,"           a  sorts suggested words alphabetically\n");
    fprintf(stdout,"           *Default sort order = r\n");
    fprintf(stdout,"        -f Force suggestions.\n");
    fprintf(stdout,"           Show suggestions (similarly spelled words) "
                               "even if the supplied word is\n");
    fprintf(stdout,"           spelled correctly.\n");
    fprintf(stdout,"           *By default -f is not in effect.\n");
    fprintf(stdout,"        -h Show this help\n");
    fprintf(stdout,"Advanced Options:\n");
    fprintf(stdout,"        -t n\n");
    fprintf(stdout,"           Set max edit-distance threshold\n");
    fprintf(stdout,"           n is edit-distance (a numeric value)\n");
    fprintf(stdout,"           Only suggestions with edit-distance less "
                               "than or equal to n will be shown\n");
    fprintf(stdout,"           Higher the value of n, more suggestions"
                               " (with reduced relevancy)\n");
    fprintf(stdout,"           *Default threshold value is %d\n",
                               DEFAULT_EDITDIST_THRESHOLD);
    fprintf(stdout," EXAMPLES:\n");
    fprintf(stdout," dicthelp happyness\n");
    fprintf(stdout," dicthelp -t 3 happyness\n");
    fprintf(stdout," dicthelp -t 3 -sa happyness\n");

}



void freewordvect(P_VECTOR_DICTWORD pv_word)
{
    int i=0;

    if(!pv_word) {
        return;
    }

    if(pv_word->pwordarray) {
        for(i=0; i < pv_word->curr_size; i++) {
            if(pv_word->pwordarray[i].dict_word) {
                free(pv_word->pwordarray[i].dict_word);
            }   
        }
        free(pv_word->pwordarray);
    }
}



int addwordtovect(P_VECTOR_DICTWORD pv_word,
                   char *word)
{
  void *realloc_ptr = NULL;

  if(pv_word->max_word == pv_word->curr_size) {
     realloc_ptr = realloc(pv_word->pwordarray,
                           pv_word->max_word*sizeof(EDITDIST) +
                           pv_word->max_word*2*sizeof(EDITDIST) + 
                           1*sizeof(EDITDIST));
     if(realloc_ptr) {
       pv_word->pwordarray = realloc_ptr;
       pv_word->max_word += pv_word->max_word*2 + 1;
     }
     else {
         fprintf(stderr,
                 "Memory allocation failed. Error: %s\n", 
                 strerror(errno));
         return (EXITCODE_FAIL_MEM);
     }
  }
  pv_word->pwordarray[pv_word->curr_size].edit_dist = UNKNOWN_EDIT_DISTANCE;
  pv_word->pwordarray[pv_word->curr_size].dict_word = malloc(strlen(word)+1);;
  if(!pv_word->pwordarray[pv_word->curr_size].dict_word) {
      fprintf(stderr,"Memory allocation failed. Error: %s\n", strerror(errno));
      return (EXITCODE_FAIL_MEM);
  }
  strcpy(pv_word->pwordarray[pv_word->curr_size].dict_word,word);
  pv_word->curr_size++;

  return (EXITCODE_SUCCESS);
}    



int calc_edit_dist(char *string1, char *string2)
{
    int i = 0;
    int j = 0;
    int strlen1 = string1 ? strlen(string1) : 0;
    int strlen2 = string2 ? strlen(string2) : 0;

    char *prev_row = NULL;
    char *curr_row = NULL;
    char *tmp = NULL; 

    int edit_dist = UNKNOWN_EDIT_DISTANCE;

   DBG_PRINTF("calc_edit_dist: Comparing %s with %s\n",string1, string2);
    
    prev_row = malloc(strlen2 + 1);
    if(!prev_row) {
        return UNKNOWN_EDIT_DISTANCE;
    }

    curr_row = malloc(strlen2 + 1);
    if(!curr_row) {
        free(prev_row);
        return UNKNOWN_EDIT_DISTANCE;
    }

    //Initialize the very first row
    for(j=0; j <= strlen2 ; j++)
        prev_row[j] = j;

   DBG_PRINTF(" \t  \t");
    for(j=0;j<strlen2;j++)
      DBG_PRINTF("%c\t",string2[j]); 
     
   DBG_PRINTF("\n");
   DBG_PRINTF(" \t");
    for(j=0;j<=strlen2;j++)
      DBG_PRINTF("%02d\t",prev_row[j]); 
  
    for(i = 0; i < strlen1; i ++) {
        curr_row[0] = i+1;
       DBG_PRINTF("\n");
       DBG_PRINTF("%c\t%02d\t",string1[i],curr_row[0]);
        for(j = 0; j < strlen2; j++) {
            if(string1[i] == string2[j]) {
                curr_row[j+1] = prev_row[j];
            }
            else {
                curr_row[j+1] = min(min(prev_row[j],prev_row[j+1]),curr_row[j]);
                curr_row[j+1]++;
            }
           DBG_PRINTF("%02d\t",curr_row[j+1]);
           }
        tmp = prev_row;
        prev_row = curr_row;
        curr_row = tmp;
    }

    edit_dist = prev_row[j];

    free(prev_row);
    free(curr_row);

   DBG_PRINTF("\n");
   DBG_PRINTF("\n");

    return edit_dist;
}



void strlwr_inplace(char *str)
{
  char *cp = str;

  while(*cp) {
      if(isupper(*cp))
      {
          *cp = tolower(*cp);
      }
      cp++;
  }
}



int cmp_heap_elements(PVOID pele1, PVOID pele2)
{
  P_EDITDIST pdist1 = (P_EDITDIST) pele1;
  P_EDITDIST pdist2 = (P_EDITDIST) pele2;

  return (pdist1->edit_dist - pdist2->edit_dist);
}

void get_programsettings(int argc, char **argv, PROGRAM_SETTINGS *psettings)
{
  int opt;

  while((opt = getopt(argc,argv,"?hvt:s:d:f")) != -1)
  {
      switch(opt) {
          case 'd':
              psettings->dict_file = optarg;
              break;
          case 't':
              psettings->editdist_threshold = atoi(optarg);
              break;
          case 's':
              if((strcmp(optarg,"r")==0) ||
                 (strcmp(optarg,"a")==0)) {
                  psettings->output_sort_order = optarg[0];
              }
              break;             
          case 'f':
              psettings->stop_on_match = FALSE;
              break;              
          case '?':
          case 'h':
              psettings->help = TRUE;
              break;              
      }
  }

}

/* main 
****************/
int main(int argc, char **argv)
{
  FILE *fp = NULL;
  char readbuff[MAX_DICTWORD_LEN+1];
  char userword[MAX_DICTWORD_LEN+1];
  int dictwordlen=0;
  signed int i=0;
  int exitcode = EXITCODE_SUCCESS; 
  BOOL match_found = FALSE;

  P_EDITDIST pworddist = NULL;
  PGNRCHEAP pheap = NULL;

  PROGRAM_SETTINGS settings = 
  {
      .help               = FALSE,
      .verbose            = FALSE,
      .editdist_threshold = DEFAULT_EDITDIST_THRESHOLD,
      .output_sort_order  = 'r',
      .dict_file          = DEFAULT_DICT_FILE,
      .stop_on_match      = TRUE
  };

  VECTOR_DICTWORD v_word = 
   {  
      .pwordarray = NULL, 
      .max_word   = 0, 
      .curr_size  = 0 
   };


  /* Field command-line arguments */
  get_programsettings(argc,argv,&settings);

  /* If 'help' is requested or incorrect arguments passed ... */
  if(settings.help) {
      usage();
      return (EXITCODE_SUCCESS);
  }

  /* Get hold of the word user is interested in */
  if(optind < argc) {
      //User has supplied the word on command-line
      strncpy(userword,argv[optind],sizeof(userword)-1);
      userword[sizeof(userword)-1] = '\0'; //Handle with grace, if 
                                           //user-supplied word is too long
  }
  else {
      //User has not supplied the word on command-line, read thru stdin
      fgets(userword,sizeof(userword),stdin);       
      i = strlen(userword);
      if(userword[i-1] == '\n') {
          userword[i-1] = '\0'; 
      }
 
  }


  /* Convert user word to lower case */
  strlwr_inplace(userword);


  /* Open the dictionary file */
  fp = fopen(settings.dict_file,"r");
  if(!fp) {
    fprintf(stderr, "Failure opening file %s. Error: %s\n",
                    settings.dict_file,
                    strerror(errno));
    return (EXITCODE_FAIL_FILE);
  } 


  /* Read all dictionary words */
  while(fgets(readbuff,sizeof(readbuff),fp)) {

      /* Remove the CR '\n' at the end of each word */
      dictwordlen = strlen(readbuff);
      if(readbuff[dictwordlen-1] == '\n') {
          readbuff[--dictwordlen] = '\0'; 
      }


      /* Ignore following 
         - 'names' (words starting with uppercase letter)
         - One letter long words 
      */
      if(isupper(readbuff[0]) ||
         strlen(readbuff) <= 1) {
              continue;
      }


      /* Add each word to the vector */
      exitcode = addwordtovect(&v_word, readbuff);
      if(exitcode != EXITCODE_SUCCESS)       
          break;
  }

  /* Close the dictionary file */
  fclose(fp);

  /* Check if any error occured in the while() loop*/
  if(exitcode != EXITCODE_SUCCESS) 
  {
      freewordvect(&v_word);
      return (exitcode);
  }

  DBG_PRINTF("Total words = %d\n",v_word.curr_size);
  DBG_PRINTF("Curr vector capacity = %d\n",v_word.max_word);

  /* Freeze the vector (free unused memory) */
  v_word.pwordarray = realloc(v_word.pwordarray,
          v_word.curr_size * sizeof(EDITDIST));

   /* Create the heap */
  pheap = gnrcheap_create(HEAP_TYPE_MIN,
                          v_word.curr_size,
                          cmp_heap_elements); 
  if(! pheap) {
     freewordvect(&v_word);
     return EXITCODE_FAIL_MEM;
  } 
 

  /* Calculate edit distance for each dictionary words v/s user word */
  for(i=0; i < v_word.curr_size; i++)
  {
      v_word.pwordarray[i].edit_dist = calc_edit_dist(
              userword, 
              v_word.pwordarray[i].dict_word);

      if(v_word.pwordarray[i].edit_dist == 0) {
        //Edit distance is ZERO, means an exact match was found in
        //the dictionary, means the user supplied word is spelled 
        //correctly          
        match_found = TRUE;  
        fprintf(stdout,"Your word '%s' was found in the dictionary, "
                       "which means it is spelled correctly.\n",
                       userword);
        if(settings.stop_on_match) {
            fprintf(stdout,"If you want to see similarly spelled words, "
                           "rerun this program with argument -f\n");
        }
        else {
            fprintf(stdout,"Below is the list of similarlty spelled words\n");
        }   
      }
      else {
        gnrcheap_insert(pheap,&v_word.pwordarray[i]); 
      }
  }

  if((match_found == FALSE) ||
     (match_found == TRUE && settings.stop_on_match == FALSE)) {     
      /* Show dictionary words and edit distances to the user word 
         in alphabetical order, if that is requested */
      if(settings.output_sort_order == 'a') {
          for(i=0; i < v_word.curr_size; i++) {
              if(v_word.pwordarray[i].edit_dist && 
                 v_word.pwordarray[i].edit_dist <= settings.editdist_threshold) {
                  fprintf(stdout,"%s\t=>\t%s\tedit-dist=%d\n", 
                          userword, 
                          v_word.pwordarray[i].dict_word,
                          v_word.pwordarray[i].edit_dist);
              }
          }
      }


      /* Show dictionary words and edit distances to the user word 
         in relevancy order (edit distance), if that is requested */
      if(settings.output_sort_order == 'r') {
          while(pworddist = gnrcheap_getmin(pheap))
          {
              if(pworddist->edit_dist <= settings.editdist_threshold) {
                  fprintf(stdout,"%s\t=>\t%s\tedit-dist=%d\n", 
                          userword, 
                          pworddist->dict_word,
                          pworddist->edit_dist);
              }
              gnrcheap_delmin(pheap,NULL);
          }
      }
  }

  /* Return the memory */
  gnrcheap_destroy(pheap,NULL); 
  freewordvect(&v_word);


  return (exitcode);
}    

