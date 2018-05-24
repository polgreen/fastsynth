/*
 * enumerative_learn.cpp
 *
 *  Created on: 29 Mar 2018
 *      Author: elipol
 */

#include "enumerative_learn.h"
#include "verify.h"
#include <solvers/smt2/smt2_conv.h>
#include <solvers/flattening/bv_pointers.h>
#include <solvers/sat/satcheck.h>
#include <langapi/language_util.h>
#include <iostream>
#include <util/simplify_expr.h>
#include <util/expr_iterator.h>

void enumerative_learnt::output_program(
    const solutiont &solution, std::ostream &out) const
{
  for(const auto &f : solution.functions)
    {
      std::string stripped_id=
              std::string(
                  id2string(f.first.get_identifier()),
                  11, std::string::npos);

            out << "C Result: "
                             << stripped_id
                             << " -> "
                             << from_expr(ns, "", f.second)
                             << '\n';
    }
}

enumerative_learnt::enumerative_learnt(
  const namespacet &_ns,
  const problemt &_problem,
  message_handlert &_message_handler):
  solver_learn_baset(_ns, _problem, _message_handler),
  program_size(1u),
  program_index(0u)
{
}

void enumerative_learnt::set_program_size(size_t program_size)
{
  if(program_size!= this->program_size)
  {
    this->program_size=program_size;
    program_index=0u;
  }
}

solutiont enumerative_learnt::get_solution() const
{
  return last_solution;
}

void enumerative_learnt::add_ce(const counterexamplet &cex)
{
  counterexamples.emplace_back(cex);
}



decision_proceduret::resultt enumerative_learnt::operator()()
{
  synth_encodingt synth_encoding;
  synth_encoding.program_size = program_size;
  synth_encoding.enable_bitwise = enable_bitwise;
  synth_encoding.literals = problem.literals;
  enumerative_assignment_generatort solver(ns, synth_encoding);

  synth_encodingt synth_encoding_verif;
  synth_encoding_verif.program_size = program_size;
  synth_encoding_verif.enable_bitwise = enable_bitwise;
  synth_encoding_verif.literals = problem.literals;

  verifyt solution_verifier(ns, problem, get_message_handler());
  synth_encoding.suffix = "$ce";
  synth_encoding.constraints.clear();

  add_problem(synth_encoding, solver);

  solver.find_variables(synth_encoding);

  while(program_index <=solver.number_of_options)
  {
    solver.generate_nth_assignment(program_index);
    std::cout << "program index " << program_index
        << " program size " << program_size<<std::endl;
    program_index++;

    solutiont test_solution = synth_encoding.get_solution(solver);
    if(solution_verifier(test_solution, counterexamples) ==
        decision_proceduret::resultt::D_SATISFIABLE)
    {
      output_program(test_solution, std::cout);
      std::cout<<"not a solution\n";
    }
    else
    {
      output_program(test_solution, std::cout);
      last_solution  = test_solution;
      std::cout<<"is a solution\n";
      return decision_proceduret::resultt::D_SATISFIABLE;
    }

  }
  return decision_proceduret::resultt::D_UNSATISFIABLE;
}


solutiont enumerative_program_generatort::get_nth_program(const std::size_t &n)
{
  solver.generate_nth_assignment(n);
  return synth_encoding.get_solution(solver);
}

void enumerative_program_generatort::set_up(problemt &problem)
{
  for(const exprt &e : problem.side_conditions)
  {
     const exprt encoded = synth_encoding(e);
     solver.set_to_true(encoded);
  }

  for(const auto &e : problem.constraints)
  {
     const exprt encoded = synth_encoding(e);
     solver.set_to_true(encoded);
  }

  for(const auto &c : synth_encoding.constraints)
  {
     solver.set_to_true(c);
  }
  solver.find_variables(synth_encoding);
}


void enumerative_program_generatort::output_program(
    std::ostream &out)
{
  solver.use_assignment(assignment_indices);
  solutiont solution = synth_encoding.get_solution(solver);

  out << "<program.";
  for(const auto &i : assignment_indices)
    out<<i<<".";
  out<<">";

  for(const auto &f : solution.functions)
  {
    out.setstate(std::ios_base::badbit);
    smt2_convt smt(ns, "", "", "", smt2_convt::solvert::Z3, out);
    out.clear();

    smt.convert_expr(f.second);
  }
  out << "</program.";
  for(const auto &i : assignment_indices)
    out<<i<<".";
  out << ">\n";
}

void enumerative_program_generatort::output_program(
    std::ostream &out, const std::size_t &index)
{
  if(index >= solver.number_of_options)
  {
    out << "Index " << index
        << " is greater than number of possible programs, which is "
        << solver.number_of_options << "\n";
    return;
  }

  solutiont solution=get_nth_program(index);
 // solver.print_assignment(out);
  out << "<program" << index << "> ";

  for(const auto &f : solution.functions)
  {
    out.setstate(std::ios_base::badbit);
    smt2_convt smt(ns, "", "", "", smt2_convt::solvert::Z3, out);
    out.clear();

    smt.convert_expr(f.second);
  }
  out << "</program " << index << ">\n";
}
