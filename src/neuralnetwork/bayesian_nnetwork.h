/*
 * MCMC bayesian neural network (using samples from p(w|data))
 *
 * supports use of samples of weights p(w) which
 * will be used to store and load network state and 
 * calculate responses.
 */

#ifndef bayesian_nnetwork_h
#define bayesian_nnetwork_h

#include "nnetwork.h"

namespace whiteice
{
  template < typename T = math::blas_real<float> >
    class bayesian_nnetwork
    {
    public:

    bayesian_nnetwork();
    virtual ~bayesian_nnetwork();

    /*
     * imports and exports samples of p(w) to and from nnetwork
     */
    unsigned int getNumberOfSamples() const throw(); // number of samples in BNN

    bool importSamples(const std::vector<unsigned int>& arch,
		       const std::vector< math::vertex<T> >& weights);
    bool importNetwork(const nnetwork<T>& net);

    bool exportSamples(std::vector<unsigned int>& arch,
		       std::vector< math::vertex<T> >& weights,
			   int latestN = 0);
    
    /*
     * downsamples number of neural networks down to N neural networks
     * or if N > number of neural networks/samples does nothing
     */
    bool downsample(unsigned int N);
    

    // calculates E[f(input,w)] = E[y|x] and Var[f(x,w)] = Var[y|x] for given input
    bool calculate(const math::vertex<T>& input,
		   math::vertex<T>& mean,
		   math::matrix<T>& covariance,
		   int latestN = 0);

    unsigned int outputSize() const throw();
    unsigned int inputSize() const throw();

    // stores and loads bayesian nnetwork to a dataset file
    // (saves all samples into files)
    bool load(const std::string& filename) throw();
    bool save(const std::string& filename) const throw();

    private:

    std::vector< nnetwork<T>* > nnets;
      
      
    };

  extern template class bayesian_nnetwork< float >;
  extern template class bayesian_nnetwork< double >;  
  extern template class bayesian_nnetwork< math::blas_real<float> >;
  extern template class bayesian_nnetwork< math::blas_real<double> >;
  
};



#endif


