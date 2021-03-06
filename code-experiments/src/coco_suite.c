#include <time.h>

#include "coco.h"
#include "coco_internal.h"

#include "suite_bbob.c"
#include "suite_biobj.c"
#include "suite_toy.c"
#include "suite_largescale.c"

/**
 * TODO: Add instructions on how to implement a new suite!
 * TODO: Add asserts regarding input values!
 */

static coco_suite_t *coco_suite_allocate(const char *suite_name,
                                         const size_t number_of_functions,
                                         const size_t number_of_dimensions,
                                         const size_t *dimensions,
                                         const char *default_instances) {

  coco_suite_t *suite;
  size_t i;

  suite = (coco_suite_t *) coco_allocate_memory(sizeof(*suite));

  suite->suite_name = coco_strdup(suite_name);

  suite->number_of_dimensions = number_of_dimensions;
  suite->dimensions = coco_allocate_memory(suite->number_of_dimensions * sizeof(size_t));
  for (i = 0; i < suite->number_of_dimensions; i++) {
    suite->dimensions[i] = dimensions[i];
  }

  suite->number_of_functions = number_of_functions;
  suite->functions = coco_allocate_memory(suite->number_of_functions * sizeof(size_t));
  for (i = 0; i < suite->number_of_functions; i++) {
    suite->functions[i] = i + 1;
  }

  suite->default_instances = coco_strdup(default_instances);

  /* Will be set to the first valid dimension index before the constructor ends */
  suite->current_dimension_idx = -1;
  /* Will be set to the first valid function index before the constructor ends  */
  suite->current_function_idx = -1;

  suite->current_instance_idx = -1;
  suite->current_problem = NULL;

  /* To be set in coco_suite_set_instance() */
  suite->number_of_instances = 0;
  suite->instances = NULL;

  /* To be set in particular suites if needed */
  suite->data = NULL;
  suite->data_free_function = NULL;

  return suite;
}

static void coco_suite_set_instance(coco_suite_t *suite,
                                    const size_t *instance_numbers) {

  size_t i;

  if (!instance_numbers) {
    coco_error("coco_suite_set_instance(): no instance given");
    return;
  }

  suite->number_of_instances = coco_numbers_count(instance_numbers, "suite instance numbers");
  suite->instances = coco_allocate_memory(suite->number_of_instances * sizeof(size_t));
  for (i = 0; i < suite->number_of_instances; i++) {
    suite->instances[i] = instance_numbers[i];
  }

}

static void coco_suite_filter_ids(size_t *items, const size_t number_of_items, const size_t *indices, const char *name) {

  size_t i, j;
  size_t count = coco_numbers_count(indices, name);
  int found;

  for (i = 1; i <= number_of_items; i++) {
    found = 0;
    for (j = 0; j < count; j++) {
      if (i == indices[j]) {
        found = 1;
        break;
      }
    }
    if (!found)
      items[i - 1] = 0;
  }

}

static void coco_suite_filter_dimensions(coco_suite_t *suite, const size_t *dims) {

  size_t i, j;
  size_t count = coco_numbers_count(dims, "dimensions");
  int found;

  for (i = 0; i < suite->number_of_dimensions; i++) {
    found = 0;
    for (j = 0; j < count; j++) {
      if (suite->dimensions[i] == dims[j])
        found = 1;
    }
    if (!found)
      suite->dimensions[i] = 0;
  }

}

/**
 * @param suite The given suite.
 * @param function_idx The index of the function in question (starting from 0).
 * @return The function number in position function_idx in the suite. If the function has been filtered out
 * through suite_options in the coco_suite function, the result is 0.
 */
size_t coco_suite_get_function_from_function_index(coco_suite_t *suite, size_t function_idx) {

  if (function_idx >= suite->number_of_functions) {
    coco_error("coco_suite_get_function_from_index(): function index exceeding the number of functions in the suite");
    return 0; /* Never reached*/
  }

 return suite->functions[function_idx];
}

