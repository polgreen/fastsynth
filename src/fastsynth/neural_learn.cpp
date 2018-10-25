/*
 * neural_learn.cpp
 *
 *  Created on: 13 May 2018
 *      Author: elipol
 */
// #define DEBUG_CMD

#include <langapi/language_util.h>
#include <solvers/sat/satcheck.h>
#include <solvers/smt2/smt2irep.h>
#include <util/arith_tools.h>
#include <util/std_types.h>

#include "neural_learn.h"
#include "sygus_parser.h"

#include <fstream>
#include <iomanip>
#include <limits>
#include <random>
#include <sstream>

neural_learnt::neural_learnt(
  const namespacet &_ns,
  const problemt &_problem,
  message_handlert &_mh,
  std::size_t &_beam_size)
  : solver_learn_baset(_ns, _problem, _mh),
    generator_satcheck(new satcheckt()),
    output_generator(new bv_pointerst(_ns, *generator_satcheck)),
    pre_verify_batch(true),
    max_number_io(10u),
    seed(0u),
    tmp_results_filename("tmp"),
    beam_size(_beam_size),
    dummy_program_return_constant(0)
{
  PRECONDITION(problem.synth_fun_set.size() == 1);
  construct_output_generator();
  std::random_device rd;
  seed = rd();
}

