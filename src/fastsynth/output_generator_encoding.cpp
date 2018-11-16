/*
 * output_generator_encoding.cpp
 *
 *  Created on: 14 May 2018
 *      Author: elipol
 */

#include "output_generator_encoding.h"
#include <util/arith_tools.h>
#include <iostream>
#include <string>
exprt output_generator_encodingt::operator()(const exprt &expr)
{
  if(expr.id()==ID_function_application)
  {
    const function_application_exprt func_app =
      to_function_application_expr(expr);
    // replace with nondet symbol
    if(func_app.function().get_identifier() ==
      "synth_fun::inv-f")
    {
      bool new_func_app=true;
      std::size_t index=0;
      for(const auto &old_func_app : function_output_map)
      {
        if(func_app.arguments()==old_func_app.first.arguments())
        {
          new_func_app=false;
          break;
        }
       index++;
      }

      std::string name =
        id2string(
         func_app.function().get_identifier()) +
        "_ret" + std::to_string(index);

      nondet_symbol_exprt tmp(name,
        func_app.type());

      if(new_func_app)
      {
        function_output_map[func_app]=tmp;
      }
      return tmp;
    }
  }

  exprt tmp = expr;
  for(auto &op : tmp.operands())
    op = (*this)(op); // recursive call

  return tmp;
}

std::vector<std::pair<counterexamplet, counterexamplet>>
output_generator_encodingt::get_output_example(
  const decision_proceduret &solver) const
{
  std::vector<std::pair<counterexamplet, counterexamplet>> result;

  // iterate over nondet symbols and get their value

  for(const auto &func_output : function_output_map)
  {
    counterexamplet input_examples, output_examples;

    exprt output_value = solver.get(func_output.second);
    output_examples.assignment[func_output.second] = output_value;

    if(
      output_value == nil_exprt() &&
      func_output.second.id() == ID_nondet_symbol)
    {
      nondet_symbol_exprt tmp_var = to_nondet_symbol_expr(func_output.second);
      tmp_var.set_identifier(
        "nondet_" +
        id2string(to_nondet_symbol_expr(func_output.second).get_identifier()));
      output_value = solver.get(tmp_var);
      output_examples.assignment[func_output.second] = output_value;
    }
    if(output_value == nil_exprt())
    {
      std::cout << "Warning: unable to find value for "
                << func_output.second.pretty() << std::endl;
      output_examples.assignment[func_output.second] =
                from_integer(0, func_output.second.type());

      std::cout << "Assume has been simplified out by solver.\n" << std::endl;
//      std::cout <<"returning "<< from_integer(0, func_output.second.type()).pretty() <<std::endl;

    }
    for(const auto &arg : func_output.first.arguments())
      input_examples.assignment[arg] = solver.get(arg);

    std::pair<counterexamplet, counterexamplet> function_application_cex(
      input_examples, output_examples);
    result.push_back(function_application_cex);
  }

  return result;
}