/**
 * @param suite The given suite.
 * @param dimension_idx The index of the dimension in question (starting from 0).
 * @return The dimension number in position dimension_idx in the suite. If the dimension has been filtered out
 * through suite_options in the coco_suite function, the result is 0.
 */
size_t coco_suite_get_dimension_from_dimension_index(coco_suite_t *suite, size_t dimension_idx) {

  if (dimension_idx >= suite->number_of_dimensions) {
    coco_error("coco_suite_get_dimension_from_index(): dimensions index exceeding the number of dimensions in the suite");
    return 0; /* Never reached*/
  }

 return suite->dimensions[dimension_idx];
}

/**
 * @param suite The given suite.
 * @param instance_idx The index of the instance in question (starting from 0).
 * @return The instance number in position instance_idx in the suite. If the instance has been filtered out
 * through suite_options in the coco_suite function, the result is 0.
 */
size_t coco_suite_get_instance_from_instance_index(coco_suite_t *suite, size_t instance_idx) {

  if (instance_idx >= suite->number_of_instances) {
    coco_error("coco_suite_get_instance_from_index(): instance index exceeding the number of instances in the suite");
    return 0; /* Never reached*/
  }

 return suite->functions[instance_idx];
}

void coco_suite_free(coco_suite_t *suite) {

  if (suite != NULL) {

    if (suite->suite_name) {
      coco_free_memory(suite->suite_name);
      suite->suite_name = NULL;
    }
    if (suite->dimensions) {
      coco_free_memory(suite->dimensions);
      suite->dimensions = NULL;
    }
    if (suite->functions) {
      coco_free_memory(suite->functions);
      suite->functions = NULL;
    }
    if (suite->instances) {
      coco_free_memory(suite->instances);
      suite->instances = NULL;
    }
    if (suite->default_instances) {
      coco_free_memory(suite->default_instances);
      suite->default_instances = NULL;
    }

    if (suite->current_problem) {
      coco_problem_free(suite->current_problem);
      suite->current_problem = NULL;
    }

    if (suite->data != NULL) {
      if (suite->data_free_function != NULL) {
        suite->data_free_function(suite->data);
      }
      coco_free_memory(suite->data);
      suite->data = NULL;
    }

    coco_free_memory(suite);
    suite = NULL;
  }
}

static coco_suite_t *coco_suite_intialize(const char *suite_name) {

  coco_suite_t *suite;

  if (strcmp(suite_name, "toy") == 0) {
    suite = suite_toy_allocate();
  } else if (strcmp(suite_name, "bbob") == 0) {
    suite = suite_bbob_allocate();
  } else if (strcmp(suite_name, "bbob-biobj") == 0) {
    suite = suite_biobj_allocate();
  } else if (strcmp(suite_name, "bbob-largescale") == 0) {
    suite = suite_largescale_allocate();
  }
  else {
    coco_error("coco_suite(): unknown problem suite");
    return NULL;
  }

  return suite;
}

static char *coco_suite_get_instances_by_year(coco_suite_t *suite, const int year) {

  char *year_string;

  if (strcmp(suite->suite_name, "bbob") == 0) {
    year_string = suite_bbob_get_instances_by_year(year);
  } else if (strcmp(suite->suite_name, "bbob-biobj") == 0) {
    year_string = suite_biobj_get_instances_by_year(year);
  } else {
    coco_error("coco_suite_get_instances_by_year(): suite '%s' has no years defined", suite->suite_name);
    return NULL;
  }

  return year_string;
}