void neural_learnt::reset_output_generator()
{
  generator_satcheck.reset(new satcheckt());
  output_generator.reset(new bv_pointerst(ns, *generator_satcheck));
  encoding.constraints.clear();
  encoding.function_output_map.clear();
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

decision_proceduret::resultt
neural_learnt::read_result(std::istream &in, verifyt &verifier)
{
  for(std::size_t i = 0; i < beam_size; i++)
  {
    std::string line;
    std::getline(in, line);
    if(line.size() == 0)
    {
      error() << "ERROR: Beam size is " << beam_size << " but only " << i
              << " functions were read in result" << eom;
      throw std::domain_error("Insufficient programs generated by network");
    }
    debug() << "Prog: " << line << eom;
    last_solution.functions.clear();

    typedef std::unordered_map<irep_idt, irept> valuest;
    valuest values;
    std::istringstream str(line);

    try
    {
      sygus_parsert parser(str);
      parser.set_message_handler(get_message_handler());
      parser.parse();

      if(parser && !parser.function_map.empty())
      {
        // add exprt tree to functions
        PRECONDITION(parser.function_map.size() == 1);
        for(const auto &f : parser.function_map)
        {
          last_solution.functions[symbol_exprt(f.first, f.second.type)] =
            f.second.body;
        }
        stock_solutions.push(last_solution);
      }
    }
    catch(...)
    {
      // continue if parser has failed
      warning() << "Network generated a syntactically incorrect program"
                << " or failed to generate programs" << eom;
    }
  }

  if(pre_verify_batch)
  {
    solutiont best_solution_so_far;
    std::size_t satisfied_inputs = 0;
    while(stock_solutions.size() > 0)
    {
      try
      {
        std::size_t new_satisfied_inputs =
          verifier(stock_solutions.front(), counterexamples);
        status() << "Solution " << stock_solutions.size() << " satisfied "
                 << new_satisfied_inputs << " inputs " << eom;

        if(new_satisfied_inputs > satisfied_inputs)
        {
          best_solution_so_far = stock_solutions.front();
          satisfied_inputs = new_satisfied_inputs;
        }
        if(satisfied_inputs == counterexamples.size())
          std::queue<solutiont>().swap(stock_solutions);
        else
          stock_solutions.pop();
      }
      catch(...)
      {
        status() << "Failed to parse; discarding solution "
                 << stock_solutions.size() << eom;
        stock_solutions.pop();
      }
    }
    last_solution = best_solution_so_far;
  }
  else if(beam_size > 1 && stock_solutions.size() > 0)
  {
    last_solution.functions.clear();
    last_solution = stock_solutions.front();
  }
  if(stock_solutions.size() > 0)
    stock_solutions.pop();
  return decision_proceduret::resultt::D_SATISFIABLE;
}

solutiont neural_learnt::dummy_program()
{
  solutiont result;

  symbol_exprt function_symbol = symbol_exprt(
    problem.synth_fun_set.begin()->first,
    problem.synth_fun_set.begin()->second);
  function_symbol.set_identifier(
    "synth_fun::" + id2string(function_symbol.get_identifier()));
  if(problem.synth_fun_set.begin()->second.codomain().id() == ID_bool)
  {
    result.functions[function_symbol] = true_exprt();
  }
  else if(
    problem.synth_fun_set.begin()->second.codomain().id() == ID_unsignedbv)
  {
    result.functions[function_symbol] =
      from_integer(dummy_program_return_constant, unsignedbv_typet(32));

    dummy_program_return_constant += 10;
  }

  return result;
}

decision_proceduret::resultt neural_learnt::operator()()
{
  verifyt verifier(ns, problem, get_message_handler());
  // only implemented to synthesise 1 function
  PRECONDITION(problem.synth_fun_set.size() == 1);

  if(stock_solutions.size() > 0)
  {
    PRECONDITION(beam_size > 1);
    last_solution = stock_solutions.front();
    stock_solutions.pop();
    return decision_proceduret::resultt::D_SATISFIABLE;
  }

  if(counterexamples.size() < 1u)
  {
    status() << "No counterexamples yet"
             << ", returning dummy program\n";
    // return empty program and get more counterexamples;
    last_solution = dummy_program();
    return decision_proceduret::resultt::D_SATISFIABLE;
  }
  else if(counterexamples.size() < 5u)
  {
    status() << "Only 1 counterexample. Generating "
             << "more random input/output examples\n";
    add_random_ces(counterexamples.back(), 5u);
  }

  // construct command line outpute
  command = "python ~/deepsynth/Python/CEGISInterface.py ";
  command += "-concatenateInputArity t "; // I have no idea what this does
  command += "-inputMode \"normBinary\" -lengthLimit 300 ";
  command += "-aliasing "; // name of function and function arguments
  command += " \"";
  command += id2string(problem.synth_fun_set.begin()->first);

  for(const auto &par : problem.synth_fun_set.begin()->second.domain())
    command += " " + id2string(par.get_identifier());
  command += "\" ";
  if(problem.synth_fun_set.begin()->second.codomain().id() == ID_bool)
    command += " -booleanFct t ";
  command += std::to_string(beam_size) + " "; // number of programs to output

  debug() << "Number of inputs: " << input_examples.size() << "\n";
  // inputs
  command += "\"[[";
  for(const auto &s : input_examples)
  {
    command += "[";
    for(const auto &in : s)
      command += in + " ";

    command += "]";
  }

  command += "]]\" ";

  // outputs
  command += "\"[";
  for(const auto &o : output_examples)
    command += o + " ";
  command += "]\"";

  command += " > ";
  command += tmp_results_filename;

#if defined(__linux__) || defined(__APPLE__)
  command += " 2>&1";
#endif

#ifdef DEBUG_CMD
  status() << "original command: " << command << eom;
  command =
    "echo \"(define-fun |synth_fun::mymax| ((x  (BitVec 32)) (y  (BitVec 32))) "
    "(BitVec 32) (ite (bvuge x y) y x))\"";
  command += "> " + tmp_results_filename;
#endif

  status() << "command: " << command << eom;

  int res = system(command.c_str());
  if(res < 0)
  {
    error() << "error running neural network" << eom;
    return decision_proceduret::resultt::D_ERROR;
  }

  std::ifstream in(tmp_results_filename);
  return read_result(in, verifier);
}

solutiont neural_learnt::get_solution() const
{
  return last_solution;
}


std::string neural_learnt::normalise(const exprt &expr)
{
  if(expr.type().id() == ID_unsignedbv)
  {
    std::string result;
    std::stringstream convert;
    unsigned int value = stol(from_expr(ns, "", expr));
    double normalised = (static_cast<double>(value) / (2147483648)) - 1;
    convert << std::setprecision(32) << normalised;
    POSTCONDITION(normalised <= 1 && normalised >= -1);
    return convert.str();
  }
  else
    return from_expr(ns, "", expr);
}

void neural_learnt::add_random_ces(const counterexamplet &c, std::size_t n)
{
  std::mt19937 gen(seed);
  counterexamplet cex = c;
  for(std::size_t i = 0; i < n; i++)
  {
    counterexamplet random_cex;
    for(const auto &it : cex.assignment)
    {
      std::uniform_int_distribution<unsigned int> dis(
        0, std::numeric_limits<char32_t>::max());
      const exprt &symbol = it.first;
      mp_integer number = dis(gen);
      constant_exprt value(integer2binary(number, 32), unsignedbv_typet(32));
      random_cex.assignment[symbol] = value;
    }
    add_ce(random_cex, false);
  }
  std::uniform_int_distribution<unsigned int> dis2(
    0, std::numeric_limits<char32_t>::max());
  seed = dis2(gen);
}

void neural_learnt::add_ce(const counterexamplet &cex)
{
  bool is_new_cex = true;
  for(const auto &old_cex : counterexamples)
  {
    if(old_cex.assignment == cex.assignment)
    {
      is_new_cex = false;
      break;
    }
  }
  if(is_new_cex)
    add_ce(cex, false);
  else
    add_random_ces(cex, 1);
}

void neural_learnt::add_ce(const counterexamplet &cex, bool add_random_cex)
{
  reset_output_generator();
  counterexamples.emplace_back(cex);

  // add counterexamples to output generator
  for(const auto &var : cex.assignment)
  {
    const exprt &symbol = var.first;
    const exprt &value = var.second;
    exprt encoded = encoding(equal_exprt(symbol, value));
    output_generator->set_to_true(encoded);
  }

  // get output
  POSTCONDITION(
    output_generator->operator()() != decision_proceduret::resultt::D_ERROR);

  // add outputs and inputs to command;
  for(const auto &example_pair : encoding.get_output_example(*output_generator))
  {
    const counterexamplet &output_cex = example_pair.second;
    const counterexamplet &input_cex = example_pair.first;

    POSTCONDITION(output_cex.assignment.size() == 1);
    output_examples.push_back(normalise(output_cex.assignment.begin()->second));

    std::size_t index=0;
    for(const auto &input_ass : input_cex.assignment)
    {
      if(input_examples.size() <= index)
        input_examples.push_back({{normalise(input_ass.second)}});
      else
        input_examples[index].push_back(normalise(input_ass.second));

      index++;
    }
  }


  while(output_examples.size() > max_number_io)
  {
    // debug() << "Number of CEX: "<< input_examples[index].size()
    // <<", removing counterexamples from beginning\n";

    for(auto &i : input_examples)
      i.erase(i.begin());

    output_examples.erase(output_examples.begin());
  }

  while(counterexamples.size() >
        max_number_io * encoding.function_output_map.size())
    counterexamples.erase(counterexamples.begin());

  debug() << "Number of counterexamples" << counterexamples.size() << "\n";
  for(const auto &i : input_examples)
    debug() << "Num input examples " << i.size() << "\n";
  debug() << "Number of output examples: " << output_examples.size() << "\n";

  if(add_random_cex)
    add_random_ces(cex, 1u);
}
