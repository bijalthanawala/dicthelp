
/* Includes
***************/   
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

/* constants
***************/

#define EXITCODE_SUCCESS 0
#define EXITCODE_FAIL_USAGE 1
#define EXITCODE_FAIL_FILE  2
#define EXITCODE_FAIL_MEM   3

#define DICT_FILE "/usr/share/dict/american-english"
#define MAX_WORD_LENGTH 100

#define UNKNOWN_EDIT_DISTANCE (-1)


/* macros
***********/

#define min(a,b)  ((a) <= (b)) ? (a) : (b)

/* structures
***************/

typedef struct dictword_editdist {
    int edit_dist;
    char *dict_word;
} EDITDIST, *P_EDITDIST;
 
typedef struct {
    P_EDITDIST pwordarray;
    int max_word;
    int curr_size;   //Number of words currently held
} VECTOR_DICTWORD, *P_VECTOR_DICTWORD;    




/* routines
****************/   
void usage(int argc, char **argv)
{
    printf("usage: %s word\n",argv[0]);
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

    for(i = 0; i < strlen1; i ++) {
        curr_row[0] = i+1;
        for(j = 0; j < strlen2; j++) {
            if(curr_row[i] == prev_row[j]) {
                curr_row[j+1] = prev_row[j];
            }
            else {
                curr_row[j+1] = min(min(prev_row[j],prev_row[j+1]),curr_row[j]);
                curr_row[j+1]++;
            }
           }
        tmp = prev_row;
        prev_row = curr_row;
        curr_row = tmp;
    }

    edit_dist = prev_row[j];

    free(prev_row);
    free(curr_row);

    return edit_dist;
}

/* main 
****************/
int main(int argc, char **argv)
{
  FILE *fp = NULL;
  char readbuff[MAX_WORD_LENGTH+1];
  int dictwordlen=0;
  signed int i=0;
  int exitcode = EXITCODE_SUCCESS; 

  VECTOR_DICTWORD v_word = 
   {  
      .pwordarray = NULL, 
      .max_word   = 0, 
      .curr_size  = 0 
   };



  /* Arguments Check */
  if(argc < 2) {
       usage(argc, argv);
       return (EXITCODE_FAIL_USAGE);
  }  

  fp = fopen(DICT_FILE,"r");
  if(!fp) {
    fprintf(stderr, "Failure opening file %s. Error: %s\n",DICT_FILE,strerror(errno));
    return (EXITCODE_FAIL_FILE);
  } 

  while(fgets(readbuff,sizeof(readbuff),fp)) {
      dictwordlen = strlen(readbuff);
      if(readbuff[dictwordlen-1] == '\n') {
          readbuff[--dictwordlen] = '\0'; 
      }
      exitcode = addwordtovect(&v_word, readbuff);
      if(exitcode != EXITCODE_SUCCESS)       
        break;
  }

  if(exitcode != EXITCODE_SUCCESS) 
  {
      freewordvect(&v_word);
      return (exitcode);
  }

  printf("Total words = %d\n",v_word.curr_size);
  printf("Curr vector capacity = %d\n",v_word.max_word);

  v_word.pwordarray = realloc(v_word.pwordarray,
                              v_word.curr_size * sizeof(EDITDIST));

  for(i=0; i < v_word.curr_size; i++)
  {
      v_word.pwordarray[i].edit_dist = calc_edit_dist(
                                        argv[1], 
                                        v_word.pwordarray[i].dict_word);
  }

  for(i=v_word.curr_size-1; i >= 0; i--) {
      printf("%s\t%s\t%d\n", argv[1], 
                     v_word.pwordarray[i].dict_word,
                     v_word.pwordarray[i].edit_dist);
  }

  freewordvect(&v_word);
  return (exitcode);

}    