static coco_problem_t *coco_suite_get_problem_from_indices(coco_suite_t *suite,
                                                           size_t function_idx,
                                                           size_t dimension_idx,
                                                           size_t instance_idx) {

  coco_problem_t *problem;

  if (strcmp(suite->suite_name, "toy") == 0) {
    problem = suite_toy_get_problem(suite, function_idx, dimension_idx, instance_idx);
  } else if (strcmp(suite->suite_name, "bbob") == 0) {
    problem = suite_bbob_get_problem(suite, function_idx, dimension_idx, instance_idx);
  } else if (strcmp(suite->suite_name, "bbob-biobj") == 0) {
    problem = suite_biobj_get_problem(suite, function_idx, dimension_idx, instance_idx);
  } else if (strcmp(suite->suite_name, "bbob-largescale") == 0) {
    problem = suite_largescale_get_problem(suite, function_idx, dimension_idx, instance_idx);
  } else {
    coco_error("coco_suite_get_problem(): unknown problem suite");
    return NULL;
  }

  return problem;
}

/**
 * Note that the problem_index depends on the number of instances a suite is defined with.
 *
 * @param suite The given suite.
 * @param problem_index The index of the problem to be returned.
 * @return The problem of the suite defined by problem_index.
 */
coco_problem_t *coco_suite_get_problem(coco_suite_t *suite, size_t problem_index) {

  size_t function_idx = 0, instance_idx = 0, dimension_idx = 0;
  coco_suite_decode_problem_index(suite, problem_index, &function_idx, &dimension_idx, &instance_idx);

  return coco_suite_get_problem_from_indices(suite, function_idx, dimension_idx, instance_idx);
}

/**
 * The number of problems in the suite is computed as a product of the number of instances, number of
 * functions and number of dimensions and therefore doesn't account for any filtering done through the
 * suite_options parameter of the coco_suite function.
 *
 * @param suite The given suite.
 * @return The number of problems in the suite.
 */
size_t coco_suite_get_number_of_problems(coco_suite_t *suite) {
  return (suite->number_of_instances * suite->number_of_functions * suite->number_of_dimensions);
}

static size_t *coco_suite_get_instance_indices(coco_suite_t *suite, const char *suite_instance) {

  int year = -1;
  char *instances = NULL;
  char *year_string = NULL;
  long year_found, instances_found;
  int parce_year = 1, parce_instances = 1;
  size_t *result = NULL;

  if (suite_instance == NULL)
    return NULL;

  year_found = coco_strfind(suite_instance, "year");
  instances_found = coco_strfind(suite_instance, "instances");

  if ((year_found < 0) && (instances_found < 0))
    return NULL;

  if ((year_found > 0) && (instances_found > 0)) {
    if (year_found < instances_found) {
      parce_instances = 0;
      coco_warning("coco_suite_parse_instance_string(): 'instances' suite option ignored because it follows 'year'");
    }
    else {
      parce_year = 0;
      coco_warning("coco_suite_parse_instance_string(): 'year' suite option ignored because it follows 'instances'");
    }
  }

  if ((year_found >= 0) && (parce_year == 1)) {
    if (coco_options_read_int(suite_instance, "year", &(year)) != 0) {
      year_string = coco_suite_get_instances_by_year(suite, year);
      result = coco_string_get_numbers_from_ranges(year_string, "instances", 1, 0);
    } else {
      coco_warning("coco_suite_parse_instance_string(): problems parsing the 'year' suite_instance option, ignored");
    }
  }

  instances = coco_allocate_memory(COCO_MAX_INSTANCES * sizeof(char));
  if ((instances_found >= 0) && (parce_instances == 1)) {
    if (coco_options_read_values(suite_instance, "instances", instances) > 0) {
      result = coco_string_get_numbers_from_ranges(instances, "instances", 1, 0);
    } else {
      coco_warning("coco_suite_parse_instance_string(): problems parsing the 'instance' suite_instance option, ignored");
    }
  }
  coco_free_memory(instances);

  return result;
}

/**
 * Iterates through the items from the current_item_id position on in search for the next positive item.
 * If such an item is found, current_item_id points to this item and the method returns 1. If such an
 * item cannot be found, current_item_id points to the first positive item and the method returns 0.
 */
