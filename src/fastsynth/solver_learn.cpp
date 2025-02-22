#include <solvers/flattening/bv_pointers.h>
#include <solvers/sat/satcheck.h>

#include <solvers/smt2/smt2_dec.h>

#include <langapi/language_util.h>

#include "synth_encoding.h"
#include "solver_learn.h"

solver_learn_baset::solver_learn_baset(
  const namespacet &_ns,
  const problemt &_problem,
  message_handlert &_message_handler,
  synth_encoding_factoryt synth_encoding_factory):
  learnt(_message_handler),
  ns(_ns),
  problem(_problem),
  synth_encoding_factory(std::move(synth_encoding_factory))
{
}

void solver_learn_baset::add_counterexample(
  const counterexamplet &ce,
  synth_encodingt &synth_encoding,
  decision_proceduret &solver)
{
  for(const auto &it : ce.assignment)
  {
    const exprt &symbol = it.first;
    const exprt &value = it.second;

    exprt encoded = synth_encoding(equal_exprt(symbol, value));
    debug() << "ce: " << from_expr(ns, "", encoded) << eom;
    solver.set_to_true(encoded);
  }
}

void solver_learn_baset::add_problem(
  synth_encodingt &encoding,
  decision_proceduret &solver)
{
  for(const exprt &e : problem.side_conditions)
  {
    const exprt encoded = encoding(e);
    debug() << "sc: " << from_expr(ns, "", encoded) << eom;
    solver.set_to_true(encoded);
  }

  for(const auto &e : problem.constraints)
  {
    const exprt encoded = encoding(e);
    debug() << "co: " << from_expr(ns, "", encoded) << eom;
    solver.set_to_true(encoded);
  }

  for(const auto &c : encoding.constraints)
  {
    solver.set_to_true(c);
    // These constraints can get huge and due to recent changes to `expr2c`
    // converting them to C is expensive and may even cause
    if(message_handler && get_message_handler().get_verbosity() >= M_DEBUG)
      debug() << "ec: " << from_expr(ns, "", c) << eom;
  }
}

solver_learnt::solver_learnt(
  const namespacet &_ns,
  const problemt &_problem,
  message_handlert &_message_handler,
  synth_encoding_factoryt synth_encoding_factory):
  solver_learn_baset(
    _ns, _problem, _message_handler, std::move(synth_encoding_factory)),
  program_size(1u)
{
}

void solver_learnt::set_program_size(const size_t program_size)
{
  this->program_size = program_size;
}

decision_proceduret::resultt solver_learnt::operator()()
{
  if(use_smt)
  {
    smt2_dect solver(
      ns, "fastsynth", "generated by fastsynth",
      logic, smt2_dect::solvert::Z3);
    solver.set_message_handler(get_message_handler());

    return this->operator()(solver);
  }
  else
  {
    satcheckt satcheck(get_message_handler());

    bv_pointerst solver(ns, satcheck, get_message_handler());

    return this->operator()(solver);
  }
}

decision_proceduret::resultt solver_learnt::operator()(
  decision_proceduret &solver)
{
  const std::unique_ptr<synth_encodingt> synth_enc(synth_encoding_factory());
  synth_enc->program_size = program_size;
  synth_enc->enable_bitwise = enable_bitwise;
  synth_enc->literals = problem.literals;

  if(counterexamples.empty())
  {
    synth_enc->suffix = "$ce";
    synth_enc->constraints.clear();

    add_problem(*synth_enc, solver);
  }
  else
  {
    std::size_t counter = 0;
    for(const auto &c : counterexamples)
    {
      synth_enc->suffix = "$ce" + std::to_string(counter);
      synth_enc->constraints.clear();
      add_counterexample(c, *synth_enc, solver);

      add_problem(*synth_enc, solver);

      counter++;
    }
  }

  const decision_proceduret::resultt result=solver();

  switch(result)
  {
  case decision_proceduret::resultt::D_SATISFIABLE:
    #if 1
    solver.print_assignment(debug());
    debug() << eom;
    #endif

    last_solution = synth_enc->get_solution(solver);
    break;

  case decision_proceduret::resultt::D_UNSATISFIABLE:
  case decision_proceduret::resultt::D_ERROR:
    break;
  }

  return result;
}

solutiont solver_learnt::get_solution() const
{
  return last_solution;
}

void solver_learnt::add_ce(
  const counterexamplet &counterexample)
{
  counterexamples.emplace_back(counterexample);
}
