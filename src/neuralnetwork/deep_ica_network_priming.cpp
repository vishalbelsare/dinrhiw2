/*
 * TESTING:
 * used to test idea where we use aprox. 
 * "non-linear" ICA calculations to initialize nnetwork 
 * weights with coefficients that calculated non-linear
 * ICA approximatedly (N-N-N-X-OUTPUT) and then use those
 * coefficients to find high-quality nnetwork paramters
 * 
 * (here we simply "drop" the non-linearities away when
 *  we do the priming as the non-linearities are nearly
 *  linear close to zeros where most of the data is. We
 *  then hope that gradient descent and optimize the solution
 *  to work properly with values that are not close to zero)
 */

#include "deep_ica_network_priming.h"
#include "eig.h"
#include "correlation.h"
#include "linear_algebra.h"
#include "ica.h"

#include <math.h>
#include <set>


namespace whiteice
{
  /**
   * calculates 2*deepness layer "neural network" weights and
   * stores them to parameters. calculations are done recursively
   * by first calculating PCA and one-layer non-linear ICA and
   * then calling this function again with deepness-1.
   *
   * NOTE: currently using tanh(x) non-linearity, sinh(x) would
   *       be BETTER but DO NOT work for nnetwork initialization
   *       which uses tanh(x) non-linearities [sinh(x) 
   *       non-linearity gives good independent components 
   *       so creating neural network that uses sinh(x) and
   *       asinh(x) non-linearities instead of tanh(x) or 
   *       sigmoidal one's would be interesting]
   */
  bool deep_nonlin_ica(std::vector< math::vertex<> >& data,
		       std::vector<deep_ica_parameters>&
		       parameters, unsigned int deepness)
  {
    if(data.size() <= 0)
      return false;
    
    if(deepness <= 0){ // time to stop
      return (parameters.size() > 0);
    }

    struct deep_ica_parameters p;
    
    // initially we calculate PCA for this layer's data
    {
      // 1. removes mean value
      // 2. calculates covariance matrix Rxx and whitens the data
      
      math::vertex<> mean;
      math::matrix<> Rxx;
      math::matrix<> V;

      if(math::mean_covariance_estimate<>(mean, Rxx, data) == false)
	return false;

      if(math::symmetric_eig<>(Rxx, V) == false)
	return false;

      math::matrix<>& D = Rxx; // Rxx contains now diagnonal matrix

      for(unsigned int i=0;i<Rxx.ysize();i++){
	if(D(i,i) < 0.0)
	  D(i,i) = math::abs(D(i,i));
	if(D(i,i) <= 10e-8){
	  D(i,i) = 0.0; // dirty way to handle singular matrixes (just force the data to zero)
	}
	else{
	  // sets diagonal variances to 0.5
	  math::blas_real<float> d = D(i,i);
	  math::blas_real<float> s = 0.5f;
	  
	  D(i,i) =s/sqrt(d);
	}
      }

      p.W_pca = V * D * V.transpose();
      p.b_pca = - p.W_pca*mean;

      // actual processes the data (PCA)
      for(unsigned int i=0;i<data.size();i++){
	data[i] = p.W_pca * data[i] + p.b_pca;
      }

      if(math::mean_covariance_estimate<>(mean, Rxx, data) == false)
	return false;

      // std::cout << "mean = " << mean << std::endl;
      // std::cout << "Rxx = " << Rxx << std::endl;
    }

    
    /*
     * next we do diagnalize E[sinh(x)sinh(x)^t] matrix 
     * where aim is to remove higher order correlations 
     * from the data
     */
    {
      std::vector< math::vertex<> > sinh_data;
      math::vertex<> sinh_mean;

      for(unsigned int i=0;i<data.size();i++){
	math::vertex<> d1 = data[i];

	for(unsigned int ii=0;ii<d1.size();ii++){
	  // d1[ii] = sinhf(d1[ii].c[0]);
	  d1[ii] = tanhf(d1[ii].c[0]);
	}

	sinh_data.push_back(d1);
      }

      math::vertex<> mean;
      math::matrix<> Rxx;
      math::matrix<> V;

      if(math::mean_covariance_estimate<>(mean, Rxx, sinh_data) == false)
	return false;

      if(math::symmetric_eig<>(Rxx, V) == false)
	return false;
      
      p.W_ica = V.transpose();

      Rxx.identity();
      Rxx = Rxx - p.W_ica;
      
      p.b_ica = Rxx*mean;

      // now we transform data through g^-1(W*g(x)) transform
      // g_new(x) = g(x) - E[g(x)] in order to keep E[g(x)] = 0
      
      for(unsigned int i=0;i<sinh_data.size();i++){
	math::vertex<> x = sinh_data[i];
	x = p.W_ica * x + p.b_ica;

	for(unsigned int ii=0;ii<x.size();ii++)
	  // x[ii] = asinhf(x[ii].c[0]);
	  
	  if(x[ii].c[0] >= 0.9999f)
	    x[ii] = atanhf(0.9999f);
	  else if(x[ii].c[0] <= -0.9999f)
	    x[ii] = atanhf(-0.9999f);
	  else
	    x[ii] = atanhf(x[ii].c[0]);
	  

	// overwrites the data vector with preprocessed data
	data[i] = x; 
      }
    }

    
    // this layer processing is done so we move to the next layer
    {
      parameters.push_back(p);

      return deep_nonlin_ica(data, parameters, deepness - 1);
    }
  }



