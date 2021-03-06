
#include "CreateRIFLdataset.h"

#include <pthread.h>
#include <sched.h>

#include <functional>

#ifdef WINOS
#include <windows.h>
#endif

#include "Log.h"


namespace whiteice
{
  
  // calculates reinforcement learning training dataset from database
  // using database_lock
  template <typename T>
  CreateRIFLdataset<T>::CreateRIFLdataset(RIFL_abstract<T> const & rifl_,
					  std::vector< std::vector< rifl_datapoint<T> > > const & database_,
					  std::mutex & database_mutex_,
					  unsigned int const & epoch_,
					  whiteice::dataset<T>& data_) :
    rifl(rifl_), 
    database(database_),
    database_mutex(database_mutex_),
    epoch(epoch_),
    data(data_)
  {
    worker_thread = nullptr;
    running = false;
    completed = false;
  }

  
  template <typename T>
  CreateRIFLdataset<T>::~CreateRIFLdataset()
  {
    std::lock_guard<std::mutex> lk(thread_mutex);
    
    if(running || worker_thread != nullptr){
      running = false;
      if(worker_thread) worker_thread->join();
      delete worker_thread;
      worker_thread = nullptr;
    }
  }
  
  // starts thread that creates NUMDATAPOINTS samples to dataset
  template <typename T>
  bool CreateRIFLdataset<T>::start(const unsigned int NUMDATAPOINTS)
  {
    if(NUMDATAPOINTS == 0) return false;

    std::lock_guard<std::mutex> lock(thread_mutex);

    if(running == true || worker_thread != nullptr)
      return false;

    try{
      NUMDATA = NUMDATAPOINTS;
      data.clear();
      data.createCluster("input-state", rifl.numStates + rifl.dimActionFeatures);
      data.createCluster("output-action", 1);
      
      completed = false;
      
      running = true;
      worker_thread = new std::thread(std::bind(&CreateRIFLdataset<T>::loop, this));
      
    }
    catch(std::exception&){
      running = false;
      if(worker_thread){ delete worker_thread; worker_thread = nullptr; }
      return false;
    }

    return true;
  }
  
  // returns true when computation is completed
  template <typename T>
  bool CreateRIFLdataset<T>::isCompleted() const
  {
    return completed;
  }
  
  // returns true if computation is running
  template <typename T>
  bool CreateRIFLdataset<T>::isRunning() const
  {
    return running;
  }

  template <typename T>
  bool CreateRIFLdataset<T>::stop()
  {
    std::lock_guard<std::mutex> lock(thread_mutex);
    
    if(running || worker_thread != nullptr){
      running = false;
      if(worker_thread) worker_thread->join();
      delete worker_thread;
      worker_thread = nullptr;

      return true;
    }
    else return false;
  }
  
  // returns reference to dataset
  // (warning: if calculations are running then dataset can change during use)
  template <typename T>
  whiteice::dataset<T> const & CreateRIFLdataset<T>::getDataset() const
  {
    return data;
  }
  
  // worker thread loop
  template <typename T>
  void CreateRIFLdataset<T>::loop()
  {
    // set thread priority (non-standard) to low (background thread)
    {
      sched_param sch_params;
      int policy = SCHED_FIFO;
      
      pthread_getschedparam(pthread_self(),
			    &policy, &sch_params);

#ifdef linux
      policy = SCHED_IDLE; // in linux we can set idle priority
#endif
      sch_params.sched_priority = sched_get_priority_min(policy);
      
      if(pthread_setschedparam(pthread_self(),
				 policy, &sch_params) != 0){
	// printf("! SETTING LOW PRIORITY THREAD FAILED\n");
      }
      
#ifdef WINOS
      SetThreadPriority(GetCurrentThread(),
			THREAD_PRIORITY_IDLE);
#endif	
    }

    
    // used to calculate avg max abs(Q)-value
    // (internal debugging for checking that Q-values are within sane limits)
    std::vector<T> maxvalues;
    
    
#pragma omp parallel for schedule(dynamic)
    for(unsigned int i=0;i<NUMDATA;i++){

      if(running == false) // we don't do anything anymore..
	continue; // exits OpenMP loop..

      database_mutex.lock();
      
      const unsigned int action = rifl.rng.rand() % rifl.numActions;
      const unsigned int index = rifl.rng.rand() % database[action].size();

      const auto datum = database[action][index];

      database_mutex.unlock();
      
      whiteice::math::vertex<T> in;
      whiteice::math::vertex<T> feature;

      rifl.getActionFeature(action, feature);
      
      in.resize(rifl.numStates + rifl.dimActionFeatures);
      in.zero();
      in.write_subvertex(datum.state, 0);

      in.write_subvertex(feature, rifl.numStates);
      
      whiteice::math::vertex<T> out(1);
      out.zero();
      
      // calculates updated utility value
	      
      whiteice::math::vertex<T> u;
      
      T unew_value = T(0.0);
      
      T maxvalue = T(-INFINITY);
      
      {
	
	for(unsigned int j=0;j<rifl.numActions;j++){
	  std::lock_guard<std::mutex> lock(rifl.model_mutex);
	  
	  whiteice::math::vertex<T> u;
	  whiteice::math::matrix<T> e;
	  
	  whiteice::math::vertex<T> input(rifl.numStates + rifl.dimActionFeatures);
	  whiteice::math::vertex<T> f;
	  
	  input.zero();
	  input.write_subvertex(datum.newstate, 0);
	  
	  rifl.getActionFeature(j, f);
	  input.write_subvertex(f, rifl.numStates);
	  
	  rifl.preprocess.preprocess(0, input);
	  
	  if(rifl.model.calculate(input, u, e, 1, 0) == true){
	    if(u.size() != 1){
	      u.resize(1);
	      u[0] = T(0.0);
	    }
	    else
	      rifl.preprocess.invpreprocess(1, u);
	  }
	  else{
	    u.resize(1);
	    u[0] = T(0.0);
	  }
	  
	  if(maxvalue < u[0])
	    maxvalue = u[0];
	}

	if(epoch > 0){
	  unew_value =
	    datum.reinforcement + rifl.gamma*maxvalue;
	}
	else{ // first iteration always uses raw reinforcement values
	  unew_value =
	    datum.reinforcement;
	}
      }
      
      out[0] = unew_value;
      
#pragma omp critical
      {
	data.add(0, in);
	data.add(1, out);

	maxvalues.push_back(maxvalue);
      }
      
    }

    if(running == false)
      return; // exit point
    
    // add preprocessing to dataset
    {
      data.preprocess
	(0, whiteice::dataset<T>::dnMeanVarianceNormalization);
      
      data.preprocess
	(1, whiteice::dataset<T>::dnMeanVarianceNormalization);
    }

    
    // for debugging purposes (reports average max Q-value)
    if(maxvalues.size() > 0)
    {
      T sum = T(0.0);
      for(auto& m : maxvalues)
	sum += abs(m);

      sum /= T(maxvalues.size());

      double tmp = 0.0;
      whiteice::math::convert(tmp, sum);

      char buffer[80];
      snprintf(buffer, 80, "CreateRIFLdataset: avg abs(max(Q))-value %f",
	       tmp);

      whiteice::logging.info(buffer);
    }

    completed = true;

    {
      std::lock_guard<std::mutex> lock(thread_mutex);
      running = false;
    }
  }
  

  template class CreateRIFLdataset< math::blas_real<float> >;
  template class CreateRIFLdataset< math::blas_real<double> >;
};
