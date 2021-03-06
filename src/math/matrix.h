/*
 * atlas optimized matrix class
 */
#ifndef matrix_h
#define matrix_h

#include "dinrhiw_blas.h"
#include "ownexception.h"
#include "number.h"

#include "compressable.h"
#include "MemoryCompressor.h"

#include <stdexcept>
#include <exception>
#include <vector>

#include <assert.h>

#ifndef DINRHIW_DEBUG
#define DINRHIW_DEBUG 1
#endif

namespace whiteice
{
  template <typename T>
    class neuronlayer;
  
  template <typename T>
    class backpropagation;
    
  namespace math
  {
    
    template <typename T> class vertex;
    template <typename T> class matrix;
      
        
    template <typename T> bool gramschmidt
      (matrix<T>& B, const unsigned int i, const unsigned int j);
    
    // correlation
    template <typename T> bool autocorrelation(matrix<T>& R, const std::vector< vertex<T> >& data);
    template <typename T> bool autocorrelation(matrix<T>& R, const matrix<T>& W);
    template <typename T> bool mean_covariance_estimate
      (vertex<T>& m, matrix<T>& R, const std::vector< vertex<T> >& data);
      
    // rotations
    template <typename T> bool rhouseholder_leftrot
      (matrix<T>& A, const unsigned int i, const unsigned int M, const unsigned int k, vertex<T>& v);    
    template <typename T> bool rhouseholder_rightrot
      (matrix<T>& A, const unsigned int i, const unsigned int M, const unsigned int k, vertex<T>& v);

    // outerproduct
    template <typename T>
      bool addouterproduct(matrix<T>& A,
			   const T& scalar,
			   const vertex<T>& a,
			   const vertex<T>& b);
    
       
    
    
    // T is type of the scalar
    
