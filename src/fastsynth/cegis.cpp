#include "cegis.h"
#include "incremental_solver_learn.h"
#include "local_cegis.h"
#include "solver_learn.h"
#include "verify.h"
#include "fm_verify.h"
#include <chrono>

#include <langapi/language_util.h>

#include <util/simplify_expr.h>

#include <memory>

decision_proceduret::resultt cegist::operator()(
  const problemt &problem)
{
  std::unique_ptr<learnt> learner;
  std::unique_ptr<verifyt> verifier;

  if((incremental_solving || use_simp_solver) && use_smt)
  {
    warning() << "WARNING: unable to use smt back end and incremental solving together\n"
              << "Using smt only" << eom;
    incremental_solving=false;
    use_simp_solver=false;
  }
  if(logic=="LIA")
  {
    warning() << "WARNING: Linear Integer Arithmetic requires SMT backend. Using SMT back end" << eom;
    use_smt=true;
    use_simp_solver=false;
    incremental_solving=false;
  }

  if(incremental_solving)
  {
    status() << "** incremental CEGIS" << eom;
    learner=std::unique_ptr<learnt>(new incremental_solver_learnt(
      ns, problem, use_simp_solver, get_message_handler()));
  }
  else
  {
    status() << "** non-incremental CEGIS" << eom;
    solver_learnt *l=new solver_learnt(
      ns, problem, get_message_handler());

    l->use_smt=use_smt;
    l->logic=logic;

    learner=std::unique_ptr<learnt>(l);
  }

  learner->enable_bitwise=enable_bitwise;

  if(use_fm)
  {
    verifier=std::unique_ptr<verifyt>(new fm_verifyt(
      ns, problem, get_message_handler()));
  }
  else
  {
    verifier=std::unique_ptr<verifyt>(new verifyt(
      ns, problem, get_message_handler()));
  }

  verifier->use_smt=use_smt;
  verifier->logic=logic;

  return loop(problem, *learner, *verifier);
}

decision_proceduret::resultt cegist::loop(
  const problemt &problem,
  learnt &learn,
  verifyt &verify)
{
  local_cegist local_cegis(ns, verify, problem);
  local_cegis.set_message_handler(get_message_handler());
  local_cegis.incremental_solving = incremental_solving;
  local_cegis.use_simp_solver = use_simp_solver;
  local_cegis.use_smt = use_smt;
  local_cegis.logic = logic;

  unsigned iteration=0;


  std::size_t program_size=min_program_size;

  // now enter the CEGIS loop
  while(true)
  {
    if(local_cegis.has_solution())
    {
      status() << "** CEGIS local search successful " << iteration << eom;
      solution = local_cegis.solution;
      return decision_proceduret::resultt::D_SATISFIABLE;
    }

    iteration++;
    status() << blue << "** CEGIS iteration " << iteration << reset << eom;
    auto iter_start_time=std::chrono::steady_clock::now();

    if(max_iterations && iteration > max_iterations)
      return decision_proceduret::resultt::D_ERROR;

    status() << "** Synthesis phase" << eom;

    learn.set_program_size(program_size);

    switch(learn())
    {
    case decision_proceduret::resultt::D_SATISFIABLE: // got candidate
      {
        std::map<symbol_exprt, exprt> old_functions;
        old_functions.swap(solution.functions);

        solution=learn.get_solution();

        for(auto &f : solution.functions)
          f.second=simplify_expr(f.second, ns);

        if(old_functions==solution.functions)
        {
          error() << bold << "NO PROGRESS MADE" << reset << eom;
          return decision_proceduret::resultt::D_ERROR;
        }
      }
      // proceed to verification phase
      break;

    case decision_proceduret::resultt::D_UNSATISFIABLE: // no candidate
      if(program_size<max_program_size)
      {
        program_size+=1;
        status() << "Failed to get candidate; "
                    "increasing program size to " << program_size << eom;
        continue; // do another attempt to synthesize
      }

      error() << bold << "FAILED TO GET CANDIDATE" << reset << eom;
      return decision_proceduret::resultt::D_UNSATISFIABLE;

    case decision_proceduret::resultt::D_ERROR:
      return decision_proceduret::resultt::D_ERROR;
    }
    status() << "Synthesis time iteration " << iteration <<": "
             << std::chrono::duration<double>(
                  std::chrono::steady_clock::now() - iter_start_time)
                  .count()
             << 's' << eom;
    status() << "** Verification phase" << eom;

    switch(verify(solution))
    {
    case decision_proceduret::resultt::D_SATISFIABLE: // counterexample
      status() << "** Verification failed" << eom;
      status() << "Total time iteration " << iteration <<": "
               << std::chrono::duration<double>(
                    std::chrono::steady_clock::now() - iter_start_time)
                    .count()
               << 's' << eom;

      learn.add_ce(verify.get_counterexample());
      if(use_local_search)
      {
        local_cegis.push_back(solution, program_size);
        local_cegis();
      }
      break;

    case decision_proceduret::resultt::D_UNSATISFIABLE: // done, got solution
      status() << "Result obtained with " << iteration << " iteration(s)"
               << eom;
      status() << "Total time iteration " << iteration <<": "
               << std::chrono::duration<double>(
                    std::chrono::steady_clock::now() - iter_start_time)
                    .count()
               << 's' << eom;
      result() << bold << "VERIFICATION SUCCESSFUL" << reset << eom;
      return decision_proceduret::resultt::D_SATISFIABLE;

    case decision_proceduret::resultt::D_ERROR:
      return decision_proceduret::resultt::D_ERROR;
    }
  }
}

void output_expressions(
  const std::map<symbol_exprt, exprt> &expressions,
  const namespacet &ns,
  std::ostream &out)
{
  for(const auto &e : expressions)
  {
    out << e.first.get_identifier()
        << " -> "
        << from_expr(ns, "", e.second)
        << '\n';
  }
}