static int coco_suite_is_next_item_found(const size_t number_of_items, const size_t *items, long *current_item_id) {

  if ((*current_item_id) != number_of_items - 1)  {
    /* Not the last item, iterate through items */
    do {
      (*current_item_id)++;
    } while (((*current_item_id) < number_of_items - 1) && (items[*current_item_id] == 0));

    assert((*current_item_id) < number_of_items);
    if (items[*current_item_id] != 0) {
      /* Next item is found, return true */
      return 1;
    }
  }

  /* Next item cannot be found, move to the first good item and return false */
  *current_item_id = -1;
  do {
    (*current_item_id)++;
  } while ((*current_item_id < number_of_items - 1) && (items[*current_item_id] == 0));
  if (items[*current_item_id] == 0)
    coco_error("coco_suite_is_next_item_found(): the chosen suite has no valid (positive) items");
  return 0;
}

/**
 * Iterates through the instances of the given suite from the current_instance_idx position on in search for
 * the next positive instance. If such an instance is found, current_instance_idx points to this instance and
 * the method returns 1. If such an instance cannot be found, current_instance_idx points to the first
 * positive instance and the method returns 0.
 */
static int coco_suite_is_next_instance_found(coco_suite_t *suite) {

  return coco_suite_is_next_item_found(suite->number_of_instances, suite->instances,
      &suite->current_instance_idx);
}

/**
 * Iterates through the functions of the given suite from the current_function_idx position on in search for
 * the next positive function. If such a function is found, current_function_idx points to this function and
 * the method returns 1. If such a function cannot be found, current_function_idx points to the first
 * positive function, current_instance_idx points to the first positive instance and the method returns 0.
 */
static int coco_suite_is_next_function_found(coco_suite_t *suite) {

  int result = coco_suite_is_next_item_found(suite->number_of_functions, suite->functions,
      &suite->current_function_idx);
  if (!result) {
    /* Reset the instances */
    suite->current_instance_idx = -1;
    coco_suite_is_next_instance_found(suite);
  }
  return result;
}

/**
 * Iterates through the dimensions of the given suite from the current_dimension_idx position on in search for
 * the next positive dimension. If such a dimension is found, current_dimension_idx points to this dimension
 * and the method returns 1. If such a dimension cannot be found, current_dimension_idx points to the first
 * positive dimension and the method returns 0.
 */
static int coco_suite_is_next_dimension_found(coco_suite_t *suite) {

  return coco_suite_is_next_item_found(suite->number_of_dimensions, suite->dimensions,
      &suite->current_dimension_idx);
}

/**
 * Currently, four suites are supported:
 * - "bbob" contains 24 <a href="http://coco.lri.fr/downloads/download15.03/bbobdocfunctions.pdf">
 * single-objective functions</a> in 6 dimensions (2, 3, 5, 10, 20, 40)
 * - "bbob-biobj" contains 55 <a href="http://numbbo.github.io/bbob-biobj-functions-doc">bi-objective
 * functions</a> in 6 dimensions (2, 3, 5, 10, 20, 40)
 * - "bbob-largescale" contains 24 <a href="http://coco.lri.fr/downloads/download15.03/bbobdocfunctions.pdf">
 * single-objective functions</a> in 6 large dimensions (40, 80, 160, 320, 640, 1280)
 * - "toy" contains 6 <a href="http://coco.lri.fr/downloads/download15.03/bbobdocfunctions.pdf">
 * single-objective functions</a> in 5 dimensions (2, 3, 5, 10, 20)
 *
 * Only the suite_name parameter needs to be non-empty. The suite_instance and suite_options can be "" or
 * NULL. In this case, default values are taken (default instances of a suite are those used in the last year
 * and the suite is not filtered by default).
 *
 * @param suite_name A string containing the name of the suite. Currently supported suite names are "bbob",
 * "bbob-biobj", "bbob-largescale" and "toy".
 * @param suite_instance A string used for defining the suite instances. Two ways are supported:
 * - "year: YEAR", where YEAR is the year of the BBOB workshop, includes the instances (to be) used in that
 * year's workshop;
 * - "instances: VALUES", where VALUES are instance numbers from 1 on written as a comma-separated list or a
 * range m-n.
 * @param suite_options A string of pairs "key: value" used to filter the suite (especially useful for
 * parallelizing the experiments). Supported options:
 * - "dimensions: LIST", where LIST is the list of dimensions to keep in the suite (range-style syntax is
 * not allowed here),
 * - "function_idx: VALUES", where VALUES is a list or a range of function indexes (starting from 1) to keep
 * in the suite, and
 * - "instance_idx: VALUES", where VALUES is a list or a range of instance indexes (starting from 1) to keep
 * in the suite.
 * @return The constructed suite object.
 */
