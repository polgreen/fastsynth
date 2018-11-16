/*
 * neural_learn.cpp
 *
 *  Created on: 13 May 2018
 *      Author: elipol
 */
// #define DEBUG_CMD

#include <langapi/language_util.h>
#include <solvers/sat/satcheck.h>
#include <solvers/smt2/smt2_dec.h>
#include <solvers/smt2/smt2irep.h>
#include <util/arith_tools.h>
#include <util/std_types.h>

#include "neural_learn.h"
#include "sygus_parser.h"
#include "solver.h"

#include <fstream>
#include <iomanip>
#include <limits>
#include <random>
#include <sstream>

neural_learnt::neural_learnt(
  const namespacet &_ns,
  const problemt &_problem,
  message_handlert &_mh,
  std::size_t &_beam_size,
  bool use_simple_network,
  bool standalone_testing)
  : solver_learn_baset(_ns, _problem, _mh),
    generator_satcheck(new satcheckt()),
    output_generator(new bv_pointerst(_ns, *generator_satcheck)),
	num_programs_to_store_per_batch(_beam_size),
	network_call_num(0u),
	single_call(standalone_testing),
    simple_network(use_simple_network),
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
  tmp_results_filename = "tmp" + std::to_string(rd());
}

neural_learnt::neural_learnt(
  const namespacet &_ns,
  const problemt &_problem,
  message_handlert &_mh,
  std::size_t &_beam_size,
  bool use_simple_network,
  bool standalone_testing,
  std::size_t _num_programs_to_store_per_batch,
  std::size_t _max_num_io):
  solver_learn_baset(_ns, _problem, _mh),
  generator_satcheck(new satcheckt()),
  output_generator(new bv_pointerst(_ns, *generator_satcheck)),
  num_programs_to_store_per_batch(_num_programs_to_store_per_batch),
  network_call_num(0u),
  single_call(standalone_testing),
  simple_network(use_simple_network),
  pre_verify_batch(true),
  max_number_io(_max_num_io),
  seed(0u),
  tmp_results_filename("tmp"),
  beam_size(_beam_size),
  dummy_program_return_constant(0)
{
	PRECONDITION(problem.synth_fun_set.size() == 1);
	construct_output_generator();
	std::random_device rd;
	tmp_results_filename = "tmp" + std::to_string(rd());
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

  const exprt encoded=encoding(conjunction(problem.constraints));
  output_generator->set_to_true(encoded);

}

void neural_learnt::reset_inv_output_generator(
		output_generator_encodingt::inv_constraintst inv_constraint_index)
{
  output_generator.reset(new smt2_dect(ns, "fastsynth", "created by fastsynth",
                    "BV", smt2_dect::solvert::Z3));
  encoding.constraints.clear();
  encoding.function_output_map.clear();
  construct_inv_output_generator(inv_constraint_index);
}

void neural_learnt::construct_inv_output_generator(
		output_generator_encodingt::inv_constraintst inv_limits)
{
  debug() << "construct output generator for neural network" << eom;
  for(const auto &e : problem.side_conditions)
  {
    const exprt encoded = encoding(e);
    output_generator->set_to_true(encoded);
  }

  exprt::operandst tmp;
  for(const auto &c: problem.constraints)
	  tmp.push_back(c);

  tmp.push_back(problem.output_generator_constraints[(int)inv_limits]);
  debug()<<"inv constraint " << inv_limits <<" set to true \n";

  const exprt encoded=encoding(conjunction(tmp));
  output_generator->set_to_true(encoded);

}



