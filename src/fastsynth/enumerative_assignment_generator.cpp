/*
 * enumerative_program_generator.cpp
 *
 *  Created on: 21 May 2018
 *      Author: elipol
 */

#include "enumerative_assignment_generator.h"
#include <langapi/language_util.h>
#include <util/expr_iterator.h>

void enumerative_assignment_generatort::set_to_true(const exprt &expr)
{
    assignment[expr]=true_exprt();
}

void enumerative_assignment_generatort::set_to(const exprt &expr, bool value)
{
  if(value)
    assignment[expr]=true_exprt();
  else
    assignment[expr]=false_exprt();
}

exprt enumerative_assignment_generatort::get(const exprt &expr) const
{
  PRECONDITION(assignment.count(expr)!=0);
  return assignment.at(expr);
}

void enumerative_assignment_generatort::print_assignment(
    std::ostream &out) const
{
  for(const auto & var : assignment)
  {
    out<<from_expr(ns, "", var.first) << " ";
    out<<from_expr(ns, "", var.second) << "\n";
  }
}

std::string enumerative_assignment_generatort::decision_procedure_text() const
{
  return "enumerative solver implemented for CEGIS\n";
}

void enumerative_assignment_generatort::use_assignment(
    std::vector<std::size_t> &assignment_indices)
{
  std::size_t sel_vec_index=0;
  for(const auto &sel_vec : selector_variables)
  {
    PRECONDITION(sel_vec.size()> assignment_indices[sel_vec_index]);
    for(std::size_t i=0; i<sel_vec.size(); i++)
    {
      if(i==assignment_indices[sel_vec_index])
      {
        assignment[sel_vec[i]]=true_exprt();
        get(sel_vec[i]);
      }
      else
      {
        assignment[sel_vec[i]]=false_exprt();
      }
    }
    sel_vec_index++;
  }
}


void enumerative_assignment_generatort::generate_nth_assignment(std::size_t n)
{
  std::size_t assignment_index=n+1; // don't do anything with index 0
  std::size_t local_n=n;
  for(const auto &sel_vec : selector_variables)
  {
    int size_of_vec = sel_vec.size();
    // Note we do not allow constants unless added into pre configured literals.
    // to enable, use size_of_vec+1 to allow for
    // the case where no selector variables
    // are true and we use a constant
    assignment_index=local_n%(size_of_vec);
      for(std::size_t i=0; i<sel_vec.size(); i++)
      {
        if(i==assignment_index)
        {
          assignment[sel_vec[i]]=true_exprt();
          get(sel_vec[i]);
        }
        else
        {
          assignment[sel_vec[i]]=false_exprt();
        }
      }
    local_n=local_n/(size_of_vec);
  }
}

void enumerative_assignment_generatort::find_variables(
    synth_encodingt &synth_encoding)
{
  selector_variables = synth_encoding.get_selector_variables();
  std::size_t index=0;
  for(const auto &v : synth_encoding.get_constant_variables())
  {
    irep_idt ID="constant_value"+std::to_string(index);
    assignment[v]=symbol_exprt(ID, unsignedbv_typet(32));
    index++;
  }
  number_of_options=1;
  for(const auto &sv : selector_variables)
  {
    debug() << "selector variable vector " << sv.size() << " ";
     number_of_options*=(sv.size());

  }
  debug() << "number of options " << number_of_options
      << ", program size "<< synth_encoding.program_size << "\n";

}