coco_suite_t *coco_suite(const char *suite_name, const char *suite_instance, const char *suite_options) {

  coco_suite_t *suite;
  size_t *instances;
  char *option_string = NULL;
  char *ptr;
  size_t *indices = NULL;
  size_t *dimensions = NULL;
  long dim_found, dim_idx_found;
  int parce_dim = 1, parce_dim_idx = 1;

  /* Initialize the suite */
  suite = coco_suite_intialize(suite_name);

  /* Set the instance */
  if ((!suite_instance) || (strlen(suite_instance) == 0))
    instances = coco_suite_get_instance_indices(suite, suite->default_instances);
  else
    instances = coco_suite_get_instance_indices(suite, suite_instance);
  coco_suite_set_instance(suite, instances);
  coco_free_memory(instances);

  /* Apply filter if any given by the suite_options */
  if ((suite_options) && (strlen(suite_options) > 0)) {
    option_string = coco_allocate_memory(COCO_PATH_MAX * sizeof(char));
    if (coco_options_read_values(suite_options, "function_idx", option_string) > 0) {
      indices = coco_string_get_numbers_from_ranges(option_string, "function_idx", 1, suite->number_of_functions);
      if (indices != NULL) {
        coco_suite_filter_ids(suite->functions, suite->number_of_functions, indices, "function_idx");
        coco_free_memory(indices);
      }
    }
    coco_free_memory(option_string);

    option_string = coco_allocate_memory(COCO_PATH_MAX * sizeof(char));
    if (coco_options_read_values(suite_options, "instance_idx", option_string) > 0) {
      indices = coco_string_get_numbers_from_ranges(option_string, "instance_idx", 1, suite->number_of_instances);
      if (indices != NULL) {
        coco_suite_filter_ids(suite->instances, suite->number_of_instances, indices, "instance_idx");
        coco_free_memory(indices);
      }
    }
    coco_free_memory(option_string);

    dim_found = coco_strfind(suite_options, "dimensions");
    dim_idx_found = coco_strfind(suite_options, "dimension_idx");

    if ((dim_found > 0) && (dim_idx_found > 0)) {
      if (dim_found < dim_idx_found) {
        parce_dim_idx = 0;
        coco_warning("coco_suite(): 'dimension_idx' suite option ignored because it follows 'dimensions'");
      }
      else {
        parce_dim = 0;
        coco_warning("coco_suite(): 'dimensions' suite option ignored because it follows 'dimension_idx'");
      }
    }

    option_string = coco_allocate_memory(COCO_PATH_MAX * sizeof(char));
    if ((dim_idx_found >= 0) && (parce_dim_idx == 1) && (coco_options_read_values(suite_options, "dimension_idx", option_string) > 0)) {
      indices = coco_string_get_numbers_from_ranges(option_string, "dimension_idx", 1, suite->number_of_dimensions);
      if (indices != NULL) {
        coco_suite_filter_ids(suite->dimensions, suite->number_of_dimensions, indices, "dimension_idx");
        coco_free_memory(indices);
      }
    }
    coco_free_memory(option_string);

    option_string = coco_allocate_memory(COCO_PATH_MAX * sizeof(char));
    if ((dim_found >= 0) && (parce_dim == 1) && (coco_options_read_values(suite_options, "dimensions", option_string) > 0)) {
      ptr = option_string;
      /* Check for disallowed characters */
      while (*ptr != '\0') {
        if ((*ptr != ',') && !isdigit((unsigned char )*ptr)) {
          coco_warning("coco_suite(): 'dimensions' suite option ignored because of disallowed characters");
          return NULL;
        } else
          ptr++;
      }
      dimensions = coco_string_get_numbers_from_ranges(option_string, "dimensions", suite->dimensions[0],
          suite->dimensions[suite->number_of_dimensions - 1]);
      if (dimensions != NULL) {
        coco_suite_filter_dimensions(suite, dimensions);
        coco_free_memory(dimensions);
      }
    }
    coco_free_memory(option_string);
  }

  /* Check that there are enough dimensions, functions and instances left */
  if ((suite->number_of_dimensions < 1)
      || (suite->number_of_functions < 1)
      || (suite->number_of_instances < 1)) {
    coco_error("coco_suite(): the suite does not contain at least one dimension, function and instance");
    return NULL;
  }

  /* Set the starting values of the current indices in such a way, that when the instance_idx is incremented,
   * this results in a valid problem */
  coco_suite_is_next_function_found(suite);
  coco_suite_is_next_dimension_found(suite);

  return suite;
}

