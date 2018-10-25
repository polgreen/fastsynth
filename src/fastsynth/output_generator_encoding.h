/*
 * output_generator_encoding.h
 *
 *  Created on: 14 May 2018
 *      Author: elipol
 */

#ifndef CPROVER_FASTSYNTH_OUTPUT_GENERATOR_ENCODING_H
#define CPROVER_FASTSYNTH_OUTPUT_GENERATOR_ENCODING_H

#include <util/decision_procedure.h>
#include <util/std_expr.h>

#include "cegis_types.h"

class output_generator_encodingt
{
public:
  output_generator_encodingt()
  {
  }

  exprt operator()(const exprt &);

  std::map<function_application_exprt, exprt> function_output_map;

  std::vector<std::pair<counterexamplet, counterexamplet>>
  get_output_example(const decision_proceduret &solver) const;

  using constraintst = std::list<exprt>;
  constraintst constraints;
};

#endif /* CPROVER_FASTSYNTH_OUTPUT_GENERATOR_ENCODING_H */
