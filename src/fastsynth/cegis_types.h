#ifndef CPROVER_CEGIS_TYPES_H_
#define CPROVER_CEGIS_TYPES_H_

#include <set>
#include <map>

#include <util/std_expr.h>
#include <util/std_types.h>


class problemt
{
public:
  std::set<exprt> free_variables;
  exprt::operandst side_conditions, constraints, output_generator_constraints;
  std::set<exprt> literals; /// Constant hints for solver.


  // needed by neural network interface, otherwise not used
  std::map<irep_idt, mathematical_function_typet> synth_fun_set;
};
  
class solutiont
{
public:
  using functionst=std::map<symbol_exprt, exprt>;
  functionst functions, s_functions;
};

class counterexamplet
{
public:
  std::map<exprt, exprt> assignment;

  // function applications
  using f_appst=std::map<function_application_exprt, exprt>;
  f_appst f_apps;

  void clear() { assignment.clear(); f_apps.clear(); }
};

#endif /* CPROVER_FASTSYNTH_SOLUTION_H_ */