/**
 * Iterates through the suite first by instances, then by functions and finally by dimensions.
 * The instances/functions/dimensions that have been filtered out using the suite_options of the coco_suite
 * function are skipped. Outputs some information regarding the current place in the iteration. The returned
 * problem is wrapped with the observer. If the observer is NULL, the returned problem is unobserved.
 *
 * @param suite The given suite.
 * @param observer The observer used to wrap the problem. If NULL, the problem is returned unobserved.
 * @returns The next problem of the suite or NULL if there is no next problem left.
 */
coco_problem_t *coco_suite_get_next_problem(coco_suite_t *suite, coco_observer_t *observer) {

  size_t function_idx;
  size_t dimension_idx;
  size_t instance_idx;
  coco_problem_t *problem;

  long previous_function_idx = suite->current_function_idx;
  long previous_dimension_idx = suite->current_dimension_idx;
  long previous_instance_idx = suite->current_instance_idx;

  /* Iterate through the suite by instances, then functions and lastly dimensions in search for the next
   * problem. Note that these functions set the values of suite fields current_instance_idx,
   * current_function_idx and current_dimension_idx. */
  if (!coco_suite_is_next_instance_found(suite)
      && !coco_suite_is_next_function_found(suite)
      && !coco_suite_is_next_dimension_found(suite)) {
    coco_info_partial("done\n");
    return NULL;
  }

  if (suite->current_problem) {
    coco_problem_free(suite->current_problem);
  }

  assert(suite->current_function_idx >= 0);
  assert(suite->current_dimension_idx >= 0);
  assert(suite->current_instance_idx >= 0);

  function_idx = (size_t) suite->current_function_idx;
  dimension_idx = (size_t) suite->current_dimension_idx;
  instance_idx = (size_t) suite->current_instance_idx;

  problem = coco_suite_get_problem_from_indices(suite, function_idx, dimension_idx, instance_idx);
  if (observer != NULL)
    problem = coco_problem_add_observer(problem, observer);
  suite->current_problem = problem;

  /* Output information regarding the current place in the iteration */
  if (((long) dimension_idx != previous_dimension_idx)
      || ((dimension_idx == 0) && (previous_instance_idx < 0))) {
    /* A new dimension started */
    time_t timer;
    char time_string[30];
    struct tm* tm_info;
    time(&timer);
    tm_info = localtime(&timer);
    strftime(time_string, 30, "%d.%m.%y %H:%M:%S", tm_info);
    if (dimension_idx > 0)
      coco_info_partial("done\n");
    else
      coco_info_partial("\n");
    coco_info_partial("COCO INFO: %s, d=%lu, running: ", time_string, suite->dimensions[dimension_idx]);
  }
  if (((long) function_idx != previous_function_idx)
      || ((dimension_idx == 0) && (previous_instance_idx < 0))){
    /* A new function started */
    coco_info_partial("f%02lu", suite->functions[function_idx]);
  }
  if ((long) instance_idx != previous_instance_idx) {
    /* A new iteration started */
    coco_info_partial(".", suite->instances[instance_idx]);
  }

  return problem;
}

