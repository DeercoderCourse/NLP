/* * * * *
 *  AzParam.hpp 
 *  Copyright (C) 2011-2014 Rie Johnson
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * * * * */

#ifndef _AZ_PARAM_HPP_
#define _AZ_PARAM_HPP_

#include "AzUtil.hpp"
#include "AzStrPool.hpp"
#include "AzPrint.hpp"
#include "AzTools.hpp"

//! Parse parameters 
#define AzParam_file_mark '@'
class AzParam {
protected:
  const char *param; 
  char dlm, kwval_dlm; 
  AzStrPool sp_used_kw; 
  bool doCheck; /* check unknown/duplicated keywords */
  AzBytArr s_param; 
  const char *pfx; 
  AzBytArr s; /* work */
public:
  AzParam(const char *inp_param, 
          bool inp_doCheck=true, 
          char inp_dlm=',',
          char inp_kwval_dlm='=') : sp_used_kw(100, 30), pfx("")
  {
    param = inp_param; 
    doCheck = inp_doCheck; 
    dlm = inp_dlm; 
    kwval_dlm = inp_kwval_dlm; 
  }
  AzParam(int argc, const char *argv[], 
          bool inp_doCheck=true, 
          char file_mark=AzParam_file_mark, 
          char inp_dlm=',', 
          char inp_kwval_dlm='=',
          char cmt='#') : sp_used_kw(100, 30), pfx("")
  {
    doCheck = inp_doCheck; 
    dlm = inp_dlm; 
    kwval_dlm = inp_kwval_dlm; 
    concat_args(argc, argv, &s_param, file_mark, dlm, cmt); 
    param = s_param.c_str(); 
  }
  AzParam(char inp_dlm, int argc, const char *argv[], 
          bool inp_doCheck=true, 
          char file_mark=AzParam_file_mark, 
          char inp_kwval_dlm='=',
          char cmt='#') : sp_used_kw(100, 30), pfx("") {
    doCheck = inp_doCheck; 
    dlm = inp_dlm; 
    kwval_dlm = inp_kwval_dlm; 
    concat_args(argc, argv, &s_param, file_mark, dlm, cmt); 
    param = s_param.c_str();   
  }

  void reset_prefix(const char *_pfx="") { pfx = _pfx; }
  
  inline const char *c_str() const { return param; }
  bool needs_help() const {
    return (s_param.length() <= 0); 
  }
  inline void swOn(bool *swch, const char *_kw, 
                   bool doCheckKw=true) {
    if (param == NULL) return; 
    const char *kw = cpfx(_kw); 
    if (doCheckKw) {
      if (strstr(_kw, "Dont") == _kw || 
          (strstr(_kw, "No") == _kw && strstr(_kw, "Normalize") == NULL)) {
        throw new AzException("AzParam::swOn", 
                              "On-kw shouldn't begin with \"Dont\" or \"No\"", kw); 
      }
    }
    const char *ptr = pointAfterKw(param, kw); 
    if (ptr != NULL && 
        (*ptr == '\0' || *ptr == dlm)) {
      *swch = true; 
    }
    if (doCheck) sp_used_kw.put(kw); 
  }
  inline void swOff(bool *swch, const char *_kw, 
                    bool doCheckKw=true) {
    if (param == NULL) return; 
    const char *kw = cpfx(_kw);     
    if (doCheckKw) {
      if (strstr(_kw, "Dont") != _kw &&
          (strstr(_kw, "No") != _kw && strstr(_kw, "Normalize") == NULL)) {
        throw new AzException("AzParam::swOff", 
                              "Off-kw should start with \"Dont\" or \"No\"", kw); 
      }
    }
    const char *ptr = pointAfterKw(param, kw); 
    if (ptr != NULL && 
        (*ptr == '\0' || *ptr == dlm) ) {
      *swch = false; 
    }
    if (doCheck) sp_used_kw.put(kw); 
  }
  inline void vStr(const char *_kw, AzBytArr *s) {
    if (param == NULL) return; 
    const char *kw = cpfx(_kw);     
    const char *bp = pointAfterKw(param, kw); 
    if (bp == NULL) return; 
    const char *ep = pointAt(bp, dlm); 
    s->reset(); 
    s->concat(bp, Az64::ptr_diff(ep-bp, "AzParam::vStr")); 
    if (doCheck) sp_used_kw.put(kw); 
  }

  inline void vFloat(const char *_kw, double *out_value) {
    if (param == NULL) return; 
    const char *kw = cpfx(_kw);     
    const char *ptr = pointAfterKw(param, kw); 
    if (ptr == NULL) return; 
    *out_value = atof(ptr); 
    if (doCheck) sp_used_kw.put(kw); 
  }
  inline void vInt(const char *_kw, int *out_value) {
    if (param == NULL) return; 
    const char *kw = cpfx(_kw);     
    const char *ptr = pointAfterKw(param, kw); 
    if (ptr == NULL) return; 
    *out_value = atol(ptr); 
    if (doCheck) sp_used_kw.put(kw); 
  }

  void check(const AzOut &out, AzBytArr *s_unused_param=NULL); 