decision_proceduret::resultt
neural_learnt::read_result(std::istream &in, verifyt &verifier)
{
  std::queue<solutiont> network_solutions;
  for(std::size_t i = 0; i < beam_size; i++)
  {
    std::string line;
    std::getline(in, line);

    if(line.size() == 0)
    {
      warning() << "ERROR: Beam size is " << beam_size << " but only " << i
              << " functions were read in result" << eom;
      break;
    }

    status() << "Prog: " << line << eom;
    last_solution.functions.clear();

    typedef std::unordered_map<irep_idt, irept> valuest;
    valuest values;
    std::istringstream str(line);


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
        network_solutions.push(last_solution);
      }
      else
    	warning() << "Syntactically incorrect program \n";
  }
  if(network_solutions.size()==0)
  {
    last_solution = dummy_program();
    return decision_proceduret::resultt::D_SATISFIABLE;
  }


  if(pre_verify_batch)
  {
    solutiont best_solution_so_far;
    std::size_t satisfied_inputs = 0;
    std::size_t satisfying_programs=0;
    while(network_solutions.size() > 0)
    {
      try
      {
        std::size_t new_satisfied_inputs =
          verifier(network_solutions.front(), counterexamples);
        status() << "Solution " << network_solutions.size() << " satisfied "
                 << new_satisfied_inputs << " inputs of " << counterexamples.size()<< eom;

        if(new_satisfied_inputs > satisfied_inputs)
        {
          best_solution_so_far = network_solutions.front();
          satisfied_inputs = new_satisfied_inputs;
        }
        if(satisfied_inputs == counterexamples.size())
        {
        	stock_solutions.push(network_solutions.front());
        	satisfying_programs++;
        	if(satisfying_programs==num_programs_to_store_per_batch)
        		std::queue<solutiont>().swap(network_solutions);
        }
        if(network_solutions.size()>0)
          network_solutions.pop();
      }
      catch(...)
      {
        status() << "Failed to parse; discarding solution "
                 << stock_solutions.size() << eom;
        network_solutions.pop();
      }
    }

    status() << "Found " << stock_solutions.size() << " solutions that satisfy all inputs \n";
    if(satisfied_inputs==counterexamples.size())
    	stock_solutions.pop();

    last_solution = best_solution_so_far;
  }
  else if(beam_size > 1 && network_solutions.size() > 0)
  {
    last_solution.functions.clear();
    last_solution = network_solutions.front();
  }
 // if(network_solutions.size() > 0)
   // network_solutions.pop();
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
  else if(output_examples.size() < max_number_io)
  {
    status() << "Not enough counterexamples. Generating "
             << "more random input/output examples\n";
    while(output_examples.size() < max_number_io-1)
    {
      status()<<"Adding another random example, number of counterexamples "<<counterexamples.size()<<" \n";
      add_random_ces(counterexamples.back(), counterexamples.size()%4);
    }  
  }

  // construct command line output
  if(single_call && network_call_num==1)
  {
	  status() << "Reached network call limit \n";
	  return decision_proceduret::resultt::D_UNSATISFIABLE;
  }


  status() << "Neural network call "<< network_call_num <<eom;
  network_call_num++;


  if(!simple_network)
  {
	command = "python ~/deepsynth/Python/CEGISInterface.py ";
    command += "-concatenateInputArity f "; // I have no idea what this does
    command += "-inputMode \"normBinary\" -lengthLimit 300 ";
  }
  else
	command = "python ~/deepsynth/Python/CEGISSimpleInterface.py ";
  
  command += "-aliasing "; // name of function and function arguments
  command += " \"";
  command += id2string(problem.synth_fun_set.begin()->first);

  for(const auto &par : problem.synth_fun_set.begin()->second.domain())
    command += " " + id2string(par.get_identifier());
  command += "\" ";
  if(problem.synth_fun_set.begin()->second.codomain().id() == ID_bool)
    command += " -booleanFct t ";
  if(!simple_network)
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
  decision_proceduret::resultt final_result =
      decision_proceduret::resultt::D_ERROR;
  try
  {
    final_result = read_result(in, verifier);
  }
  catch(...)
  {
    // have another go
    final_result = read_result(in, verifier);
  }
  return final_result;
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
    debug() << "Normalised "<< std::to_string(value) << " to " << convert.str() << eom;
    POSTCONDITION(normalised <= 1 && normalised >= -1);
    return convert.str();
  }
  else if(expr.type().id()==ID_nil)
  {
    debug() <<" Got nil exprt, replacing with 0" <<eom;
    return "0";
  }
  else
  {
	debug() <<" no need to normalise "<< expr.pretty()<<eom;
    return from_expr(ns, "", expr);
  }
}

void neural_learnt::add_random_ces(
    const counterexamplet &c, std::size_t index_of_constraint )
{
  std::mt19937 gen(seed);
  counterexamplet cex = c;

	// this currently uses over-approximate example generation, i.e., assumes the invariant is
	// true when unknown.
	// TODO: make this code nicer. Add option to use UNKNOWN_ASSUME_FALSE
	if(problem.output_generator_constraints.size()>0 && index_of_constraint%4!=0)
	  reset_inv_output_generator(
			  static_cast<output_generator_encodingt::inv_constraintst>(index_of_constraint));
	else
	  reset_output_generator();
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
//    if(!is_duplicate_counterexample(random_cex))
      add_ce(random_cex,
    		 problem.output_generator_constraints.size()>0);


  std::uniform_int_distribution<unsigned int> dis2(
    0, std::numeric_limits<char32_t>::max());
  seed = dis2(gen);
}