/**
 * Constructs a suite and observer given their options and runs the optimizer on all the problems in the
 * suite.
 *
 * @param suite_name A string containing the name of the suite. See suite_name in the coco_suite function for
 * possible values.
 * @param suite_instance A string used for defining the suite instances. See suite_instance in the coco_suite
 * function for possible values ("" and NULL result in default suite instances).
 * @param suite_options A string of pairs "key: value" used to filter the suite. See suite_options in the
 * coco_suite function for possible values ("" and NULL result in a non-filtered suite).
 * @param observer_name A string containing the name of the observer. See observer_name in the coco_observer
 * function for possible values ("", "no_observer" and NULL result in not using an observer).
 * @param observer_options A string of pairs "key: value" used to pass the options to the observer. See
 * observer_options in the coco_observer function for possible values ("" and NULL result in default observer
 * options).
 * @param optimizer An optimization algorithm to be run on each problem in the suite.
 */
void coco_run_benchmark(const char *suite_name,
                        const char *suite_instance,
                        const char *suite_options,
                        const char *observer_name,
                        const char *observer_options,
                        coco_optimizer_t optimizer) {

  coco_suite_t *suite;
  coco_observer_t *observer;
  coco_problem_t *problem;

  suite = coco_suite(suite_name, suite_instance, suite_options);
  observer = coco_observer(observer_name, observer_options);

  while ((problem = coco_suite_get_next_problem(suite, observer)) != NULL) {

    optimizer(problem);

  }

  coco_observer_free(observer);
  coco_suite_free(suite);

}

/* See coco.h for more information on encoding and decoding problem index */

/**
 * @param suite The suite.
 * @param function_idx Index of the function (starting with 0).
 * @param dimension_idx Index of the dimension (starting with 0).
 * @param instance_idx Index of the insatnce (starting with 0).
 * @return The problem index in the suite computed from function_idx, dimension_idx and instance_idx.
 */
size_t coco_suite_encode_problem_index(coco_suite_t *suite,
                                       const size_t function_idx,
                                       const size_t dimension_idx,
                                       const size_t instance_idx) {

  return instance_idx + (function_idx * suite->number_of_instances) +
      (dimension_idx * suite->number_of_instances * suite->number_of_functions);

}

/**
 * @param suite The suite.
 * @param problem_index Index of the problem in the suite (starting with 0).
 * @param function_idx Pointer to the index of the function, which is set by this function.
 * @param dimension_idx Pointer to the index of the dimension, which is set by this function.
 * @param instance_idx Pointer to the index of the instance, which is set by this function.
 */
void coco_suite_decode_problem_index(coco_suite_t *suite,
                                     const size_t problem_index,
                                     size_t *function_idx,
                                     size_t *dimension_idx,
                                     size_t *instance_idx) {

  if (problem_index > (suite->number_of_instances * suite->number_of_functions * suite->number_of_dimensions) - 1) {
    coco_warning("coco_suite_decode_problem_index(): problem_index too large");
    function_idx = 0;
    instance_idx = 0;
    dimension_idx = 0;
    return;
  }

  *instance_idx = problem_index % suite->number_of_instances;
  *function_idx = (problem_index / suite->number_of_instances) % suite->number_of_functions;
  *dimension_idx = problem_index / (suite->number_of_instances * suite->number_of_functions);

}
