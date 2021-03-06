#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "coco.h"
#include "coco.c"

#include "mex.h"

/* The gateway function */
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    size_t *ref;
    coco_problem_t *pb = NULL;
    const char *res;

    /* check for proper number of arguments */
    if(nrhs!=1) {
        mexErrMsgIdAndTxt("cocoProblemGetName:nrhs","One input required.");
    }
    /* get the problem */
    ref = (size_t *)mxGetData(prhs[0]);
    pb = (coco_problem_t *)(*ref);
    /* call coco_problem_get_name(...) */
    res = coco_problem_get_name(pb);
    /* prepare the return value */
    plhs[0] = mxCreateString(res);
}