bool neural_learnt::is_duplicate_counterexample(const counterexamplet &cex) {
	bool is_new_cex = true;
	for (const auto &old_cex : counterexamples) {
		if (old_cex.assignment == cex.assignment) {
			is_new_cex = false;
			break;
		}
	}
	if(output_examples.size()>5)
	{
        std::size_t total_false=0;
        std::size_t total_true=0;
        for(const auto &output: output_examples)
        {
          if(output == "FALSE")
            total_false++;
          if(output == "TRUE")
            total_true++;
        }
        if(total_false==output_examples.size()|| total_true==output_examples.size())
          is_new_cex=false;
          }
	return !is_new_cex;
}


void neural_learnt::add_complementary_cex(const counterexamplet &cex)
{
  std::size_t total_false=0;
  std::size_t total_true=0;
  for(const auto &output: output_examples)
  {
    if(output == "FALSE")
      total_false++;
    if(output == "TRUE")
      total_true++;
  }

  if(total_false>total_true)
    add_random_ces(cex,0);
  else
    add_random_ces(cex,1);
}

void neural_learnt::add_ce(const counterexamplet &cex)
{
  bool duplicate = is_duplicate_counterexample(cex);

  if(!duplicate && cex.f_apps.empty())
    add_ce(cex, false);
  else if(!duplicate && !cex.f_apps.empty()
      && problem.synth_fun_set.begin()->second.codomain().id() == ID_bool)
	  add_ce_with_outputs(cex);
//  else
    add_complementary_cex(cex);
}




void neural_learnt::add_ce_with_outputs(const counterexamplet &cex)
{

  counterexamples.emplace_back(cex);
  std::size_t index=0;
  std::size_t function_calls = cex.f_apps.size();
  std::size_t number_of_vars = problem.synth_fun_set.begin()->second.domain().size();

  PRECONDITION(number_of_vars == cex.assignment.size()/function_calls);
  std::vector<std::string> empty_vector;

  while(input_examples.size()<number_of_vars)
    input_examples.push_back(empty_vector);

  for(const auto &input_ass : cex.assignment)
  {
    if(index==number_of_vars)
      break;
    input_examples[index].push_back(normalise(input_ass.second));
    index++;
  }

  std::size_t function_call_index=0;
  for(const auto & f_app : cex.f_apps)
  {
    if(function_call_index == function_calls-1)
      break;
    if(f_app.second==true_exprt())
      output_examples.push_back(normalise(false_exprt()));
    else
      output_examples.push_back(normalise(true_exprt()));
    function_call_index++;
  }

  while(output_examples.size() > max_number_io)
    {
      debug() << "Number of CEX: "<< counterexamples.size()
       <<", removing counterexamples from beginning\n";

      for(auto &i : input_examples)
        i.erase(i.begin());

      output_examples.erase(output_examples.begin());
      counterexamples.erase(counterexamples.begin());
    }
    debug()<<"CEX: counterexamples size: "<<counterexamples.size();
}



void neural_learnt::add_ce(const counterexamplet &cex, bool add_random_cex)
{
  if(!add_random_cex)
    reset_output_generator();

  counterexamples.emplace_back(cex);

  // add counterexamples to output generator
  for(const auto &var : cex.assignment)
  {
    const exprt &symbol = var.first;
    const exprt &value = var.second;
    exprt encoded = encoding(equal_exprt(symbol, value));
    if(! add_random_cex)
      output_generator->set_to_true(encoded);
  }

  // get output
  POSTCONDITION(
    output_generator->operator()() != decision_proceduret::resultt::D_ERROR);

  std::size_t function_calls=0;
  // add outputs and inputs to command;
  for(const auto &example_pair : encoding.get_output_example(*output_generator))
  {
    if(function_calls==1)
     break;
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
    function_calls++;
  }


  while(output_examples.size() > max_number_io)
  {
    debug() << "Number of CEX: "<< output_examples.size()
     <<", removing counterexamples from beginning\n";

    for(auto &i : input_examples)
      i.erase(i.begin());

    output_examples.erase(output_examples.begin());
  }

  while(counterexamples.size() > (output_examples.size() / function_calls))
    counterexamples.erase(counterexamples.begin());
}
