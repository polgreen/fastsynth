/*
 * output_generator_encoding.h
 *
 *  Created on: 14 May 2018
 *      Author: elipol
 */

#ifndef CPROVER_OUTPUT_GENERATOR_ENCODING_H_
#define CPROVER_OUTPUT_GENERATOR_ENCODING_H_

#include <util/std_expr.h>
#include <util/decision_procedure.h>


#include "cegis_types.h"

class output_generator_encodingt
{
public:
  output_generator_encodingt()
  {
  }

  exprt operator()(const exprt &);

  std::vector<exprt>function_outputs;

  counterexamplet get_output_example(const decision_proceduret &solver) const;

  using constraintst=std::list<exprt>;
  constraintst constraints;
};

#endif /* CPROVER_OUTPUT_GENERATOR_ENCODING_H_ */