    template <typename T=blas_real<float> >      
      class matrix : public number< matrix<T>, T, T, unsigned int>
      // compressable has been commented out: see vertex.h
      //, public compressable
    {
      public:
      
      matrix(const unsigned int size_y = 4,
	     const unsigned int size_x = 4);
      matrix(const matrix<T>& M);
      //matrix(matrix<T>&& t);
      matrix(const vertex<T>& diagonal);
      virtual ~matrix();
      
      
      matrix<T> operator+(const matrix<T>&) const throw(illegal_operation);
      matrix<T> operator-(const matrix<T>&) const throw(illegal_operation);
      matrix<T> operator*(const matrix<T>&) const throw(illegal_operation);
      matrix<T> operator/(const matrix<T>&) const throw(illegal_operation);
      matrix<T> operator!() const throw(illegal_operation);
      matrix<T> operator-() const throw(illegal_operation);
      
      matrix<T>& operator+=(const matrix<T>&) throw(illegal_operation);
      matrix<T>& operator-=(const matrix<T>&) throw(illegal_operation);
      matrix<T>& operator*=(const matrix<T>&) throw(illegal_operation);
      matrix<T>& operator/=(const matrix<T>&) throw(illegal_operation);
      
      matrix<T>& operator=(const matrix<T>&) throw(illegal_operation);
      //matrix<T>& operator=(matrix<T>&& t) throw(illegal_operation);
      
      bool operator==(const matrix<T>&) const throw(uncomparable);
      bool operator!=(const matrix<T>&) const throw(uncomparable);
      bool operator>=(const matrix<T>&) const throw(uncomparable);
      bool operator<=(const matrix<T>&) const throw(uncomparable);
      bool operator< (const matrix<T>&) const throw(uncomparable);
      bool operator> (const matrix<T>&) const throw(uncomparable);

      // scalars and matrix interaction
      matrix<T>& operator= (const T&) throw(illegal_operation);

      matrix<T>  operator* (const T&) const throw();
      
      template <typename TT>
      friend matrix<TT> operator*(const TT&, const matrix<TT>&) throw(std::invalid_argument);
      
      
      matrix<T>  operator/ (const T&) const throw(std::invalid_argument);
      
      matrix<T>& operator*=(const T&) throw();
      matrix<T>& operator/=(const T&) throw(std::invalid_argument);
      
      vertex<T> operator*(const vertex<T>&) const throw(std::invalid_argument);      
      
      // doesn't behave as expected. returns index:th
      // value from matrix (left->right, top->bottom order)
      T& operator[](const unsigned int& index) throw(std::out_of_range, illegal_operation)
      {
#ifdef _GLIBCXX_DEBUG
	if(index >= numRows*numCols){ assert(0); throw std::out_of_range("vertex index out of range"); }
#endif
	return data[index]; // no range check
      }
      
      const T& operator[](const unsigned int& index) const throw(std::out_of_range, illegal_operation)
      {
#ifdef _GLIBCXX_DEBUG
	if(index >= numRows*numCols){ assert(0); throw std::out_of_range("vertex index out of range"); }
#endif	
	return data[index]; // no range checks
      }
      
      
      T& operator()(unsigned int y, unsigned int x) throw(std::out_of_range, illegal_operation)
      {
#ifdef _GLIBCXX_DEBUG
	if(y >= numRows || x >= numCols){ assert(0); throw std::out_of_range("vertex index out of range"); }
#endif
	
	return data[y*numCols + x]; // no range checks
      }
      
      
      const T& operator()(unsigned y, unsigned int x) const throw(std::out_of_range, illegal_operation)
      {
#ifdef _GLIBCXX_DEBUG
	if(y >= numRows || x >= numCols){ assert(0); throw std::out_of_range("vertex index out of range"); }
#endif	
	return data[y*numCols + x]; // no range checks
      }
      
      matrix<T>& identity();
      matrix<T>& zero();
      
      matrix<T>& crossproduct(const vertex<T>& v) throw(std::domain_error);
      
      // euclidean rotation
      matrix<T>& rotation(const T& xr, const T& yr, const T& zr) throw();
      
      // translation
      matrix<T>& translation(const T& dx, const T& dy, const T& dz) throw();
      matrix<T>& abs() throw();
      
      // transposes itself (changes matrix)
      matrix<T>& transpose() throw();

      // calculates hermitian matrix (conjugate transpose matrix)
      matrix<T>& hermite() throw();

      T det() const throw(std::logic_error); // determinate      
      T trace() const throw(std::logic_error);
      void diag(vertex<T>& diagonal) const throw();

      // inverse (returns false if matrix is singular and
      // value of matrix is garbage/undefined)
      bool inv() throw();
      
      // calculates pseudoinverse using svd (should not never fail)
      matrix<T>& pseudoinverse(const T machine_epsilon = T(0.0)) throw();

      // symmetric pseudoinverse (symmetric eig to calculate evd)
      bool symmetric_pseudoinverse(const T machine_epsilon = T(0.0)) throw();
      
      
      unsigned int size() const throw();
      unsigned int ysize() const throw(); // rows      
      unsigned int xsize() const throw(); // columns
      
      bool resize_x(unsigned int d) throw(); // columns
      bool resize_y(unsigned int d) throw(); // rows
      bool resize(unsigned int y, unsigned int x) throw();
      
      // calculates 2-norm of row or column subvector. if second coordinate is bigger
      // than number of columns/rows then it will be set to be number of columns/rows
      // (latter coordinate is last element which will be included < x/y size)
      T rownorm(unsigned int y, unsigned int x1,
		unsigned int x2 = ((unsigned int)-1)) const throw(std::out_of_range);
      
      T colnorm(unsigned int x, unsigned int y1,
		unsigned int y2 = ((unsigned int)-1)) const throw(std::out_of_range);
      
      // copies row data to a given vector, M(y,x1:x2) -> v
      void rowcopyto(vertex<T>& v, unsigned int y, unsigned int x1 = 0,
		     unsigned int x2 = ((unsigned int)-1)) const throw(std::out_of_range);
      
      // copies column data to a given vector, M(y1:y2,x) -> v
      void colcopyto(vertex<T>& v, unsigned int x, unsigned int y1 = 0,
		     unsigned int y2 = ((unsigned int)-1)) const throw(std::out_of_range);
      
      // copies data from vector to a row
      void rowcopyfrom(const vertex<T>& v, unsigned int y, unsigned int x1 = 0,
		       unsigned int x2 = ((unsigned int)-1)) throw(std::out_of_range);
      
      // copies data from vector to a column
      void colcopyfrom(const vertex<T>& v, unsigned int x, unsigned int y1 = 0,
		       unsigned int y2 = ((unsigned int)-1)) throw(std::out_of_range);
      
      // creates matrix M from submatrix ([x0,x0+xs-1],[y0:y0+ys-1])
      bool submatrix(matrix<T>& M,
		     unsigned int x0, unsigned int y0,
		     unsigned int xs, unsigned int ys) const;
      
      // writes matrix M to submatrix area ([x0+M.xsize()-1],[y0+M.ysize()-1])
      bool write_submatrix(const matrix<T>& M,
			   unsigned int x0, unsigned int y0);
      
      
      // writes and reads matrix data to/from vertex
      bool save_to_vertex(vertex<T>& out, unsigned int x0 = 0) const;
      bool load_from_vertex(const vertex<T>& in, unsigned int x0 = 0);
      
      
      // normalizes length of each row
      void normalize() throw();
      
      bool comparable() throw();

      // converts matrix to printable string
      void toString(std::string& line) const throw();
      
      //////////////////////////////////////////////////
      // matrix data compression
      
      bool compress() throw();
      bool decompress() throw();
      bool iscompressed() const throw();
      float ratio() const throw(); // compression ratio
      
      //////////////////////////////////////////////////
      
      friend class vertex<T>;
      friend class whiteice::neuronlayer<T>;
      friend class whiteice::backpropagation<T>;
      
      
      friend bool gramschmidt<T>(matrix<T>& B,
				 const unsigned int i,
				 const unsigned int j);
      
      // correlation
      friend bool autocorrelation<T>(matrix<T>& R, const std::vector< vertex<T> >& data);
      friend bool autocorrelation<T>(matrix<T>& R, const matrix<T>& W);
      friend bool mean_covariance_estimate<T>(vertex<T>& m, matrix<T>& R, const std::vector< vertex<T> >& data);
      
      // rotations
      friend bool rhouseholder_leftrot<T> (matrix<T>& A,
					   const unsigned int i,
					   const unsigned int M,
					   const unsigned int k,
					   vertex<T>& v);
      
      friend bool rhouseholder_rightrot<T>(matrix<T>& A,
					   const unsigned int i,
					   const unsigned int M,
					   const unsigned int k,
					   vertex<T>& v);

      // outerproduct
      friend bool addouterproduct<T>(matrix<T>& A,
				     const T& scalar,
				     const vertex<T>& a,
				     const vertex<T>& b);
      
      
      private:
      
      T* data;
      
      unsigned int numRows; // ysize
      unsigned int numCols; // xsize
      
      MemoryCompressor* compressor;
    };
    
    
    template <typename T>
      matrix<T> operator*(const T&, const matrix<T>&) throw(std::invalid_argument);
    
