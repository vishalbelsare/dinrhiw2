/*
 * Cart Pole (Inverted Pendulum) problem
 * 
 * Equations are from
 * 
 * "Correct equations for the dynamics of the cart-pole system"
 * Razvan V. Florian 2007
 * 
 * Applied force 
 */

#ifndef __whiteice__cartpole_h
#define __whiteice__cartpole_h

#include "RIFL_abstract.h"

#ifdef USE_SDL
#include <SDL.h>
#endif

#include <condition_variable>

namespace whiteice
{

  template <typename T>
    class CartPole : public RIFL_abstract<T>
    {
    public:
      CartPole();
      ~CartPole();

      bool physicsIsRunning(){ return this->running; }
      
    protected:

      virtual bool getState(whiteice::math::vertex<T>& state);
      
      virtual bool performAction(const unsigned int action,
				 whiteice::math::vertex<T>& newstate,
				 T& reinforcement);

    protected:

#ifdef USE_SDL
      int W, H;
      
      SDL_Window* window;
      SDL_Renderer* renderer;
#endif

      // resets cart-pole variables
      void reset();

      T sign(T value);

      // parameters
      T mc, mp, g, l;
      T uc, up;
      
      T theta, theta_dot, theta_dotdot;
      T x, x_dot, x_dotdot;
      T Nc;

      T F;
      std::mutex F_change;
      bool F_processed;

      T dt;
      int iteration;

      volatile bool running;
      std::thread* physics_thread;
      std::mutex physics_mutex;

      void physicsLoop();
      
    };

  
  extern template class CartPole< math::blas_real<float> >;
  extern template class CartPole< math::blas_real<double> >;
};


#endif
