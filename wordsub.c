const char *rcsid = "$Id: wordsub.c,v 1.1 2012/04/11 11:30:50 dyuret Exp $";
const char *usage = "Usage: wordsub [-s seed] < input > output\n"
  "Input format: word sub1 logp1 sub2 logp2 ...\n"
  "Output format: word random-sub\n";

// #define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "dlib.h"

int main(int argc, char **argv) {
  int opt;
  while ((opt = getopt(argc, argv, "s:h")) != -1) {
    switch (opt) {
    case 's': srand(atoi(optarg)); break;
    default: fputs(usage, stderr); exit(0);
    }
  }
  forline(buf, NULL) {
    double sum = 0;
    int col = 0;
    char *word = NULL;
    char *sub = NULL;
    char *last = NULL;
    fortok(tok, buf) {
      if (col == 0) {
	word = tok;
      } else if (col % 2) {
	last = tok;
      } else {
	double logp = atof(tok);
	double p = pow(10.0, logp);
	sum += p;
	if (sum * rand() / RAND_MAX <= p) {
	  sub = last;
	}
      }
      col++;
    }
    printf("%s\t%s\n", word, sub);
  }
}