    template <typename T>
      std::ostream& operator<<(std::ostream& ios, const matrix<T>& M);
    
    
    // tries to convert matrix of type S to matrix of type T (B = A)    
    template <typename T, typename S>
      bool convert(matrix<T>& B, const matrix<S>& A) throw()
      {
	try{
	  if(B.resize(A.ysize(), A.xsize()) == false)
	    return false;
	  
	  for(unsigned int j=0;j<A.ysize();j++)
	    for(unsigned int i=0;i<A.xsize();i++)
	      B(j,i) = static_cast<T>(A(j,i));
	  
	  return true;
	}
	catch(std::exception& e){
	  return false;
	}
      }
  };
};


#include "linear_algebra.h"
#include "correlation.h"
#include "matrix_rotations.h"
#include "backpropagation.h"
#include "neuronlayer.h"  
#include "vertex.h"
#include "norms.h"
#include "outerproduct.h"

namespace whiteice
{
  namespace math
  {
    
    extern template class matrix<float>;
    extern template class matrix<double>;
    extern template class matrix<complex<float> >;
    extern template class matrix<complex<double> >;
    
    extern template class matrix<int>;
    extern template class matrix<char>;
    extern template class matrix<unsigned int>;
    extern template class matrix<unsigned char>;
        
    extern template class matrix< blas_real<float> >;
    extern template class matrix< blas_real<double> >;
    extern template class matrix< blas_complex<float> >;
    extern template class matrix< blas_complex<double> >;
    
    
    extern template matrix<float> operator*<float>(const float&, const matrix<float>&) throw(std::invalid_argument);
    extern template matrix<double> operator*<double>(const double&, const matrix<double>&) throw(std::invalid_argument);
    extern template matrix<complex<float> > operator*<complex<float> >(const complex<float>&, const matrix<complex<float> >&)
      throw(std::invalid_argument);    
    extern template matrix<complex<double> > operator*<complex<double> >(const complex<double>&, const matrix<complex<double> >&)
      throw(std::invalid_argument);
    
