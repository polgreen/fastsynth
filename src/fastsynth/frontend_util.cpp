/*******************************************************************\

 Module: Fastsynth Language Frontend Utility

 Author: Matthias Weiss, matthias.weiss@diffblue.com

\*******************************************************************/

#include "frontend_util.h"
#include "cegis.h"

#include <goto-programs/goto_model.h>
#include <goto-programs/remove_complex.h>
#include <goto-programs/remove_returns.h>
#include <goto-programs/remove_vector.h>
#include <goto-programs/rewrite_union.h>

#include <goto-symex/symex_target_equation.h>

#include <langapi/language_util.h>

#include <util/cmdline.h>
#include <util/cout_message.h>
#include <util/mathematical_expr.h>
#include <util/mathematical_types.h>
#include <util/prefix.h>

#include <iostream>

/// Prefix that precedes each synthesised expression.
#define EXPRESSION_PREFIX "EXPRESSION"
/// Default logic for the CEGIS algorithm (SMT2 only).
#define DEFAULT_CEGIS_LOGIC "BV"
/// Default maximum size for the program to be synthesised.
#define DEFAULT_MAX_PROGRAM_SIZE 5u
/// Max verbosity level (Numbers above that value are cut).
#define MAX_VERBOSITY 10u

std::set<irep_idt> find_expressions(const symbol_tablet &symbol_table)
{
  std::set<irep_idt> result;

  for(const std::pair<const irep_idt, symbolt> &symbol : symbol_table.symbols)
  {
    if(
      !symbol.second.is_state_var && symbol.second.type.id() == ID_code &&
      symbol.second.value.is_nil() &&
      has_prefix(id2string(symbol.first), EXPRESSION_PREFIX))
      result.insert(symbol.first);
  }

  return result;
}

void set_verbosity(const cmdlinet &cmdline, console_message_handlert &mh)
{
  // Default verbosity.
  unsigned int verbosity = messaget::M_STATISTICS;

  if(cmdline.isset("verbosity"))
  {
    verbosity = std::stol(cmdline.get_value("verbosity"));
    if(verbosity > MAX_VERBOSITY)
      verbosity = MAX_VERBOSITY;
  }
  mh.set_verbosity(verbosity);
}

void show_formula(
  const symex_target_equationt &src,
  const namespacet &ns,
  const irep_idt mode)
{
  for(const SSA_stept &step : src.SSA_steps)
  {
    if(step.is_assignment() || step.is_assume())
      std::cout << "P: " << from_expr(ns, mode, step.cond_expr) << '\n';
    else if(step.is_assert())
      std::cout << "A: " << from_expr(ns, mode, step.cond_expr) << '\n';
  }
}

void instrument_expressions(
  const std::set<irep_idt> &expressions,
  goto_modelt &goto_model)
{
  for(std::pair<const irep_idt, goto_functionst::goto_functiont> &function :
      goto_model.goto_functions.function_map)
    for(goto_programt::instructiont &instruction :
        function.second.body.instructions)
      if(instruction.is_function_call())
      {
        const code_function_callt &call =
          to_code_function_call(instruction.code);
        if(call.function().id() == ID_symbol)
        {
          const irep_idt identifier{
            to_symbol_expr(call.function()).get_identifier()};
          if(
            expressions.find(identifier) != expressions.end() &&
            call.lhs().is_not_nil())
          {
            const code_typet &code_type = to_code_type(call.function().type());
            const typet &codomain = code_type.return_type();
            const code_typet::parameterst &params = code_type.parameters();
            mathematical_function_typet::domaint domain(params.size());

            transform(
              begin(params),
              end(params),
              begin(domain),
              [](const code_typet::parametert &param) { return param.type(); });

            const mathematical_function_typet type(domain, codomain);

            instruction.type = ASSIGN;

            function_application_exprt e(
              symbol_exprt(identifier, type),
              call.arguments(),
              call.lhs().type());

            instruction.code = code_assignt(call.lhs(), e);
          }
        }
      }
}

void process_goto_model(goto_modelt &goto_model)
{
  remove_returns(goto_model);
  remove_vector(goto_model);
  remove_complex(goto_model);
  rewrite_union(goto_model);
}

void set_cegis_cmdline_properties(const cmdlinet &cmdline, cegist &cegis)
{
  if(cmdline.isset("max-program-size"))
    cegis.max_program_size = std::stol(cmdline.get_value("max-program-size"));
  else
    cegis.max_program_size = DEFAULT_MAX_PROGRAM_SIZE;

  cegis.incremental_solving = cmdline.isset("incremental");
  cegis.use_simp_solver = cmdline.isset("simplifying-solver");
  cegis.use_fm = cmdline.isset("fm");
  cegis.enable_bitwise = !cmdline.isset("no-bitwise");
  cegis.use_smt = cmdline.isset("smt");
  cegis.enable_division = cmdline.isset("enable-division");
  cegis.logic = DEFAULT_CEGIS_LOGIC;
}

void print_expressions(messaget &message, const std::set<irep_idt> &expressions)
{
  for(const irep_idt &expression : expressions)
    message.status() << "EXPRESSION: " << expression << messaget::eom;
}

void print_synthesis_result(
  messaget &message,
  const cegist &cegis,
  const namespacet &ns,
  const irep_idt mode)
{
  for(const std::pair<const symbol_exprt, exprt> &function :
      cegis.solution.functions)
  {
    message.result() << "Result: " << function.first.get_identifier() << " -> "
                     << from_expr(ns, mode, function.second) << '\n';
  }
}

void print_synthesis_time(
  messaget &message,
  const std::chrono::duration<double> &synthesis_time)
{
  message.statistics() << "Synthesis time: " << synthesis_time.count() << 's'
                       << messaget::eom;
}
