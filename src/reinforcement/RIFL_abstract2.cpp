// Reinforcement learning using continuous state and continuous actions


#include "RIFL_abstract2.h"

#include "NNGradDescent.h"
#include "PolicyGradAscent.h"

#include <assert.h>
#include <list>


namespace whiteice
{

  template <typename T>
  RIFL_abstract2<T>::RIFL_abstract2(unsigned int numActions,
				    unsigned int numStates)
  {
    // initializes parameters
    {
      gamma = T(0.8);
      epsilon = T(0.66);

      learningMode = true;

      hasModel.resize(2);
      hasModel[0] = false; // Q-network
      hasModel[1] = false; // policy-network
      
      this->numActions = numActions;
      this->numStates  = numStates;
    }

    
    // initializes neural network architecture and weights randomly
    {
      std::vector<unsigned int> arch;
      
      {
	std::lock_guard<std::mutex> lock(Q_mutex);
	
	arch.push_back(numStates + numActions);
	arch.push_back(numStates*20);
	arch.push_back(numStates*20);
	arch.push_back(1);
	
	{
	  whiteice::nnetwork<T> nn(arch, whiteice::nnetwork<T>::halfLinear);
	  nn.setNonlinearity(nn.getLayers()-1, whiteice::nnetwork<T>::pureLinear);
	  nn.randomize();
	  
	  Q.importNetwork(nn);
	}
      }

      {
	std::lock_guard<std::mutex> lock(policy_mutex);
	
	arch.clear();
	arch.push_back(numStates);
	arch.push_back(numStates*20);
	arch.push_back(numStates*20);
	arch.push_back(numActions);

	// policy outputs action is (should be) [0,1]^D vector
	{
	  whiteice::nnetwork<T> nn(arch, whiteice::nnetwork<T>::halfLinear);
	  // nn.setNonlinearity(nn.getLayers()-1, whiteice::nnetwork<T>::sigmoid);
	  nn.setNonlinearity(nn.getLayers()-1, whiteice::nnetwork<T>::pureLinear);
	  nn.randomize();
	  
	  policy.importNetwork(nn);
	}
      }
      
    }
    
    
    thread_is_running = 0;
    rifl_thread = nullptr;
  }

  template <typename T>
  RIFL_abstract2<T>::~RIFL_abstract2() throw()
  {
    // stops executing thread
    {
      if(thread_is_running <= 0) return;

      std::lock_guard<std::mutex> lock(thread_mutex);

      if(thread_is_running <= 0) return;

      thread_is_running--;

      if(rifl_thread){
	rifl_thread->join();
	delete rifl_thread;
      }

      rifl_thread = nullptr;
    }
  }

  
  // starts Reinforcement Learning thread
  template <typename T>
  bool RIFL_abstract2<T>::start()
  {
    if(thread_is_running != 0) return false;

    std::lock_guard<std::mutex> lock(thread_mutex);

    if(thread_is_running != 0) return false;

    try{
      thread_is_running++;
      rifl_thread = new thread(std::bind(&RIFL_abstract2<T>::loop, this));
    }
    catch(std::exception& e){
      thread_is_running--;
      rifl_thread = nullptr;

      return false;
    }

    return true;
  }

  
  // stops Reinforcement Learning thread
  template <typename T>
  bool RIFL_abstract2<T>::stop()
  {
    if(thread_is_running <= 0) return false;

    std::lock_guard<std::mutex> lock(thread_mutex);

    if(thread_is_running <= 0) return false;

    thread_is_running--;

    if(rifl_thread){
      rifl_thread->join();
      delete rifl_thread;
    }

    rifl_thread = nullptr;
    return true;
  }

  template <typename T>
  bool RIFL_abstract2<T>::isRunning() const
  {
    return (thread_is_running > 0);
  }


  // epsilon E [0,1] percentage of actions are chosen according to model
  //                 1-e percentage of actions are random (exploration)
  template <typename T>
  bool RIFL_abstract2<T>::setEpsilon(T epsilon) throw()
  {
    if(epsilon < T(0.0) || epsilon > T(1.0)) return false;
    this->epsilon = epsilon;
    return true;
  }
  