    extern template matrix<int> operator*<int>(const int&, const matrix<int>&) throw(std::invalid_argument);
    extern template matrix<char> operator*<char>(const char&, const matrix<char>&) throw(std::invalid_argument);
    extern template matrix<unsigned int> operator*<unsigned int>(const unsigned int&, const matrix<unsigned int>&)
      throw(std::invalid_argument);
    extern template matrix<unsigned char> operator*<unsigned char>(const unsigned char&, const matrix<unsigned char>&)
      throw(std::invalid_argument);
    
    
    extern template matrix<blas_real<float> > operator*<blas_real<float> >
      (const blas_real<float>&, const matrix<blas_real<float> >&) throw(std::invalid_argument);
       
    extern template matrix<blas_real<double> > operator*<blas_real<double> >
      (const blas_real<double>&, const matrix<blas_real<double> >&) throw(std::invalid_argument);
    
    
    extern template matrix<blas_complex<float> > operator*<blas_complex<float> >
      (const blas_complex<float>&, const matrix<blas_complex<float> >&) throw(std::invalid_argument);
    extern template matrix<blas_complex<double> > operator*<blas_complex<double> >
      (const blas_complex<double>&, const matrix<blas_complex<double> >&) throw(std::invalid_argument);
        
    extern template std::ostream& operator<< <float>(std::ostream& ios, const matrix<float>& M);
    extern template std::ostream& operator<< <double>(std::ostream& ios, const matrix<double>& M);
    extern template std::ostream& operator<< <complex<float> >(std::ostream& ios, const matrix<complex<float> >& M);
    extern template std::ostream& operator<< <complex<double> >(std::ostream& ios, const matrix<complex<double> >& M);
    extern template std::ostream& operator<< <int>(std::ostream& ios, const matrix<int>& M);
    extern template std::ostream& operator<< <char>(std::ostream& ios, const matrix<char>& M);
    extern template std::ostream& operator<< <unsigned int>(std::ostream& ios, const matrix<unsigned int>& M);
    extern template std::ostream& operator<< <unsigned char>(std::ostream& ios, const matrix<unsigned char>& M);
    extern template std::ostream& operator<< <blas_real<float> >(std::ostream& ios, const matrix<blas_real<float> >& M);
    extern template std::ostream& operator<< <blas_real<double> >(std::ostream& ios, const matrix<blas_real<double> >& M);
    extern template std::ostream& operator<< <blas_complex<float> >(std::ostream& ios, const matrix<blas_complex<float> >& M);
    extern template std::ostream& operator<< <blas_complex<double> >(std::ostream& ios, const matrix<blas_complex<double> >& M);
      
    
  };
};


#endif

