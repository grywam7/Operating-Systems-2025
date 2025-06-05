#include <stdio.h>
#include <stdlib.h>

int collatz_conjecture(int input) {
    if (input % 2 == 0) {
        return input / 2;
    } else {
        return 3 * input + 1;
    }
}

int test_collatz_convergence(int input, int max_iter, int *steps) {
  if (max_iter <= 0 || steps == NULL) {
      return 0;
  }
  
  steps[0] = input;
  int count = 1;

  while (input != 1 && count < max_iter) {
      input = collatz_conjecture(input);
      steps[count] = input;
      count++;
  }

  if (input == 1) {
      return count; 
  } else {
      return 0;
  }
}