  /**
   * calculates 2*deepness layer "neural network" weights and
   * stores them to parameters. calculations are done recursively
   * by first calculating PCA and one-layer non-linear ICA and
   * then calling this function again with deepness-1.
   *
   * NOTE: this one uses sinh(x) non-linearity that works
   *       with sinh_nnetwork()
   */
  bool deep_nonlin_ica_sinh(std::vector< math::vertex<> >& data,
			    std::vector<deep_ica_parameters>&
			    parameters, unsigned int deepness)
  {
    if(data.size() <= 0)
      return false;
    
    if(deepness <= 0){ // time to stop
      return (parameters.size() > 0);
    }

    struct deep_ica_parameters p;
    
    // initially we calculate PCA for this layer's data
    {
      // 1. removes mean value
      // 2. calculates covariance matrix Rxx and whitens the data
      
      math::vertex<> mean;
      math::matrix<> Rxx;
      math::matrix<> V;

      if(math::mean_covariance_estimate<>(mean, Rxx, data) == false)
	return false;

      if(math::symmetric_eig<>(Rxx, V) == false)
	return false;

      math::matrix<>& D = Rxx; // Rxx contains now diagnonal matrix

      for(unsigned int i=0;i<Rxx.ysize();i++){
	if(D(i,i) < 0.0)
	  D(i,i) = math::abs(D(i,i));
	if(D(i,i) <= 10e-8){
	  D(i,i) = 0.0; // dirty way to handle singular matrixes (just force the data to zero)
	}
	else{
	  // sets diagonal variances to 0.5
	  math::blas_real<float> d = D(i,i);
	  math::blas_real<float> s = 0.5f;
	  
	  D(i,i) =s/sqrt(d);
	}
      }

      p.W_pca = V * D * V.transpose();
      p.b_pca = - p.W_pca*mean;

      // actual processes the data (PCA)
      for(unsigned int i=0;i<data.size();i++){
	data[i] = p.W_pca * data[i] + p.b_pca;
      }

      if(math::mean_covariance_estimate<>(mean, Rxx, data) == false)
	return false;

      // std::cout << "mean = " << mean << std::endl;
      // std::cout << "Rxx = " << Rxx << std::endl;
    }

    
    /*
     * next we do diagnalize E[sinh(x)sinh(x)^t] matrix 
     * where aim is to remove higher order correlations 
     * from the data
     */
    {
      std::vector< math::vertex<> > sinh_data;
      math::vertex<> sinh_mean;

      for(unsigned int i=0;i<data.size();i++){
	math::vertex<> d1 = data[i];

	for(unsigned int ii=0;ii<d1.size();ii++){
	  d1[ii] = sinhf(d1[ii].c[0]);
	  // d1[ii] = tanhf(d1[ii].c[0]);
	}

	sinh_data.push_back(d1);
      }

      math::vertex<> mean;
      math::matrix<> Rxx;
      math::matrix<> V;

      if(math::mean_covariance_estimate<>(mean, Rxx, sinh_data) == false)
	return false;

      if(math::symmetric_eig<>(Rxx, V) == false)
	return false;
      
      p.W_ica = V.transpose();

      Rxx.identity();
      Rxx = Rxx - p.W_ica;
      
      p.b_ica = Rxx*mean;

      // now we transform data through g^-1(W*g(x)) transform
      // g_new(x) = g(x) - E[g(x)] in order to keep E[g(x)] = 0
      
      for(unsigned int i=0;i<sinh_data.size();i++){
	math::vertex<> x = sinh_data[i];
	x = p.W_ica * x + p.b_ica;

	for(unsigned int ii=0;ii<x.size();ii++)
	  x[ii] = asinhf(x[ii].c[0]);
	  
	/*
	  if(x[ii].c[0] >= 0.9999f)
	    x[ii] = atanhf(0.9999f);
	  else if(x[ii].c[0] <= -0.9999f)
	    x[ii] = atanhf(-0.9999f);
	  else
	    x[ii] = atanhf(x[ii].c[0]);
	*/

	// overwrites the data vector with preprocessed data
	data[i] = x; 
      }
    }

    
    // this layer processing is done so we move to the next layer
    {
      parameters.push_back(p);

      return deep_nonlin_ica(data, parameters, deepness - 1);
    }
  }
  