  template <typename T>
  T RIFL_abstract2<T>::getEpsilon() const throw()
  {
    return epsilon;
  }


  template <typename T>
  void RIFL_abstract2<T>::setLearningMode(bool learn) throw()
  {
    learningMode = learn;
  }

  template <typename T>
  bool RIFL_abstract2<T>::getLearningMode() const throw()
  {
    return learningMode;
  }


  template <typename T>
  void RIFL_abstract2<T>::setHasModel(bool hasModel) throw()
  {
    this->hasModel[0] = hasModel;
    this->hasModel[1] = hasModel;
  }

  template <typename T>
  bool RIFL_abstract2<T>::getHasModel() throw()
  {
    return (hasModel[0] && hasModel[1]);
  }

  
  // saves learnt Reinforcement Learning Model to file
  template <typename T>
  bool RIFL_abstract2<T>::save(const std::string& filename) const
  {
    std::lock_guard<std::mutex> lock1(Q_mutex);
    std::lock_guard<std::mutex> lock2(policy_mutex);

    char buffer[256];
    
    snprintf(buffer, 256, "q-%s", filename.c_str());    
    if(Q.save(buffer) == false) return false;

    snprintf(buffer, 256, "policy-%s", filename.c_str());
    if(policy.save(buffer) == false) return false;

    return true;
  }

  
  // loads learnt Reinforcement Learning Model from file
  template <typename T>
  bool RIFL_abstract2<T>::load(const std::string& filename)
  {
    std::lock_guard<std::mutex> lock1(Q_mutex);
    std::lock_guard<std::mutex> lock2(policy_mutex);

    char buffer[256];
        
    snprintf(buffer, 256, "q-%s", filename.c_str());    
    if(Q.load(buffer) == false) return false;

    snprintf(buffer, 256, "policy-%s", filename.c_str());
    if(policy.load(buffer) == false) return false;
    
    return true;
  }
  

