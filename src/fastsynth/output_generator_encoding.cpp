/*
 * output_generator_encoding.cpp
 *
 *  Created on: 14 May 2018
 *      Author: elipol
 */

#include "output_generator_encoding.h"

exprt output_generator_encodingt::operator()(const exprt &expr)
{
  if(expr.id()==ID_function_application)
  {
    // replace with nondet symbol
    nondet_symbol_exprt tmp("test",
        to_function_application_expr(expr).type());
    function_outputs.push_back(tmp);
    return tmp;
  }
  else if(expr.id()==ID_symbol)
  {
    // add the suffix
    symbol_exprt tmp=to_symbol_expr(expr);
    tmp.set_identifier(id2string(tmp.get_identifier()));
    return tmp;
  }
  else if(expr.id()==ID_nondet_symbol)
  {
    // add the suffix
    nondet_symbol_exprt tmp=to_nondet_symbol_expr(expr);
    irep_idt identifier=tmp.get_identifier();
    tmp.set_identifier(id2string(identifier));
    return tmp;
  }
  else
  {
    exprt tmp=expr;

    for(auto &op : tmp.operands())
      op=(*this)(op); // recursive call

    return tmp;
  }
}
#include <iostream>
counterexamplet output_generator_encodingt::get_output_example(
    const decision_proceduret &solver) const
{
  counterexamplet result;
  // iterate over nondet symbols and get their value
  for(const auto &var : function_outputs)
  {
    exprt value=solver.get(var);
    result.assignment[var]=value;
    if(value==nil_exprt() && var.id()==ID_nondet_symbol)
    {
      nondet_symbol_exprt tmp_var=to_nondet_symbol_expr(var);
      tmp_var.set_identifier(
          "nondet_"+id2string(to_nondet_symbol_expr(var).get_identifier()));
      value=solver.get(tmp_var);
      result.assignment[var]=value;
    }
    if(value==nil_exprt())
    {
      std::cout << "Warning: unable to find value for "
          << var.pretty()<<std::endl;
      result.assignment[var] = constant_exprt("0", var.type());
      std::cout<<"Assume has been simplified out by solver.\n" <<std::endl;
    }
  }
  return result;
}

