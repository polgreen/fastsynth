#include "verify.h"
#include "solver.h"

#include <langapi/language_util.h>

void verifyt::output(
  const solutiont::functionst &functions,
  std::ostream &out)
{
  for(const auto &f : functions)
  {
    out << f.first.get_identifier()
        << " -> "
        << from_expr(ns, "", f.second)
        << '\n';
  }
}

decision_proceduret::resultt verifyt::operator()(
    solutiont &solution, std::vector<counterexamplet> &counterexamples,
    bool force_new_cex)
{
  if(!force_new_cex)
    return this->operator ()(solution);

  output(solution.functions, debug());
  debug() << eom;

  solvert solver_container(use_smt, logic, ns, get_message_handler());
  auto &solver = solver_container.get();

  decision_proceduret::resultt result;

  verify_encodingt verify_encoding;
  verify_encoding.functions = solution.functions;
  output(verify_encoding.functions, debug());
  verify_encoding.free_variables = problem.free_variables;

  add_problem(verify_encoding, solver);

  for(const auto &c : counterexamples)
  {
    for(const auto &it : c.assignment)
    {
      const exprt &symbol = it.first;
      const exprt &value = it.second;

      exprt encoded = verify_encoding(notequal_exprt(symbol, value));
      debug() << "ce: " << from_expr(ns, "", encoded) << eom;
      solver.set_to_true(encoded);
    }
  }

  result = solver();

  if(result == decision_proceduret::resultt::D_SATISFIABLE)
    counterexample = verify_encoding.get_counterexample(solver);
  else
  {
    // no more unique counterexamples
    if(this->operator ()(solution)
        == decision_proceduret::resultt::D_SATISFIABLE)
      return decision_proceduret::resultt::D_ERROR;
    else
    {
      // no counterexamples, genuinely UNSAT
      counterexample.clear();
    }
  }

  return result;
}


std::size_t verifyt::operator()(
    solutiont &solution, std::vector<counterexamplet> &counterexamples)
{
  output(solution.functions, debug());
  debug() << eom;
  std::size_t result=0;

  for(const auto &c : counterexamples)
  {
    // for each counterexample, creates a solver and checks whether there is
    // an assignment that triggers the assertion for that cex.
    // TODO: do this with solving under assumptions to avoid constructing
    // the solver multiple times
    solvert solver_container(use_smt, logic, ns, get_message_handler());
    auto &solver = solver_container.get();

    verify_encodingt verify_encoding;
    verify_encoding.functions = solution.functions;
    output(verify_encoding.functions, debug());
    verify_encoding.free_variables = problem.free_variables;

    add_problem(verify_encoding, solver);

    for(const auto &it : c.assignment)
    {
      const exprt &symbol = it.first;
      const exprt &value = it.second;

      exprt encoded = verify_encoding(equal_exprt(symbol, value));
      debug() << "ce: " << from_expr(ns, "", encoded) << eom;
      solver.set_to_true(encoded);
    }
    if(solver() == decision_proceduret::resultt::D_UNSATISFIABLE)
      result++;
  }
  return result;
}


decision_proceduret::resultt verifyt::operator()(
  solutiont &solution)
{
  output(solution.functions, debug());
  debug() << eom;

  // check that the parameters in the given solution
  // are consistent with the function signature
  verify_encodingt::check_function_bodies(solution.functions);

  solvert solver_container(use_smt, logic, ns, get_message_handler());
  auto &solver=solver_container.get();

  decision_proceduret::resultt result;

  verify_encodingt verify_encoding;
  verify_encoding.functions=solution.functions;
  output(verify_encoding.functions, debug());
  verify_encoding.free_variables=problem.free_variables;

  add_problem(verify_encoding, solver);
  result=solver();

  if(result==decision_proceduret::resultt::D_SATISFIABLE)
    counterexample=
      verify_encoding.get_counterexample(solver);
  else
    counterexample.clear();

  return result;
}

void verifyt::add_problem(
  verify_encodingt &verify_encoding,
  decision_proceduret &solver)
{
  debug() << "verify: add problem" << eom;
  for(const auto &e : problem.side_conditions)
  {
    const exprt encoded=verify_encoding(e);
    debug() << "sc: " << from_expr(ns, "", encoded) << eom;
    solver.set_to_true(encoded);
  }

  const exprt encoded=verify_encoding(conjunction(problem.constraints));
  debug() << "co: !(" << from_expr(ns, "", encoded) << ')' << eom;
  solver.set_to_false(encoded);
}

