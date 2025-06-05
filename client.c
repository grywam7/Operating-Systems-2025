#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "collatz.h"

#define MAX_ITER 100

void run_collatz(int (*collatz_func)(int, int, int *), int input) {
    int steps[MAX_ITER];
    int count = collatz_func(input, MAX_ITER, steps);
    
    if (count > 0) {
        printf("Collatz sequence for %d: ", input);
        for (int i = 0; i < count; i++) {
            printf("%d ", steps[i]);
        }
        printf("\nSteps: %d\n", count);
    } else {
        printf("Collatz sequence for %d did not converge in %d iterations.\n",
               input, MAX_ITER);
    }
}

int main() {
    int inputs[] = {6, 11, 19, 27, 42};
    int num_inputs = sizeof(inputs) / sizeof(inputs[0]);

#ifdef LOAD_DYNAMICALLY
    // biblioteka współdzielona dynamicznie
    void *handle = dlopen("./libcollatz.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Error loading shared library: %s\n", dlerror());
        return 1;
    }
    
    int (*dyn_test_collatz_convergence)(int, int, int *) = dlsym(handle, "test_collatz_convergence");
    if (!dyn_test_collatz_convergence) {
        fprintf(stderr, "Error finding function: %s\n", dlerror());
        dlclose(handle);
        return 1;
    }
    
    printf("Using dynamically loaded library.\n");

    for (int i = 0; i < num_inputs; i++) {
        run_collatz(dyn_test_collatz_convergence, inputs[i]);
    }

    dlclose(handle);
#else
    // biblioteka statyczna / współdzielona
    printf("Using statically or shared linked library.\n");
    int (*static_test_collatz_convergence)(int, int, int *) = test_collatz_convergence;

    for (int i = 0; i < num_inputs; i++) {
        run_collatz(static_test_collatz_convergence, inputs[i]);
    }
#endif
    return 0;
}