  static void concat(const AzByte *inp, int len, 
                     AzBytArr *s_out,
                     AzByte dlm=',', 
                     AzByte cmt='#'); 
  static void read(const char *fn, 
                   AzBytArr *s_out, 
                   AzByte dlm=',',
                   AzByte cmt='#');                  
  static void concat_args(int argc, 
                          const char *argv[], 
                          AzBytArr *s_out, /* output */ 
                          char file_mark=AzParam_file_mark, 
                          char dlm=',', 
                          char cmt='#'); 
                   
protected:
  inline const char *pointAfterKw(const char *inp_inp, const char *kw) const {
    const char *ptr = NULL; 
    const char *inp = inp_inp; 
    for ( ; ; ) {
      ptr = strstr(inp, kw); 
      if (ptr == NULL) return NULL; 
      if (ptr == inp || *(ptr-1) == dlm) {
        break; 
      }
      inp = ptr + strlen(kw); 
    }
    ptr += strlen(kw); 
    return ptr; 
  }
  inline static const char *pointAt(const char *inp, const char *kw) {
    const char *ptr = strstr(inp, kw); 
    if (ptr == NULL) return inp + strlen(inp); 
    return ptr; 
  }
  inline static const char *pointAt(const char *inp, char ch) {
    const char *ptr = strchr(inp, ch); 
    if (ptr == NULL) return inp + strlen(inp); 
    return ptr; 
  }

  void analyze(AzStrPool *sp_unused, 
               AzStrPool *sp_kw); 
  const char *cpfx(const char *kw) {
    s.reset(pfx); 
    s.concat(kw); 
    return s.c_str(); 
  }               
}; 

/*-----  throw if input error  ----*/
class AzXi {
public: 
  template <class V> 
  static void throw_if_negative(V val, const char *eyec, const char *valname, const char *pfx="") {
    AzBytArr s; 
    if (val < 0) {
      throw new AzException(AzInputError, eyec, cpfx(valname,pfx,s), "must be non-negative."); 
    }
  }
  template <class V>
  static void throw_if_nonpositive(V val, const char *eyec, const char *valname, const char *pfx="") {
    AzBytArr s; 
    if (val <= 0) {
      throw new AzException(AzInputError, eyec, cpfx(valname,pfx,s), "must be positive."); 
    }
  }
  static void throw_if_empty(const AzBytArr &s, const char *eyec, const char *valname, const char *pfx="") {
    AzBytArr s_wrk; 
    if (s.length() <= 0) {
      throw new AzException(AzInputError, eyec, cpfx(valname,pfx,s_wrk), "must be specified."); 
    }
  } 
  static void throw_if_both_or_neither(const AzBytArr &s0, const AzBytArr &s1, const char *eyec, const char *nm0, const char *nm1, const char *pfx="") {
    if (s0.length() > 0 && s1.length() > 0 || s0.length () <= 0 && s1.length() <= 0) {
      AzBytArr s_wrk; 
      AzBytArr s("Specify either "); s << cpfx(nm0,pfx,s_wrk) << " or " << cpfx(nm1,pfx,s_wrk) << " (not both)."; 
      throw new AzException(AzInputError, eyec, s.c_str()); 
    }
  }

  static void throw_if_both(bool cond, const char *eyec, const char *nm0, const char *nm1, const char *pfx="") {
    if (!cond) return; 
    AzBytArr s_wrk; 
    AzBytArr s("Mutually-exclusive parameters: "); s << cpfx(nm0,pfx,s_wrk) << " and " << cpfx(nm1,pfx,s_wrk); 
    throw new AzException(AzInputError, eyec, s.c_str()); 
  }
   
  static void invalid_input(bool cond, const char *eyec, const char *item, const char *pfx="") {
    if (cond) {
      AzBytArr s("INVALID VALUE: "); s << pfx << item; 
      throw new AzException(AzInputError, eyec, s.c_str()); 
    }
  }
  static void check_input(const char *str, const AzStrPool *sp_okay, const char *eyec, const char *nm) {
    if (sp_okay == NULL) throw new AzException(eyec, "No valid values are given."); 
    if (sp_okay->find_anyway(str) < 0) {
      AzBytArr s("INVALID VALUE: ", nm, str); s.c("  Valid values are: "); 
      for (int ix = 0; ix < sp_okay->size(); ++ix) {
        s.c(sp_okay->c_str(ix)); s.c(" "); 
      }
      throw new AzException(AzInputError, eyec, s.c_str()); 
    }
  }
  static void check_input(const AzBytArr &s_str, const AzStrPool *sp_okay, const char *eyec, const char *nm) {
    check_input(s_str.c_str(), sp_okay, eyec, nm); 
  }
  static void check_input(const char *str, const char *okay1, const char *okay2, 
                          const char *eyec, const char *nm) {  
    AzStrPool sp_okay(10, 10); sp_okay.put(okay1, okay2); 
    check_input(str, &sp_okay, eyec, nm); 
  }  
  static void check_input(const AzBytArr &s_str, const char *okay1, const char *okay2, 
                          const char *eyec, const char *nm) {   
    check_input(s_str.c_str(), okay1, okay2, eyec, nm); 
  }    
protected:   
  inline static const char *cpfx(const char *kw, const char *prefix, AzBytArr &s) {
    s.reset(prefix); 
    s.concat(kw); 
    return s.c_str(); 
  }
};
#endif 

