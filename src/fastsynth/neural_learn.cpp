/*
 * neural_learn.cpp
 *
 *  Created on: 13 May 2018
 *      Author: elipol
 */
//#define DEBUG_CMD

#include <solvers/smt2/smt2irep.h>
#include <langapi/language_util.h>
#include <solvers/sat/satcheck.h>
#include <util/arith_tools.h>
#include <util/std_types.h>

#include "neural_learn.h"
#include "sygus_parser.h"

#include <fstream>
#include <limits>
#include <random>
#include <sstream>
#include <iomanip>

neural_learnt::neural_learnt(
    const namespacet &_ns,
    const problemt &_problem,
    message_handlert &_mh):
    solver_learn_baset(_ns, _problem, _mh),
    generator_satcheck(new satcheckt()),
    output_generator(new bv_pointerst(_ns, *generator_satcheck)),
    seed(0u),
    tmp_results_filename("tmp"),
    beam_size(1u),
    dummy_program_return_constant(0)
{
    PRECONDITION(problem.synth_fun_set.size()==1);
    construct_output_generator();
    std::random_device rd;
    seed = rd();
}

void neural_learnt::reset_output_generator()
{
  generator_satcheck.reset(new satcheckt());
  output_generator.reset(new bv_pointerst(ns, *generator_satcheck));
  encoding.constraints.clear();
  encoding.function_outputs.clear();
  construct_output_generator();
}

void neural_learnt::construct_output_generator()
{
  debug() << "construct output generator for neural network" << eom;
  for(const auto &e : problem.side_conditions)
  {
    const exprt encoded = encoding(e);
    output_generator->set_to_true(encoded);
  }

  for(const auto &e : problem.constraints)
  {
    const exprt encoded = encoding(e);
    output_generator->set_to_true(encoded);
  }

  for(const auto &c : encoding.constraints)
  {
    output_generator->set_to_true(c);
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
  parser.set_message_handler(get_message_handler());

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

  symbol_exprt function_symbol =
      symbol_exprt(
          problem.synth_fun_set.begin()->first,
          problem.synth_fun_set.begin()->second);
  function_symbol.set_identifier(
      "synth_fun::"+id2string(function_symbol.get_identifier()));

  result.functions[function_symbol] =
      from_integer(dummy_program_return_constant, unsignedbv_typet(32));

  dummy_program_return_constant+=10;

  return result;
}


decision_proceduret::resultt neural_learnt::operator()()
{
  // only implemented to synthesise 1 function
  PRECONDITION(problem.synth_fun_set.size()==1);
  // handling beam size of greater than 1 need to be implemented
  PRECONDITION(beam_size==1);


  if(counterexamples.size()<1u)
  {
    status() << "No counterexamples yet"
              << ", returning dummy program\n";
    // return empty program and get more counterexamples;
    last_solution=dummy_program();
    return decision_proceduret::resultt::D_SATISFIABLE;
  }
  else if(counterexamples.size()==1u)
  {
    status() << "Only 1 counterexample. Generating "
             << "more random input/output examples\n";
    add_random_ces(counterexamples.back(), 4u);
  }

  // construct command line outpute
  command="python ~/deepsynth/Python/CEGISInterface.py ";
  command+="-concatenateInputArity t "; // I have no idea what this does
  command+="-inputMode \"normBinary\" -lengthLimit 300 ";
  command+="-aliasing "; // name of function and function arguments
  command+= " \"";
  command+= id2string(problem.synth_fun_set.begin()->first);

  for(const auto &par : problem.synth_fun_set.begin()->second.domain())
    command+=" "+id2string(par.get_identifier());
  command+="\" ";
  command+=std::to_string(beam_size)+ " "; // number of programs to output

  // inputs
  command+="\"[[";
  for(const auto &s : input_examples)
    command+="["+ s +"]";

  command+="]]\" ";

  // outputs
  command+="\"[[["+ output_examples+"]]]\"";

  command += " > ";
  command += tmp_results_filename;

#if defined(__linux__) || defined(__APPLE__)
  command += " 2>&1";
#endif


#ifdef DEBUG_CMD
  status() << "original command: " << command << eom;
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



std::string neural_learnt::normalise(const exprt &expr)
{
  std::string result;
  std::stringstream convert;
  unsigned int value = stol(from_expr(ns, "", expr));
  double normalised = (static_cast<double>(value)/(2147483648)) - 1;
  convert << std::setprecision(32)<<normalised;
  POSTCONDITION(normalised <= 1 && normalised >= -1);
  return convert.str();
}


void neural_learnt::add_random_ces(const counterexamplet &c, std::size_t n)
{
  std::mt19937 gen(seed);
  counterexamplet cex = c;
  for(std::size_t i=0; i<n; i++)
  {
    counterexamplet random_cex;
    for(const auto &it : cex.assignment)
    {
      std::uniform_int_distribution<>dis(
          0, std::numeric_limits<char32_t>::max());
      const exprt &symbol = it.first;
      mp_integer number = dis(gen);
      constant_exprt value(integer2binary(number, 32), unsignedbv_typet(32));
      random_cex.assignment[symbol] = value;
    }
    add_ce(random_cex);
  }
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
      input_examples.push_back(normalise(value));
    else
      input_examples[index]+=" "+normalise(value);
    index++;

// add input to solver
    exprt encoded = encoding(equal_exprt(symbol, value));
    output_generator->set_to_true(encoded);
  }
// get output
  POSTCONDITION(output_generator->operator ()()!=
      decision_proceduret::resultt::D_ERROR);

  // add output to command;
  for(const auto &o_it:
      encoding.get_output_example(*output_generator).assignment)
  {
    output_examples+=normalise(o_it.second)+" ";
  }
  for(const auto &s : input_examples)
    debug() << "inputs: " << s << eom;
  debug() <<"output examples: "<< output_examples << eom;
}