  /**
   * constructs nnetwork from deep_nonlin_ica solved parameters  
   * by adding rest_of_arch randomly initiaized extra layers
   * after the deep ica layers. The idea is that we construct a network
   * that extract high-quality (non-linear) ICA solution features and
   * then use gradient descent to get better solutions.
   *
   * This way we may construct 10-50 layer neural network that
   * then uses gradient descent to go to the nearest local minimum.
   *
   */
  bool initialize_nnetwork(const std::vector<deep_ica_parameters>& parameters,
			   nnetwork<>& nnet)
  {
    if(parameters.size() <= 0)
      return false;
    
    nnet.randomize();
    
    
    for(unsigned int i=0;i<(2*parameters.size() - 1);i+=2){
      const deep_ica_parameters& p = parameters[i/2];;

      if(nnet.setWeights(p.W_pca, i) == false) return false;
      if(nnet.setBias(p.b_pca, i) == false) return false;

      if(nnet.setWeights(p.W_ica, i+1) == false) return false;
      if(nnet.setBias(p.b_ica, i+1) == false) return false;
    }

    return true;
  }
  
  
  /**
   * function to cause negative feedback between neurons of 
   * each network layer (except the last one). This means that neurons
   * do differentiate to different inputs. This can be used as a training heuristic
   * during learning.
   */
  template <typename T>
  bool negative_feedback_between_neurons(nnetwork<T>& nnet, 
					 const dataset<T>& data,
					 const T& alpha, bool processLastLayer)
  {
    // use pca to set directions towards independenct components of 
    // the inputs of each layer, also sets data variance of network to 1.0

    unsigned int L = nnet.getLayers()-1;
    if(L > 0) L--; // we leave one regular layer there to be optimized normally
    
    for(unsigned int l=0;l<L;l++)
    {
      // goes through the data and collects samples per layer
      for(unsigned int i=0;i<data.size(0);i++){
	nnet.input() = data.access(0, i);
	nnet.calculate(false, true);
      }
	
      if(neuronlayerwise_ica(nnet, alpha, l) == false){
	std::cout << "Warning: calculating ICA for input data failed (layer: " 
		  << l << ")" << std::endl;
	return false;
      }
	
      nnet.clearSamples();
    }
    
#if 1
    // optimizes last layer using linear least squares MSE
    if(processLastLayer)
    {
      // goes through the data and collects samples per layer
      for(unsigned int i=0;i<data.size(0);i++){
	nnet.input() = data.access(0, i);
	nnet.calculate(false, true);
      }
      
      const unsigned int l = nnet.getLayers()-1;
      
      if(neuronlast_layer_mse(nnet, data, l) == false){
	std::cout << "Warning: calculating MSE optimization for the last layer failed (layer: " 
		  << l << ")" << std::endl;
	return false;
      }

      nnet.clearSamples();
    }
#endif
    
    return true;
    
    
#if 0
    std::vector<unsigned int> arch;
    nnet.getArchitecture(arch);
    
    unsigned int N = arch.size() - 1;
    
    if(processLastLayer == false)
      N--;
    
    for(unsigned int i=0;i<N;i++){
      math::matrix<T> W;
      math::vertex<T> w;
      
      nnet.getWeights(W, i);
      
      
      // orthonormalizes the weight vector basis
      {
	std::vector< math::vertex<T> > rows;
	rows.resize(W.ysize());
	
	for(unsigned int j=0;j<W.ysize();j++){
	  W.rowcopyto(w, j);
	  rows[j] = w;
	}
	
	gramschmidt(rows, 0, rows.size());

	for(unsigned int j=0;j<W.ysize();j++){
	  W.rowcopyto(w, j);
	  
	  T l = w.norm();
	  
	  w.normalize();
	  w = (T(1.0f) - alpha)*w + alpha*rows[j];
	  w.normalize();
	  
	  w *= l; // T(2.0f); no scaling
	  
	  W.rowcopyfrom(w,j);
	  
	}
      }
      
      nnet.setWeights(W,i);

      // do not do anything to bias terms
    }

    return true;
#endif
  }
  
  
  template <typename T>
  bool neuronlayerwise_ica(nnetwork<T>& nnet, const T& alpha, unsigned int layer)
  {
    std::vector<unsigned int> arch;
    nnet.getArchitecture(arch);
    
    const unsigned int& l = layer;
    
    {
      math::matrix<T> W;
      math::vertex<T> w, wxx;
      math::vertex<T> b;
      
      if(nnet.getWeights(W, l) == false)
	return false;
      
      if(nnet.getBias(b, l) == false)
	return false;
      
      std::vector< math::vertex<T> > samples;
      if(nnet.getSamples(samples, l) == false)
	return false;
      
      
      // calculates ICA of layer's input
      math::matrix<T> Wxx;
      math::vertex<T> m_wxx;
      math::vertex<T> sinh_x;
      math::vertex<T> m;
      
      {
	math::matrix<T> Cxx;
	
	if(mean_covariance_estimate(m, Cxx, samples) == false)
	  return false;
	
	math::matrix<T> V, D(Cxx);
	
	if(symmetric_eig(D, V) == false)
	  return false;
	
	math::matrix<T> invD = D;
	
	T scaling = T(0.0f);
	
	for(unsigned int i=0;i<invD.ysize();i++){
	  T d = whiteice::math::abs(invD(i,i));
	  scaling += d;
	  
	  if(d > T(10e-8)){
	    invD(i,i) = whiteice::math::sqrt(T(1.0)/whiteice::math::abs(d));
	  }
	  else{
	    invD(i,i) = T(0.0f);
	  }
	  
	}
	
	scaling /= T(invD.ysize());
	
	// std::cout << "scaling = " << scaling << std::endl;
	// scaling = T(2.0f);
	scaling = T(1.0f);
	
	Wxx = invD * V.transpose(); // for ICA use: ICA * invD * Vt;
	// Wxx = V.transpose(); // for ICA use: ICA * invD * Vt;
	
	sinh_x = m; 
	sinh_x.zero();
	
	for(unsigned int i=0;i<samples.size();i++){
	  for(unsigned int d=0;d<sinh_x.size();d++)
	    sinh_x[d] += nnet.inv_nonlin(samples[i][d], l);
	  
	  samples[i] -= m;
	  samples[i] = Wxx*samples[i]; // whitens data for the ICA step
	}
	
	sinh_x /= T((float)samples.size());
	
	Wxx = scaling*Wxx;
	
#if 0
	math::matrix<T> ICA;
	
	if(math::ica(samples, ICA) == false)
	  return false;
	
	// we assume E{g(y)} = E{x} (so we force E{g(x)} to be "linear"!)
	// should be: E{g(y)} = E{y}
	
	Wxx = scaling*(ICA * invD * V.transpose());
#endif	
	// m_wxx = - Wxx*m;
      }
     
      
      {
	// calculates additional rotation Z that optimizes Wxx towards W
	// min ||W - Z*Wxx||
	
	math::matrix<T> INV = Wxx;
	INV.inv();
	math::matrix<T> S = W*INV;
	math::matrix<T> U, V;
	
	// std::cout << "START SVD" << std::endl;
	// std::cout << "SVD SIZE: " << S.xsize() << " x " << S.ysize() << std::endl;
	if(svd(S, U, V) == false)
	  return false;
	// std::cout << "END SVD" << std::endl;
	
	S = U*V.transpose();

	Wxx = S*Wxx;
	
	m_wxx = -(Wxx*m);
	
	for(unsigned int j=0;j<W.ysize() && j<Wxx.ysize();j++){
	  Wxx.rowcopyto(wxx, j);
	  W.rowcopyfrom(wxx, j);
	  
	  // b[j] = m_wxx[j] + sinh_x[j];
	  b[j] = m_wxx[j];
	}
      }
      
      
      if(nnet.setWeights(W, l) == false)
	return false;
      
      if(nnet.setBias(b, l) == false)
	return false;
      
    }

    return true;
  }
  

