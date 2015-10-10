
/*
// Name    : ngram.c
// Usage   : ngram < TEXT.FILE
// Depends : C, Unix, GNU/Linux
// Purpose : Snip. get ngram tokens from texdata
// Help    :
//
// ngram,1.0.1 extract ngram tokens from textdata
// ngram [-n [INT|INT-INT] [-u] [-c] [-s] [-S] [-i] [-l] [-d STRING]
//
//  -h                print this help
//  -n [INT|INT-INT]  build ngrams of length N or within range N-N
//  -u                uniq, discard all but one of successive identical ngrams
//  -c                count, prefix ngrams by the number of occurrences
//  -s                write sorted concatenation of all ngrams to standard output
//  -S                reverse the result of comparisons
//  -i                ignore words shorter as ngram length
//  -l                lower case input before processing. May decrease performance
//  -d [STRING]       use STRING as word delimiters
//
// Example: ngram -n 4 -u -c -S < TEXT.FILE
*/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#define PACKAGE   "ngram"
#define VERSION   "1.0.1"
#define NSORT     "/usr/bin/sort"
#define RSORT     "/usr/bin/sort -nr"
#define MAXLINE   1024

/* ngram container for binary tree */
struct tnode {
 char *ngrm;
 int count;
 struct tnode *left;
 struct tnode *right;
};

struct tnode *root = NULL;
int u_flag = 0;  /* uniq */
int c_flag = 0;  /* uniq + count */
int r_flag = 0;  /* range given */
int i_flag = 0;  /* ignore ngram, if wordlen < ngramlen */
int l_flag = 0;  /* lower case input before ... */

/* extract tokens from word */
void ngram(char *word, int N);
/* add ngram to binary tree */
struct tnode *addtree(struct tnode *, char *);
/* return memory for new node */
struct tnode *talloc(void);
/* print the binary tree */
void treeprint(struct tnode *, FILE *fp);
/* status epilepticus, print help */
void print_help(void);

int main(int argc, char *argv[]) {
 /* delim, for ``word'' tokens */
 char *delim = ".,:;`/\"+-_(){}[]<>*&^%$#@!?~/|\\=1234567890 \t\n";
 char *word = NULL;
 char *ptr  = NULL;
 char line[MAXLINE];
 FILE *fp; /* popen() for sort */

 int opt;
 int slice = 2; /* ngram token length */
 int nhigh = 0; /* range high */
 int nlow  = 0; /* range low */

 if(argc == 1) {
  print_help();
  return 1;
 }

 while((opt = getopt(argc, argv, "hn:ucsSild:")) != -1) {
  switch(opt) {
   case 'h':
    print_help();
    return 0;
   case 'n':
    if(strchr(optarg, '-') == NULL) {
     slice = atoi(optarg);
     if(slice < 1 || slice > 10) {
      fprintf(stderr, "%s: Error: Value out of range: 1 to 10\n", PACKAGE);
      print_help();
      return 1;
     }
    } else {
     r_flag = 1;
     sscanf(optarg, "%d-%d", &nlow, &nhigh);
     if((nhigh > 10 || nhigh < 0) || (nlow > 10 || nlow < 0) || (nhigh <= nlow)) {
      fprintf(stderr, "%s: Error: Invalid range given\n", PACKAGE);
      print_help();
      return 1;
     }
    }
    break;
   case 'u':
    u_flag = 1;
    break;
   case 'c':
    c_flag = 1;
    break;
   case 's':
    if((fp = popen(NSORT, "w")) == NULL) {
     fprintf(stderr, "%s: Error: Failed, popen %s\n", PACKAGE, NSORT);
     return 1;
    }
    break;
   case 'S':
    if((fp = popen(RSORT, "w")) == NULL) {
     fprintf(stderr, "%s: Error: Failed, popen %s\n", PACKAGE, RSORT);
     return 1;
    }
    break;
   case 'i':
    i_flag = 1;
    break;
   case 'l':
    l_flag = 1;
    break;
   case 'd':
    delim = optarg;
    break;
   case ':':
    fprintf(stderr, "%s: Error: Option needs a argument: %c ?\n", PACKAGE, optopt);
    print_help();
    break;
   case '?':
    fprintf(stderr, "%s: Error: Unkown option: %c\n", PACKAGE, optopt);
    print_help();
    break;
  }
 }

 while((fgets(line, MAXLINE, stdin)) != NULL) {
  if(strlen(line) > 1) {

   if(l_flag == 1)
    for(ptr = line; *ptr; ptr++)
     if(isupper(*ptr)) *ptr = tolower(*ptr);

   word = strtok(line, delim);
   while(word != NULL) {
   
    if(i_flag == 1 && strlen(word) < slice) {
     word = strtok(NULL, delim); 
     continue;
    }

    if(r_flag == 0)
     ngram(word, slice);
    else 
     for(slice = nlow; slice <= nhigh; slice++)
      ngram(word, slice);

    word = strtok(NULL, delim);
   } /* while */
  } /* if */
 } /* while */

 if(fp == NULL)
  treeprint(root, stdout);
 else
  treeprint(root, fp), pclose(fp);

 return 0;
}

