/*
 * simplex linear optimization algorithm
 * (this is classical simplex algorithm
 *  afaik there are also faster methods)
 *
 * 
 * TODO: 
 * 
 * - detect of circling
 *   (long periods of time with varaibles and solution doesn't improve,
 *   it is then likely to have multiple optimum solutions).
 *   If this happens then it could be good idea to use 'watchdog'
 *   thread to oversee optimization task and if activity seems
 *   like circling then watchdog thread can mathmetically check
 *   if it is true (=slow)
 * 
 * - detection, solving and reporting of alternative optimimum
 *   solutions (page 101 in operations research book)
 * 
 * - handling of infeasible problems (returns false)
 *
 * - optimizations
 *
 */

#ifndef simplex_h
#define simplex_h

#include <pthread.h>
#include <vector>



namespace whiteice
{
  namespace math
  {
    template <typename T>
      class simplex
      {
      public:
	// input: number of variables and constraints
	// in the optimization problem
	simplex(unsigned int variables, unsigned int constraints);
	~simplex();
	
	// format: [c_1 c_2 ... c_n], where F = SUM( c_i*x_i )
	bool setTarget(const std::vector<T>& target) throw();
	
	// format: [c_1 c2_ ... c_n, c], where 
	// eqtype = 0: SUM( c_i*x_i ) <= c
	// eqtype = 1: SUM( c_i*x_i ) =  c
	// eqtype = 2: SUM( c_i*x_i ) >= c
	bool setConstraint(unsigned int index,
			   const std::vector<T>& constraint,
			   unsigned int eqtype = 0) throw();
	
	// format - look at set setX()s documentation
	bool getTarget(std::vector<T>& target) const throw();
	bool getConstraint(unsigned int index, std::vector<T>& constraint) const throw();
	
	unsigned int getNumberOfConstraints() const throw();
	unsigned int getNumberOfVariables() const throw();
	
	
	// maximizes problem, returns true if maximum is found
	// else returns false (maximum not found)
	bool maximize() throw();
	
	bool hasResult() throw();
	
	// format [x_1, x_2,... x_n, F], where F = SUM(c_i*x_i) (optimum value)
	bool getSolution(std::vector<T>& solution) const;
	
	// show simplex table, for debugging/testing etc.
	bool show_simplex() const throw();
	
      private:
	
	// finds pivot element and row
	bool find_indexes(T*& target, std::vector<T*>& constraints,
			  unsigned int& eindex, unsigned int& lindex) const throw();
	
	bool find_indexes2(T*& target, std::vector<T*>& constraints,
			   unsigned int& eindex, unsigned int& lindex,
			   const std::vector<unsigned int>& bsols,
			   const unsigned int& pseudoStart) const throw();
	
	bool show_simplex(T* target) const throw();
	
	unsigned int numVariables;
	unsigned int numArtificials; // number of artificial variables
	
	// basic and non-basic variables in current solution
	std::vector<unsigned int> basic; // rows
	std::vector<unsigned int> nonbasic; // columns;
	
	// simplex table
	T* target;
	
	std::vector<T*> constraints;
	std::vector<unsigned int> ctypes; // constraint types
	
	// maximization thread iteration counter
	unsigned int iter;
	bool running, has_result;
	
	
	mutable pthread_mutex_t simplex_lock;
	pthread_t maximization_thread;
	
      public:
	// maximization process loop
	// DO NOT CALL
	
	void threadloop();
	
      };
    
    
    extern template class simplex<float>;
    extern template class simplex<double>;
    
  }
}
  

#endif