  template <typename T>
  bool neuronlast_layer_mse(nnetwork<T>& nnet, const whiteice::dataset<T>& ds, unsigned int layer)
  {
    try{
      std::vector< whiteice::math::vertex<T> > output;
      
      if(ds.getData(1, output) == false)
	return false;
      
      math::matrix<T> W;
      math::vertex<T> b;
      
      if(nnet.getWeights(W, layer) == false)
	return false;
      
      if(nnet.getBias(b, layer) == false)
	return false;
      
      std::vector< math::vertex<T> > input;
      if(nnet.getSamples(input, layer) == false)
	return false;
      
      if(input.size() != output.size())
	return false;
      
      const unsigned int N = input.size();
      
      if(N <= 0) return false;
      
      {
	math::matrix<T> Cxx, Cxy;
	math::vertex<T> mx, my;
	
	Cxx.resize(input[0].size(),input[0].size());
	Cxy.resize(input[0].size(),output[0].size());
	mx.resize(input[0].size());
	my.resize(output[0].size());
	
	Cxx.zero();
	Cxy.zero();
	mx.zero();
	my.zero();
	
	for(unsigned int i=0;i<N;i++){
	  Cxx += input[i].outerproduct();
	  Cxy += input[i].outerproduct(output[i]);
	  mx  += input[i];
	  my  += output[i];
	}
	
	Cxx /= T((float)N);
	Cxy /= T((float)N);
	mx  /= T((float)N);
	my  /= T((float)N);
      
	Cxx -= mx.outerproduct();
	Cxy -= mx.outerproduct(my);
	
	Cxx.inv();
	
	W = Cxy.transpose() * Cxx;
	b = my - W*mx;
      }    
      
      
      if(nnet.setWeights(W, layer) == false)
	return false;
      
      if(nnet.setBias(b, layer) == false)
	return false;
    
      return true;
    }
    catch(std::exception& e){
      std::cout << "Unexpected exception: " << e.what() << std::endl;
      return false;
    }
  }
  
  
  /**
   * helper function to normalize neural network weight vectors 
   * ||w|| = 1 and ||b|| = 1 for each layer
   * 
   * (forcing this between every gradient descent steps in directly
   *  forces neural network weights to COMPETE against each other,
   *  this can be also useful in random search and in initialization
   *  step of the neural network weights)
   *
   * NOTE: if input x ~ N(0, I) then 
   *          Var[w^t x] = w^t COV(x) * w = ||w|| = 1 and variance
   *          of the output layer is fixed to be 1, which forces
   *          the problem to be "nice"
   *
   * NOTE2: normalization of the last layer does not make
   *        much sense so we don't do it as a default.
   *        (We need to get results that are close to correct ones)
   */
  template <typename T>
  bool normalize_weights_to_unity(nnetwork<T>& nnet,
				  bool normalizeLastLayer)
  {
    std::vector<unsigned int> arch;

    nnet.getArchitecture(arch);

    unsigned int N = arch.size()-1;

    if(normalizeLastLayer == false)
      N = arch.size()-2;

    for(unsigned int i=0;i<N;i++){
      math::matrix<T> W;
      math::vertex<T> b;
     
      if(!nnet.getWeights(W, i)) return false;

      for(unsigned int j=0;j<W.ysize();j++){
	W.rowcopyto(b, j);
	b.normalize();
	
	b *= T(2.0f); // scaling
	
	W.rowcopyfrom(b, j);
      }
      
      nnet.setWeights(W,i);

      // do not do anything to bias terms
    }

    return true;
  }
  
  
  template bool neuronlast_layer_mse<float>(nnetwork<float>& nnet, const dataset<float>& data, unsigned int layer);
  template bool neuronlast_layer_mse<double>(nnetwork<double>& nnet, const dataset<double>& data, unsigned int layer);
  template bool neuronlast_layer_mse< math::blas_real<float> >(nnetwork< math::blas_real<float> >& nnet, const dataset< math::blas_real<float> >& data, unsigned int layer);
  template bool neuronlast_layer_mse< math::blas_real<double> >(nnetwork< math::blas_real<double> >& nnet, const dataset< math::blas_real<double> >& data, unsigned int layer);
  
