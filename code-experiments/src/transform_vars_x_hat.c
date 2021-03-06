#include <assert.h>

#include "coco.h"
#include "coco_problem.c"
#include "suite_bbob_legacy_code.c"

typedef struct {
  long seed;
  double *x;
  coco_free_function_t old_free_problem;
} transform_vars_x_hat_data_t;

static void transform_vars_x_hat_evaluate(coco_problem_t *self, const double *x, double *y) {
  size_t i;
  transform_vars_x_hat_data_t *data;
  coco_problem_t *inner_problem;
  data = coco_transformed_get_data(self);
  inner_problem = coco_transformed_get_inner_problem(self);
  do {
    bbob2009_unif(data->x, self->number_of_variables, data->seed);

    for (i = 0; i < self->number_of_variables; ++i) {
      if (data->x[i] - 0.5 < 0.0) {
        data->x[i] = -x[i];
      } else {
        data->x[i] = x[i];
      }
    }
    coco_evaluate_function(inner_problem, data->x, y);
    assert(y[0] + 1e-13 >= self->best_value[0]);
  } while (0);
}

static void transform_vars_x_hat_free(void *thing) {
  transform_vars_x_hat_data_t *data = thing;
  coco_free_memory(data->x);
}

/**
 * Multiply the x-vector by the vector 1+-.
 */
static coco_problem_t *f_transform_vars_x_hat(coco_problem_t *inner_problem, long seed) {
  transform_vars_x_hat_data_t *data;
  coco_problem_t *self;
  size_t i;

  data = coco_allocate_memory(sizeof(*data));
  data->seed = seed;
  data->x = coco_allocate_vector(inner_problem->number_of_variables);

  self = coco_transformed_allocate(inner_problem, data, transform_vars_x_hat_free);
  self->evaluate_function = transform_vars_x_hat_evaluate;
  /* Dirty way of setting the best parameter of the transformed f_schwefel... */
  bbob2009_unif(data->x, self->number_of_variables, data->seed);
  for (i = 0; i < self->number_of_variables; ++i) {
      if (data->x[i] - 0.5 < 0.0) {
          self->best_parameter[i] = -0.5 * 4.2096874633;
      } else {
          self->best_parameter[i] = 0.5 * 4.2096874633;
      }
  }
  return self;
}