void ngram(char *word, int N) {
 char *ptr1 = NULL;
 char *ptr2 = NULL;
 char *padword = NULL;
 char *buff = NULL;
 int i = 0;                 /* jump determiner */
 int b = N - 1;             /* back jump */

 if((buff = (char *)malloc(N + 1)) == NULL) {
  fprintf(stderr, "%s: Error: Failed malloc\n", PACKAGE);
  return;
 }

 if((padword = (char *)malloc(strlen(word) + N + 2)) == NULL) {
  fprintf(stderr, "%s: Error: Failed malloc\n", PACKAGE);
  return;
 }

 sprintf(padword, "_%s", word);
 for(i = 0; i < N - 1; i++) strcat(padword, "_");

 ptr1 = padword, ptr2 = buff, i = 0;
 while(*ptr1)
  if(i == N)
   *ptr2++ = '\0', root = addtree(root, buff), i = 0, ptr1 -= b, ptr2 = buff;
  else
   *ptr2++ = *ptr1++, i++;

 free(padword);
 free(buff);
}

struct tnode *addtree(struct tnode *p, char *w) {
 int cond;

 if(p == NULL) {
  p = talloc();
  p->ngrm = strdup(w);
  p->count = 1;
  p->left = p->right = NULL;
 } else if((cond = strcmp(w, p->ngrm)) == 0)
  p->count++;
 else if(cond < 0)
  p->left = addtree(p->left, w);
 else
  p->right = addtree(p->right, w);

 return p;
}

struct tnode *talloc(void) {
 return(struct tnode *)malloc(sizeof(struct tnode));
}

void treeprint(struct tnode *p, FILE *fp) {
 if(p != NULL) {
  treeprint(p->left, fp);

  if(c_flag == 1)
   fprintf(fp, "%4d %s\n", p->count, p->ngrm);
  else
   fprintf(fp, "%s\n", p->ngrm);

  treeprint(p->right, fp);
 }
}

void print_help(void) {
 printf("%s,%s extract ngram tokens from textdata\n", PACKAGE, VERSION); 
 printf("%s [-n [INT|INT-INT] [-u] [-c] [-s] [-S] [-i] [-l] [-d STRING]\n\n", PACKAGE);

 printf("  -h                print this help\n");
 printf("  -n [INT|INT-INT]  build ngrams of length N or within range N-N\n");
 printf("  -u                uniq, discard all but one of successive identical ngrams\n");
 printf("  -c                count, prefix ngrams by the number of occurrences\n");
 printf("  -s                write sorted concatenation of all ngrams to standard output\n");
 printf("  -S                reverse the result of comparisons\n");
 printf("  -i                ignore words shorter as ngram length\n");
 printf("  -l                lower case input before processing. May decrease performance\n");
 printf("  -d [STRING]       use STRING as word delimiters\n\n");

 printf(" Example: %s -n 4 -u -c -S < TEXT.FILE\n\n", PACKAGE);
}