  template bool neuronlayerwise_ica<float>(nnetwork<float>& nnet, const float& alpha, unsigned int layer);
  template bool neuronlayerwise_ica<double>(nnetwork<double>& nnet, const double& alpha, unsigned int layer);
  template bool neuronlayerwise_ica< math::blas_real<float> >(nnetwork< math::blas_real<float> >& nnet, const math::blas_real<float>& alpha, unsigned int layer);
  template bool neuronlayerwise_ica< math::blas_real<double> >(nnetwork< math::blas_real<double> >& nnet, const math::blas_real<double>& alpha, unsigned int layer);

  template bool negative_feedback_between_neurons<float>(nnetwork<float>& nnet, const dataset<float>& data, const float& alpha, bool processLastLayer);
  template bool negative_feedback_between_neurons<double>(nnetwork<double>& nnet, const dataset<double>& data, const double& alpha, bool processLastLayer);
  template bool negative_feedback_between_neurons< math::blas_real<float> >(nnetwork< math::blas_real<float> >& nnet, const dataset< math::blas_real<float> >& data, const math::blas_real<float>& alpha, bool processLastLayer);
  template bool negative_feedback_between_neurons< math::blas_real<double> >(nnetwork< math::blas_real<double> >& nnet, const dataset< math::blas_real<double> >& data, const math::blas_real<double>& alpha, bool processLastLayer);

  template bool normalize_weights_to_unity<float>(nnetwork<float>& nnet, bool normalizeLastLayer);
  template bool normalize_weights_to_unity<double>(nnetwork<double>& nnet, bool normalizeLastLayer);
  template bool normalize_weights_to_unity< math::blas_real<float> >(nnetwork< math::blas_real<float> >& nnet, bool normalizeLastLayer);
  template bool normalize_weights_to_unity< math::blas_real<double> >(nnetwork< math::blas_real<double> >& nnet, bool normalizeLastLayer);
  
  
};
