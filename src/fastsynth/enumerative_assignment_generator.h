/*
 * enumerative_program_generator.h
 *
 *  Created on: 21 May 2018
 *      Author: elipol
 */

#ifndef CPROVER_FASTSYNTH_ENUMERATIVE_ASSIGNMENT_GENERATOR_H
#define CPROVER_FASTSYNTH_ENUMERATIVE_ASSIGNMENT_GENERATOR_H

#include <util/namespace.h>
#include <util/decision_procedure.h>

#include <fastsynth/synth_encoding.h>

/// generates an assignment to all unassigned variables
/// in the synth_encoding
class enumerative_assignment_generatort :
    public decision_proceduret
{
public:
  explicit enumerative_assignment_generatort(
      const namespacet &_ns,
      synth_encodingt synth_encoding):
  decision_proceduret(_ns),
  number_of_options(1u)
  {
  }

  // get a value for a variable
  virtual exprt get(const exprt &expr) const;

  /// find selector variables in the synth encoding
  void find_variables(synth_encodingt &synth_encoding);

  /// generate the nth assignment to the selector variables
  /// \param n nth assignment
  void generate_nth_assignment(std::size_t n);

  /// assign the selector variables using the given assignment
  /// \param assignment
  void use_assignment(std::vector<std::size_t> &assignment);

  virtual void set_to_true(const exprt &expr);
  virtual void set_to(const exprt &expr, bool value);

  /// required because this is an inherited class. Should never be used
  virtual resultt dec_solve()
  {
    // should never be called
    return resultt::D_ERROR;
  }

  /// number of possible programs that exist for given synth encoding
  std::size_t number_of_options;

  /// print the assignment
  /// \param out ostream to print to
  virtual void print_assignment(std::ostream &out) const;

  /// returns the decision procedure text to print when the
  /// solver is called. Again should never be called
  virtual std::string decision_procedure_text() const;

  /// vector of selector variables in the synth encoding
  std::vector<std::vector<exprt>> selector_variables;

  /// assignment to be used
  std::map<exprt, exprt> assignment;
};


#endif /* CPROVER_FASTSYNTH_ENUMERATIVE_ASSIGNMENT_GENERATOR_H */