  template <typename T>
  void RIFL_abstract2<T>::loop()
  {
    std::vector< rifl2_datapoint<T> > database;
    
    whiteice::dataset<T> data;    
    whiteice::math::NNGradDescent<T> grad; // Q(state,action) model optimizer

    whiteice::dataset<T> data2;
    whiteice::PolicyGradAscent<T> grad2;   // policy(state)=action model optimizer
    
    std::vector<unsigned int> epoch;

    epoch.resize(2);
    epoch[0] = 0;
    epoch[1] = 0;

    const unsigned int DATASIZE = 10000;
    const unsigned int SAMPLESIZE = 100;

    // const T tau = T(0.30); // we keep 30% of the new networks weights (60% old)
    const T tau = T(1.00); // we keep 100% of the new networks weights (0% old)

    

    bool firstTime = true;
    whiteice::math::vertex<T> state;

    while(thread_is_running > 0){

      // 1. gets current state
      {
	auto oldstate = state;
      
	if(getState(state) == false){
	  state = oldstate;
	  if(firstTime) continue;
	}

	firstTime = false;
      }

      // 2. selects action using policy (+ add random noise ~Normal for exploration)
      whiteice::math::vertex<T> action(numActions);
      
      {
	std::lock_guard<std::mutex> lock(policy_mutex);

	whiteice::math::vertex<T> u;
	whiteice::math::matrix<T> e;

	if(policy.calculate(state, u, e, 1, 0) == true){
	  if(u.size() != numActions){
	    u.resize(numActions);
	    for(unsigned int i=0;i<numActions;i++){
	      u[i] = T(0.0);
	    }
	  }
	}
	else{
	  u.resize(numActions);
	  for(unsigned int i=0;i<numActions;i++){
	    u[i] = T(0.0);
	  }
	}

	// it is assumed that action data should have zero mean and is roughly
	// normally distributed (with StDev[n] = 1) so data is close to zero

	// FIXME add better random normally distributed noise (exploration)
	if(learningMode){
	  auto noise = u;
	  rng.normal(noise); // Normal E[n]=0 StDev[n]=1
	  u += noise;

#if 0
	  for(unsigned int i=0;i<u.size();i++){
	    if(u[i] < T(0.0)) u[i] = T(0.0); // [keep things between [0,1]
	    else if(u[i] > T(1.0)) u[i] = T(1.0);
	  }
#endif
	}

	// if have no model then make random selections (normally distributed)
	if(hasModel[0] == false || hasModel[1] == false){
	  rng.normal(u);

#if 0
	  for(unsigned int i=0;i<u.size();i++){
	    if(u[i] < T(0.0)) u[i] = T(0.0); // [keep things between [0,1]
	    else if(u[i] > T(1.0)) u[i] = T(1.0);
	  }
#endif
	}

	action = u;
      }

      whiteice::math::vertex<T> newstate;
      T reinforcement = T(0.0);

      // 3. perform action and get newstate and reinforcement
      {
	if(performAction(action, newstate, reinforcement) == false){
	  continue;
	}
      }

      
      if(learningMode == false){
	continue; // we do not do learning
      }
      

      // 4. updates database (of actions and responses)
      {
	struct rifl2_datapoint<T> data;

	data.state = state;
	data.action = action;
	data.newstate = newstate;
	data.reinforcement = reinforcement;	

	if(database.size() >= DATASIZE){
	  const unsigned int index = rng.rand() % database.size();
	  database[index] = data;
	}
	else{
	  database.push_back(data);
	}

	printf("DATABASE SIZE: %d\n", (int)database.size());
	fflush(stdout);
      }

      
      // 5. update/optimize Q(state, action) network
      // activates batch learning if it is not running
      if(database.size() >= SAMPLESIZE)
      {
	// skip if other optimization step is behind us
	if(epoch[0] > epoch[1])
	  goto q_optimization_done;
	
	
	if(grad.isRunning() == false){
	  whiteice::nnetwork<T> nn;
	  T error;
	  unsigned int iters;

	  if(grad.getSolution(nn, error, iters) == false){
	    std::vector< math::vertex<T> > weights;

	    std::lock_guard<std::mutex> lock(Q_mutex);
	    
	    if(Q.exportSamples(nn, weights, 1) == false){
	      assert(0);
	    }
	    
	    assert(weights.size() > 0);
	    
	    if(nn.importdata(weights[0]) == false){
	      assert(0);
	    }
	  }
	  else{
	    if(epoch[0] > 0){
	      std::lock_guard<std::mutex> lock(Q_mutex);
	      
	      // we keep previous network to some degree
	      {
		whiteice::nnetwork<T> nnprev = nn;
		std::vector< whiteice::math::vertex<T> > prevweights;
		
		if(Q.exportSamples(nnprev, prevweights)){
		  whiteice::math::vertex<T> newweights;
		  
		  if(nn.exportdata(newweights)){
		    newweights = tau*newweights + (T(1.0)-tau)*prevweights[0];
		    
		    nn.importdata(newweights);
		  }
		}
		
		Q.importNetwork(nn);
		hasModel[0] = true;
	      }
	    }
	    else{
	      std::lock_guard<std::mutex> lock(Q_mutex);
	      Q.importNetwork(nn);
	      hasModel[0] = true;
	    }

	    epoch[0]++;
	  }

	  // const unsigned int BATCHSIZE = database.size()/2;
	  const unsigned int BATCHSIZE = 1000;
	  
	  data.clear();
	  data.createCluster("input-state", numStates + numActions);
	  data.createCluster("output-action", 1);
	  
	  for(unsigned int i=0;i<BATCHSIZE;){
	    const unsigned int index = rng.rand() % database.size();

	    whiteice::math::vertex<T> in(numStates + numActions);
	    in.zero();
	    in.write_subvertex(database[index].state, 0);
	    in.write_subvertex(database[index].action, numStates);
	    
	    whiteice::math::vertex<T> out(1);
	    out.zero();

	    // calculates updated utility value
	    whiteice::math::vertex<T> y(1);
	    
	    {
	      whiteice::math::vertex<T> tmp(numStates + numActions);
	      tmp.write_subvertex(database[index].newstate, 0);
	      
	      {
		whiteice::math::vertex<T> u; // new action..
		whiteice::math::matrix<T> e;

		policy.calculate(database[index].newstate, u, e, 1, 0);

		// add exporation noise?

		tmp.write_subvertex(u, numStates); // writes policy's action
	      }
	      
	      nn.calculate(tmp, y);
	      
	      if(epoch[0] > 0){
		out[0] = database[index].reinforcement + gamma*y[0];
	      }
	      else{ // the first iteration of reinforcement learning do not use Q
		out[0] = database[index].reinforcement;
	      }

	    }

	    data.add(0, in);
	    data.add(1, out);

	    i++;
	  }

	  grad.startOptimize(data, nn, 2, 150);
	}
	else{
	  whiteice::nnetwork<T> nn;
	  T error = T(0.0);
	  unsigned int iters = 0;

	  if(grad.getSolution(nn, error, iters)){
	    printf("Q-EPOCH %d OPTIMIZER %d ITERS: ERROR %f\n",
		   epoch[0], iters, error.c[0]);
	  }
	}
      }
      
    q_optimization_done:
      
      // 6. update/optimize policy(state) network
      // activates batch learning if it is not running
      
      if(database.size() >= SAMPLESIZE)
      {
	// skip if other optimization step is behind us
	if(epoch[1] > epoch[0])
	  goto policy_optimization_done; 

	
	if(grad2.isRunning() == false){
	  whiteice::nnetwork<T> nn;
	  T meanq;
	  unsigned int iters;

	  if(grad2.getSolution(nn, meanq, iters) == false){
	    std::vector< math::vertex<T> > weights;

	    std::lock_guard<std::mutex> lock(policy_mutex);
	    
	    if(policy.exportSamples(nn, weights, 1) == false){
	      assert(0);
	    }
	    
	    assert(weights.size() > 0);
	    
	    if(nn.importdata(weights[0]) == false){
	      assert(0);
	    }
	  }
	  else{
	    if(epoch[1] > 0){
	      std::lock_guard<std::mutex> lock(policy_mutex);
	      
	      // we keep previous network to some degree
	      {
		whiteice::nnetwork<T> nnprev = nn;
		std::vector< whiteice::math::vertex<T> > prevweights;
		
		if(policy.exportSamples(nnprev, prevweights)){
		  whiteice::math::vertex<T> newweights;
		  
		  if(nn.exportdata(newweights)){
		    newweights = tau*newweights + (T(1.0)-tau)*prevweights[0];
		    
		    nn.importdata(newweights);
		  }
		}
		
		policy.importNetwork(nn);
		hasModel[1] = true;
	      }
	    }
	    else{
	      std::lock_guard<std::mutex> lock(policy_mutex);
	      policy.importNetwork(nn);
	      hasModel[1] = true;
	    }
	  }

	  epoch[1]++;
	    
	  // const unsigned int BATCHSIZE = database.size()/2;
	  const unsigned int BATCHSIZE = 1000;
	  
	  data2.clear();
	  data2.createCluster("input-state", numStates);
	  
	  for(unsigned int i=0;i<BATCHSIZE;i++){
	    const unsigned int index = rng.rand() % database.size();
	    data2.add(0, database[index].state);
	  }

	  {
	    whiteice::nnetwork<T> q_nn;	    

	    {
	      std::lock_guard<std::mutex> lock(Q_mutex);
	      std::vector< math::vertex<T> > weights;
	      
	      if(Q.exportSamples(q_nn, weights, 1) == false){
		assert(0);
	      }
	      
	      assert(weights.size() > 0);
	      
	      if(q_nn.importdata(weights[0]) == false){
		assert(0);
	      }
	    }
	    
	    grad2.startOptimize(&data2, q_nn, nn, 2, 150);
	  }
	}
	else{
	  whiteice::nnetwork<T> nn;
	  T value = T(0.0);
	  unsigned int iters = 0;

	  if(grad2.getSolution(nn, value, iters)){
	    printf("POLICY-EPOCH %d OPTIMIZER %d ITERS: MEAN Q-VALUE %f\n",
		   epoch[1], iters, value.c[0]);
	  }
	}
      }
      
    policy_optimization_done:
      
      (1 == 1); // dummy [work-around bug/feature goto requiring expression]
      
    }

    grad.stopComputation();
    grad2.stopComputation();
    
  }

  template class RIFL_abstract2< math::blas_real<float> >;
  template class RIFL_abstract2< math::blas_real<double> >;
  
};