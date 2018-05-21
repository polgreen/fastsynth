/*
 * neural_learn.cpp
 *
 *  Created on: 13 May 2018
 *      Author: elipol
 */
#define DEBUG_CMD

#include <solvers/smt2/smt2irep.h>
#include <langapi/language_util.h>
#include <solvers/sat/satcheck.h>
#include <util/arith_tools.h>

#include "neural_learn.h"
#include "sygus_parser.h"

#include <fstream>

neural_learnt::neural_learnt(
    const namespacet &_ns,
    const problemt &_problem,
    message_handlert &_mh):
    solver_learn_baset(_ns, _problem, _mh),
    output_generator(_ns, *new satcheck_no_simplifiert()),
    message_handler(_mh),
    tmp_results_filename("tmp"),
    beam_size(1u),
    dummy_program_return_constant(0)
{
    PRECONDITION(problem.synth_fun_set.size()==1);
    construct_output_generator(_problem);
}



void neural_learnt::construct_output_generator(
    const problemt &problem)
{
  debug() << "construct output generator for neural network" << eom;
  for(const auto &e : problem.side_conditions)
  {
    const exprt encoded = encoding(e);
    output_generator.set_to_true(encoded);
  }

  for(const auto &e : problem.constraints)
  {
    const exprt encoded = encoding(e);
    output_generator.set_to_true(encoded);
  }

  for(const auto &c : encoding.constraints)
  {
    output_generator.set_to_true(c);
    debug() << "ec: " << from_expr(ns, "", c) << eom;
  }
}


decision_proceduret::resultt neural_learnt::read_result(std::istream &in)
{
  std::string line;
  last_solution.functions.clear();

  typedef std::unordered_map<irep_idt, irept> valuest;
  valuest values;

  sygus_parsert parser(in);
  parser.set_message_handler(message_handler);

  parser.parse();

  if(!parser || parser.function_map.empty())
    return decision_proceduret::resultt::D_ERROR;

  // add exprt tree to functions
  if(beam_size==1)
    PRECONDITION(parser.function_map.size()==1);
  for(const auto & f : parser.function_map)
  {
    last_solution.functions[symbol_exprt(
        f.first, f.second.type)] = f.second.body;
  }

  return decision_proceduret::resultt::D_SATISFIABLE;
}

solutiont neural_learnt::dummy_program()
{
  solutiont result;

  symbol_exprt function_symbol = problem.synth_fun_set[0];
  function_symbol.set_identifier(
      "synth_fun::"+id2string(function_symbol.get_identifier()));

  result.functions[function_symbol] =
      from_integer(dummy_program_return_constant, unsignedbv_typet(32));

  dummy_program_return_constant+=10;

  return result;
}

decision_proceduret::resultt neural_learnt::operator()()
{
  if(counterexamples.size()<5u)
  {
    status() << "not enough counterexamples to call NN yet"
              << ", returning dummy program\n";
    // return empty program and get more counterexamples;
    last_solution=dummy_program();
    return decision_proceduret::resultt::D_SATISFIABLE;
  }

  // construct command line output
  // inputs
  command="[[";
  for(const auto &s : input_examples)
    command+="[ "+ s +" ]";

  command+="]]";

  // outputs
  command+="[[["+ output_examples+"]]]";

  command += " > ";
  command += tmp_results_filename;

#if defined(__linux__) || defined(__APPLE__)
  command += " 2>&1";
#endif


#ifdef DEBUG_CMD
  command="echo \"(define-fun |synth_fun::mymax| ((x  (BitVec 32)) (y  (BitVec 32))) (BitVec 32) (ite (bvuge x y) y x))\"";
  command+= "> " + tmp_results_filename;
#endif

  status() <<"command: " << command << eom;

  int res = system(command.c_str());
  if(res < 0)
  {
    error() << "error running neural network" << eom;
    return decision_proceduret::resultt::D_ERROR;
  }

  std::ifstream in(tmp_results_filename);
  return read_result(in);
}

solutiont neural_learnt::get_solution() const
{
  return last_solution;
}

void neural_learnt::add_ce(const counterexamplet & cex)
{
  counterexamples.emplace_back(cex);
  std::size_t index=0;
  // get output example
  for(const auto &it : cex.assignment)
  {
    const exprt &symbol = it.first;
    const exprt &value = it.second;
// add input to command
    if(input_examples.size()<=index)
      input_examples.push_back(from_expr(ns, "", it.second)+", ");
    else
      input_examples[index]+=from_expr(ns, "", it.second)+", ";
    debug() << "input examples: "<< input_examples[index] << eom;
    index++;

// add input to solver
    exprt encoded = encoding(equal_exprt(symbol, value));
    debug() << "ce: " << from_expr(ns, "", encoded) << eom;
    output_generator.set_to_true(encoded);
  }
// get output
  POSTCONDITION(output_generator()!=decision_proceduret::resultt::D_ERROR);

  // add output to command;
  for(const auto &it:
      encoding.get_output_example(output_generator).assignment)
  {
    output_examples+=from_expr(ns, "", it.second)+", ";
  }
  debug() <<"output examples: "<< output_examples << eom;
}



