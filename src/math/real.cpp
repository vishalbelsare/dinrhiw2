/*
 * abtrary precision real number simulation implementation
 *
 */

#include "real.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace whiteice
{
  namespace math
  {
    
    
    realnumber::realnumber(unsigned long int prec){
      if(!prec) mpf_init(data);
      else mpf_init2(data, prec);
    }
    
    
    realnumber::realnumber(const realnumber& r, unsigned long int prec){
      if(!prec) mpf_init2(data, r.getPrecision());
      else mpf_init2(data, prec);
      
      mpf_set(data, r.data);
    }
    
    
    realnumber::realnumber(signed long i, unsigned long int prec){
      if(!prec) mpf_init(data);
      else mpf_init2(data, prec);
      
      mpf_set_si(data, i);
    }
    
    
    realnumber::realnumber(unsigned long i, unsigned long int prec){
      if(!prec) mpf_init(data);
      else mpf_init2(data, prec);
      
      mpf_set_ui(data, i);
    }
    
    
    realnumber::realnumber(double d, unsigned long int prec){
      if(!prec) mpf_init(data);
      else mpf_init2(data, prec);
      
      mpf_set_d(data, d);
    }
    
    
    realnumber::realnumber(const std::string& s, unsigned long int prec){
      if(!prec) mpf_init(data);
      else mpf_init2(data, prec);
      
      mpf_set_str(data, s.c_str(), 10);
    }
    
    realnumber::realnumber(const mpf_t& d){
      mpf_init2(data, mpf_get_prec(d));
      mpf_set(data, d);
      
      // data[0] = d[0];
      
      // memcpy(data, d, sizeof(mpf_t));
    }
    
    
    realnumber::~realnumber(){ mpf_clear(data); }
    
    
    ////////////////////////////////////////////////////////////
    
    
    unsigned long int realnumber::getPrecision() const throw(){
      return mpf_get_prec(data);
    }
    
    
    void realnumber::setPrecision(unsigned long int prec) throw(){
      mpf_set_prec(data, prec);
    }
    
    
    // operators
    realnumber realnumber::operator+(const realnumber& r) const throw(illegal_operation){
      mpf_t rval;
      unsigned long int p1 = mpf_get_prec(data);
      unsigned long int p2 = mpf_get_prec(r.data);
      if(p1 >= p2) mpf_init2(rval, p1);
      else mpf_init2(rval, p2);
      
      mpf_add(rval, data, r.data);
      
      auto ret = realnumber(rval);
      mpf_clear(rval);
      
      return ret;
    }
    
    
    realnumber realnumber::operator-(const realnumber& r) const throw(illegal_operation){
      mpf_t rval;
      unsigned long int p1 = mpf_get_prec(data);
      unsigned long int p2 = mpf_get_prec(r.data);
      if(p1 >= p2) mpf_init2(rval, p1);
      else mpf_init2(rval, p2);
      
      mpf_sub(rval, data, r.data);

      auto ret = realnumber(rval);
      mpf_clear(rval);
      
      return ret;
    }
    
    
    realnumber realnumber::operator*(const realnumber& r) const throw(illegal_operation){
      mpf_t rval;
      unsigned long int p1 = mpf_get_prec(data);
      unsigned long int p2 = mpf_get_prec(r.data);
      if(p1 >= p2) mpf_init2(rval, p1);
      else mpf_init2(rval, p2);
      
      mpf_mul(rval, data, r.data);

      auto ret = realnumber(rval);
      mpf_clear(rval);
      
      return ret;
    }
    
    
    realnumber realnumber::operator/(const realnumber& r) const throw(illegal_operation){
      mpf_t rval;
      unsigned long int p1 = mpf_get_prec(data);
      unsigned long int p2 = mpf_get_prec(r.data);
      if(p1 >= p2) mpf_init2(rval, p1);
      else mpf_init2(rval, p2);
      
      if(mpf_cmp_si(r.data, 0) == 0){ // division by zero
	throw illegal_operation("Division by zero");
      }
      
      mpf_div(rval, data, r.data);
      
      auto ret = realnumber(rval);
      mpf_clear(rval);
      
      return ret;
    }
    
    
    // complex conjugate
    realnumber realnumber::operator!() const throw(illegal_operation){
      return realnumber(*this); // nothing t do
    }
    
    
    realnumber realnumber::operator-() const throw(illegal_operation){
      mpf_t rval;
      mpf_init2(rval, mpf_get_prec(data));
      mpf_neg(rval, data);

      auto ret = realnumber(rval);
      mpf_clear(rval);
      
      return ret;
    }
    
    
    realnumber& realnumber::operator+=(const realnumber& r) throw(illegal_operation){
      mpf_t temp;
      mpf_init2(temp, mpf_get_prec(data));
      mpf_set(temp, data);
      mpf_add(data, temp, r.data);
      mpf_clear(temp);
      
      return (*this);
    }
    
    realnumber& realnumber::operator-=(const realnumber& r) throw(illegal_operation){
      mpf_t temp;
      mpf_init2(temp, mpf_get_prec(data));
      mpf_set(temp, data);
      mpf_sub(data, temp, r.data);
      mpf_clear(temp);
      
      return (*this);
    }
    
    realnumber& realnumber::operator*=(const realnumber& r) throw(illegal_operation){
      mpf_t temp;
      mpf_init2(temp, mpf_get_prec(data));
      mpf_set(temp, data);
      mpf_mul(data, temp, r.data);
      mpf_clear(temp);
      
      return (*this);
    }
    
    realnumber& realnumber::operator/=(const realnumber& r) throw(illegal_operation){
      mpf_t temp;
      mpf_init2(temp, mpf_get_prec(data));
      mpf_set(temp, data);
      mpf_div(data, temp, r.data);
      mpf_clear(temp);
      
      return (*this);
    }
    
    
    realnumber& realnumber::operator=(const realnumber& r) throw(illegal_operation){
      if(this != &r){
	mpf_set_prec(data, mpf_get_prec(r.data));
	mpf_set(data, r.data);
      }
      
      return (*this);
    }
    
    
    /*************************************************************/
    
    
    bool realnumber::operator==(const realnumber& r) const throw(uncomparable){
      return (mpf_cmp(data, r.data) == 0);
    }
    
    bool realnumber::operator!=(const realnumber& r) const throw(uncomparable){
      return (mpf_cmp(data, r.data) != 0);
    }
    
    bool realnumber::operator>=(const realnumber& r) const throw(uncomparable){
      return (mpf_cmp(data, r.data) >= 0);
    }
    
    bool realnumber::operator<=(const realnumber& r) const throw(uncomparable){
      return (mpf_cmp(data, r.data) <= 0);
    }
    
    bool realnumber::operator< (const realnumber& r) const throw(uncomparable){
      return (mpf_cmp(data, r.data) < 0);
    }
    
    bool realnumber::operator> (const realnumber& r) const throw(uncomparable){
      return (mpf_cmp(data, r.data) > 0);
    }
    
    
    // scalar operation
    realnumber& realnumber::operator= (const double& s) throw(illegal_operation){
      mpf_set_d(data, s);
      return (*this);
    }
    
    
    realnumber  realnumber::operator+ (const double& s) const throw(){
      mpf_t temp, ss;
      mpf_init2(temp, mpf_get_prec(data));
      mpf_init2(ss, mpf_get_prec(data));
      mpf_set_d(ss, s);
      mpf_add(temp, data, ss);
      mpf_clear(ss);

      auto ret = realnumber(temp);
      mpf_clear(temp);
      
      return ret;
    }
    
    
    realnumber  realnumber::operator- (const double& s) const throw(){
      mpf_t temp, ss;
      mpf_init2(temp, mpf_get_prec(data));
      mpf_init2(ss, mpf_get_prec(data));
      mpf_set_d(ss, s);
      mpf_sub(temp, data, ss);
      mpf_clear(ss);

      auto ret = realnumber(temp);
      mpf_clear(temp);
      
      return ret;
    }
    
    
    realnumber& realnumber::operator+=(const double& s) throw(){
      mpf_t temp, ss;
      mpf_init2(temp, mpf_get_prec(data));
      mpf_init2(ss, mpf_get_prec(data));
      mpf_set(temp, data);
      mpf_set_d(ss, s);
      mpf_add(data, temp, ss);
      mpf_clear(ss);
      mpf_clear(temp);
      
      return (*this);
    }
    
    
    realnumber& realnumber::operator-=(const double& s) throw(){
      mpf_t temp, ss;
      mpf_init2(temp, mpf_get_prec(data));
      mpf_init2(ss, mpf_get_prec(data));
      mpf_set(temp, data);
      mpf_set_d(ss, s);
      mpf_sub(data, temp, ss);
      mpf_clear(ss);
      mpf_clear(temp);
      
      return (*this);
    }
    
    
    realnumber  realnumber::operator* (const double& s) const throw(){
      mpf_t ss, res;
      mpf_init2(ss, mpf_get_prec(data));
      mpf_init2(res, mpf_get_prec(data));
      mpf_set_d(ss, s);
      mpf_mul(res, ss, data);
      mpf_clear(ss);

      auto ret = realnumber(res);
      mpf_clear(res);
      
      return ret;
    }
    
    realnumber  realnumber::operator/ (const double& s) const throw(std::invalid_argument){
      mpf_t ss, res;
      mpf_init2(ss, mpf_get_prec(data));
      mpf_init2(res, mpf_get_prec(data));
      mpf_set_d(ss, s);
      mpf_div(res, ss, data);
      mpf_clear(ss);

      auto ret = realnumber(res);
      mpf_clear(res);
      
      return ret;
    }
    
    
    realnumber& realnumber::operator*=(const double& s) throw(){
      mpf_t ss, temp;
      mpf_init2(ss, mpf_get_prec(data));
      mpf_init2(temp, mpf_get_prec(data));
      mpf_set(temp, data);
      mpf_set_d(ss, s);
      mpf_mul(data, temp, ss);
      mpf_clear(temp);
      mpf_clear(ss);
      
      return (*this);
    }
    
    
    realnumber& realnumber::operator/=(const double& s) throw(std::invalid_argument){
      mpf_t ss, temp;
      mpf_init2(ss, mpf_get_prec(data));
      mpf_init2(temp, mpf_get_prec(data));
      mpf_set(temp, data);
      mpf_set_d(ss, s);
      mpf_div(data, temp, ss);
      mpf_clear(temp);
      mpf_clear(ss);
      
      return (*this);
    }
    
    
    // scalar comparisions
    bool realnumber::operator==(const double d) const throw(){
      return (mpf_cmp_d(data, d) == 0);
    }
    
    
    bool realnumber::operator!=(const double d) const throw(){
      return (mpf_cmp_d(data, d) != 0);
    }
    
    
    bool realnumber::operator>=(const double d) const throw(){
      return (mpf_cmp_d(data, d) >= 0);
    }
    
    
    bool realnumber::operator<=(const double d) const throw(){
      return (mpf_cmp_d(data, d) <= 0);
    }
    
    
    bool realnumber::operator< (const double d) const throw(){
      return (mpf_cmp_d(data, d) < 0);
    }
    
    
    bool realnumber::operator> (const double d) const throw(){
      return (mpf_cmp_d(data, d) > 0);
    }
    
    
    bool realnumber::operator==(const signed long int i) const throw(){
      return (mpf_cmp_si(data, i) == 0);
    }
    
    
    bool realnumber::operator!=(const signed long int i) const throw(){
      return (mpf_cmp_si(data, i) != 0);
    }
    
    
    bool realnumber::operator>=(const signed long int i) const throw(){
      return (mpf_cmp_si(data, i) >= 0);
    }
    
    
    bool realnumber::operator<=(const signed long int i) const throw(){
      return (mpf_cmp_si(data, i) <= 0);
    }
    
    
    bool realnumber::operator< (const signed long int i) const throw(){
      return (mpf_cmp_si(data, i) < 0);
    }
    
    
    bool realnumber::operator> (const signed long int i) const throw(){
      return (mpf_cmp_si(data, i) > 0);
    }
    
    
    
    realnumber& realnumber::abs() throw(){
      mpf_t sd;
      mpf_init2(sd, mpf_get_prec(data));
      mpf_set(sd, data);
      mpf_abs(data, sd);
      mpf_clear(sd);
      
      return (*this);
    }
    
    
    realnumber& realnumber::ceil() throw(){
      mpf_t sd;
      mpf_init2(sd, mpf_get_prec(data));
      mpf_set(sd, data);
      mpf_ceil(data, sd);
      mpf_clear(sd);
      
      return (*this);
    }
    
    
    realnumber& realnumber::floor() throw(){
      mpf_t sd;
      mpf_init2(sd, mpf_get_prec(data));
      mpf_set(sd, data);
      mpf_floor(data, sd);
      mpf_clear(sd);
      
      return (*this);
    }
    
    
    realnumber& realnumber::trunc() throw(){
      mpf_t sd;
      mpf_init2(sd, mpf_get_prec(data));
      mpf_set(sd, data);
      mpf_trunc(data, sd);
      mpf_clear(sd);
      
      return (*this);
    }
    
    
    // returns sign of real number
    // returns 1 if r > 0, 0 if r == 0 and -1 if r < 0
    int realnumber::sign() const throw(){
      return mpf_sgn(data);
    }
    
    
    double& realnumber::operator[](const unsigned long& index)
      throw(std::out_of_range, illegal_operation){
      
      throw illegal_operation("whiteice::math::realnumber: no subelements");
    }
    
    const double& realnumber::operator[](const unsigned long& index) const
      throw(std::out_of_range, illegal_operation){
      throw illegal_operation("whitece::math::realnumber: no subelements");
    }
    
    
    //////////////////////////////////////////////////
    // conversions
    
    // rounds to closest double
    double realnumber::getDouble() const throw(){
      return mpf_get_d(data);
    }
    
    // returns floor(realnumber) conversion to integer
    // integer realnumber::getInteger() const throw(); ****** TODO *******
    
    // returns human-readable and realnumber(std::string) ctor
    // understandable representation of realnumber
    std::string realnumber::getString(size_t ndigits) const throw()
    {
      mp_exp_t exp_ = 1;
      char* str = mpf_get_str(NULL, &exp_, 10, ndigits, data);
      std::string s;
      
      if(strlen(str)){
	if(mpf_sgn(data) >= 0){
	  
	  if(exp_ < 6 && exp_ >= 0){
	    
	    for(int k=0;k<exp_;k++){
	      s += " ";
	      s[k] = str[k];
	    }
	    
	    if(exp_ == 0)
	      s += "0";
	    
	    s += ".";
	    
	    if(str[exp_] == '\0')
	      s += "0";
	    else
	      s += &(str[exp_]);
	    
	  }
	  else if(exp_ > -4 && exp_ < 0){
	    exp_ = -exp_;
	    
	    s = "0.";
	    
	    for(int k=0;k<exp_;k++)
	      s += "0";
	    
	    s += str;
	    
	  }
	  else{
	    exp_--;
	    
	    s = " .";
	    s[0] = str[0];
	    s += &(str[1]);
	    
	    char buf[32];
	    sprintf(buf, "e%+.2ld", exp_);
	    s += buf;
	    
	  }
	}
	else{ // has negative - sign
	  
	  if(exp_ < 6 && exp_ >= 0){
	    s = "-";
	    
	    for(int k=0;k<exp_;k++){
	      s += " ";
	      s[k+1] = str[k+1];
	    }
	    
	    if(exp_ == 0)
	      s += "0";
	    
	    s += ".";
	    
	    if(str[exp_+1] == '\0')
	      s += "0";
	    else
	      s += &(str[exp_+1]);
	    
	    s += &(str[exp_+1]);
	    
	  }
	  else if(exp_ > -4 && exp_ < 0){
	    exp_ = -exp_;
	    
	    s = "-0.";
	    
	    for(int k=0;k<exp_;k++)
	      s += "0";
	    
	    s += &(str[1]);
	    
	  }
	  else{
	    exp_--;
	    
	    s = "- .";
	    s[1] = str[1];
	    s += &(str[2]);
	    
	    char buf[32];
	    sprintf(buf, "e%+.2ld", exp_);
	    s += buf;
	  }
	}
	
      }
      else
	s = "0";
      
      
      free(str);
      return s;
    }
    
    
    ////////////////////////////////////////////////////////////
    
    
    std::ostream& operator<<(std::ostream& ios,
			     const whiteice::math::realnumber& r)
    {
      ios << r.getString(0);
      
      return ios;
    }
    
  };
};
