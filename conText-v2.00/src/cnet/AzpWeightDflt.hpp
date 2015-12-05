/* * * * *
 *  AzpWeightDflt.hpp
 *  Copyright (C) 2014-2015 Rie Johnson
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

#ifndef _AZP_WEIGHT_DFLT_HPP_
#define _AZP_WEIGHT_DFLT_HPP_

#include "AzpWeight_.hpp"
#include "AzpLmSgd.hpp"
#include "AzpLocalw.hpp"

/**********************************************************/  
class AzpWeightDflt : public virtual AzpWeight_ {
protected: 
  AzDataArr<AzpLmSgd> lmods_sgd; 
  AzBaseArr<AzpLm *> lms; 
  AzBaseArr<const AzpLm *> lms_const;   
  AzpLmParam p; 
  AzpLmSgd_Param ps, org_ps; 
  bool is_localw; 
  AzpLocalw lw; 
  bool do_thru; /* Don't apply weights.  Set input to uotput as it is. */
  int thru_dim; 
  
  virtual void reset_lms() {
    lms_const.realloc(lmods_sgd.size(), "AzpWeightDflt::reset_lms", "lms_const"); 
    lms.realloc(lmods_sgd.size(), "AzpWeightDflt::reset_lms", "lms");    
    for (int lx = 0; lx < lmods_sgd.size(); ++lx) {
      lms_const(lx, lmods_sgd[lx]); 
      lms(lx, lmods_sgd(lx)); 
    }
  }

  virtual AzpLm *lm() { return lms[0]; }  
  virtual const AzpLm *lm_const() const { return lms_const[0]; }
  virtual const AzpLm *templ() const {
    AzX::throw_if((lms_const.size() <= 0), "AzpWeightDflt::templ", "No tempalte"); 
    return lms_const[0]; 
  }
  static const int version = 0; 
/*  static const int reserved_len = 64;  */
  static const int reserved_len = 59;  /* 04/13/2015: do_thru(1) and thru_dim(4) */
  virtual void read_common(AzFile *file) {  
    AzTools::read_header(file, reserved_len);  
    do_thru = file->readBool(); /* 04/13/2015 */
    thru_dim = file->readInt(); /* 04/13/2015 */
    is_localw = file->readBool();  
  }    
  virtual void write_common(AzFile *file) {
    AzTools::write_header(file, version, reserved_len); 
    file->writeBool(do_thru); /* 04/13/2015 */
    file->writeInt(thru_dim); /* 04/13/2015 */
    file->writeBool(is_localw); 
  }  

public: 
  AzpWeightDflt() : is_localw(false), do_thru(false), thru_dim(-1) {}  
  
  virtual bool are_weights_fixed() const {
    return p.dont_update(); 
  }
  
  virtual void reset(int loc_num, /* used only when is_localw is true */
             int w_num, int inp_dim, bool is_spa, bool is_var) {
    const char *eyec = "AzpWeightDflt::reset"; 
    if (do_thru) {
      AzX::no_support(is_spa, eyec, "The thru option with sparse input"); 
      force_thru(inp_dim); 
      AzX::throw_if((w_num != thru_dim), eyec, "With the thru option, the dimensionality of input and output must be the same"); 
      return; 
    }
    AzX::no_support((is_localw && (is_spa || is_var)), eyec, "local weights with sparse or variable-sized input"); 
    int sz = (!is_localw) ? 1 : loc_num; 
    lmods_sgd.reset(sz); 
    for (int lx = 0; lx < lmods_sgd.size(); ++lx) ((AzpLm *)lmods_sgd(lx))->reset(w_num, p, inp_dim);
    reset_lms();    
  }
  virtual void force_thru(int inp_dim) {
    do_thru = true; 
    lmods_sgd.reset(); 
    reset_lms(); 
    thru_dim = inp_dim; 
  } 
  virtual void setup_for_reg_L2init() {
    for (int lx = 0; lx < lms.size(); ++lx) lms[lx]->setup_for_reg_L2init(p); 
  }
  virtual void check_for_reg_L2init() const {
    for (int lx = 0; lx < lms.size(); ++lx) lms[lx]->check_for_reg_L2init(p); 
  }  
  virtual AzpWeight_ *clone() const {
    AzpWeightDflt *o = new AzpWeightDflt();    
    o->lmods_sgd.reset(&lmods_sgd); 
    o->reset_lms(); 
    o->p = p; 
    o->ps = ps; 
    o->org_ps = org_ps; 
    o->is_localw = is_localw;    
    o->lw.reset(&lw); 
    o->do_thru = do_thru; 
    o->thru_dim = thru_dim; 
    return o; 
  }   
  virtual void read(AzFile *file) {  
    read_common(file); 
    lmods_sgd.read(file); reset_lms(); 
  }
  virtual void write(AzFile *file) {
    write_common(file); 
    lmods_sgd.write(file); 
  }      
  
  /*------------------------------------------------------------*/   
  #define kw_do_thru "ThruWeights"
  #define kw_is_localw "LocalWeights"
  /*------------------------------------------------------------*/ 
  virtual void _resetParam(AzParam &azp, const char *pfx, bool is_warmstart=false) {
    if (!is_warmstart) {
      azp.reset_prefix(pfx); 
      azp.swOn(&do_thru,  kw_do_thru); 
      azp.swOn(&is_localw, kw_is_localw); 
      azp.reset_prefix(""); 
    }    
    p.resetParam(azp, pfx, is_warmstart); 
    ps.resetParam(azp, pfx, is_warmstart); 
    org_ps = ps; /* save the original parameters */
  }
  virtual void printParam(const AzOut &out, const char *pfx) const {
    AzPrint o(out, pfx);
    o.printSw(kw_do_thru, do_thru); 
    o.printSw(kw_is_localw, is_localw);   
    if (!do_thru) {
      p.printParam(out, pfx); 
      ps.printParam(out, pfx); 
    }
  }
  virtual void printHelp(AzHelp &h) const { 
    /* kw_is_localw */
    p.printHelp(h); 
    ps.printHelp(h); 
  }
  virtual void resetParam(AzParam &azp, const char *pfx_dflt, const char *pfx, bool is_warmstart=false) {
    _resetParam(azp, pfx_dflt, is_warmstart); 
    _resetParam(azp, pfx, is_warmstart); 
    if (!do_thru) {
      p.checkParam(pfx); 
      ps.checkParam(pfx); 
    }
  }  
  
  /*---  do something about parameters  ---*/
  virtual void reset_do_no_intercept(bool flag) {
    p.do_no_intercept = flag; 
  }  
  virtual void multiply_to_stepsize(double factor, const AzOut *out=NULL) {
    ps.eta = org_ps.eta * factor; 
    if (out != NULL) {
      AzBytArr s("eta="); s << ps.eta; 
      AzPrint::writeln(*out, s); 
    }
  }
  virtual void set_momentum(double newmom, const AzOut *out=NULL) {
    ps.momentum = newmom; 
    if (out != NULL) {
      AzBytArr s("momentum="); s << ps.momentum; 
      AzPrint::writeln(*out, s); 
    }
  }
  
  virtual void force_no_intercept() {
    p.do_no_intercept = true; 
    p.do_reg_intercept = false; 
  }
 
  virtual void initWeights() {
    for (int lx = 0; lx < lms.size(); ++lx) lms[lx]->initWeights(p); 
  }
  virtual void initWeights(const AzpLm *inp, double coeff) {
    if (do_thru) return; 
    check_localw("initWeights(lmod,coeff)");    
    lm()->reset(p, inp, coeff); 
  } 
  virtual void upward(bool is_test, const AzPmat *m_x, AzPmat *m_out) {
    if (do_thru) {
      m_out->set(m_x); 
      return;      
    }
    m_out->reform_noinit(classNum(), m_x->colNum()); 
    if (!is_localw) lm()->apply(m_x, m_out); 
    else            lw.apply(lms, m_x, m_out); 
  }
  virtual void upward(bool is_test, const AzPmatSpa *m_x, AzPmat *m_out) {
    AzX::no_support(do_thru, "AzpWeightDflt::upward(spa)", "The thru option with sparse input"); 
    check_localw("upward with sparse input"); 
    lm()->apply(m_x, m_out);
  }
  virtual void downward(const AzPmat *m_lossd, AzPmat *m_d) const {
    if (do_thru) {
      m_d->set(m_lossd); 
      return;       
    }
    m_d->reform_noinit(get_dim(), m_lossd->colNum()); 
    if (!is_localw) lm_const()->unapply(m_d, m_lossd); 
    else            lw.unapply(lms, m_d, m_lossd);  
  }  
  virtual void updateDelta(int d_num, const AzPmat *m_x, const AzPmat *m_lossd) {             
    if (do_thru) return; 
    if (!is_localw) lm()->updateDelta(d_num, p, m_x, m_lossd); 
    else            lw.updateDelta(lms, p, d_num, m_x, m_lossd);              
  }
  virtual void updateDelta(int d_num, const AzPmatSpa *m_x, const AzPmat *m_lossd) {             
    if (do_thru) return; 
    check_localw("updateDelta with sparse input");    
    lm()->updateDelta(d_num, p, m_x, m_lossd); 
  }
  virtual void flushDelta() {
    for (int lx = 0; lx < lmods_sgd.size(); ++lx) lmods_sgd(lx)->flushDelta(p, ps); 
  }

  virtual void end_of_epoch() {
    for (int lx = 0; lx < lms.size(); ++lx) lms[lx]->end_of_epoch(p); 
  }

  /*------------------------------------------------------------*/    
  virtual double regloss(double *iniloss) const {
    double loss = 0; 
    for (int lx = 0; lx < lms_const.size(); ++lx) loss += lms_const[lx]->regloss(p, iniloss); 
    return loss; 
  }
  virtual int num_weights() const {
    int num = 0; 
    for (int lx = 0; lx < lms_const.size(); ++lx) {   
      num += lms_const[lx]->get_dim() * lms_const[lx]->classNum(); 
    }
    return num; 
  }  
  virtual void show_stat(AzBytArr &s) const {
    if (lms_const.size() <= 0) return; 
    if (!is_localw) {
      s.c("cnv/fc:"); 
      lm_const()->show_stat(s); 
    }
    else {
      s.c("local:"); 
      for (int lx = 0; lx < lms_const.size(); ++lx) {    
        if (lx == 0 || lx == lms_const.size()-1) {
          s.c("lmods["); s.cn(lx); s.c("]:"); 
          lms_const[lx]->show_stat(s); 
        }
        else s.c("."); 
      }    
    }
  }
  virtual int get_dim() const { 
    if (do_thru) return thru_dim; 
    return templ()->get_dim(); 
  }
  virtual int classNum() const { 
    if (do_thru) return thru_dim; 
    return templ()->classNum(); 
  }

  virtual const AzpLm *linmod() const {
    check_thru("linmod"); 
    check_localw("linmod"); 
    return lm_const(); 
  }
  virtual AzpLm *linmod_u() {
    check_thru("linmod_u");    
    check_localw("linmod_u"); 
    return lm(); 
  }  
  virtual bool isLocalW() const {
    return is_localw; 
  }
  
protected:    
  virtual void check_localw(const char *msg) const {
    AzX::throw_if(is_localw, "AzpWeightDflt::check_local", msg, "cannot be used for local weights"); 
  }
  virtual void check_thru(const char *msg) const {
    AzX::throw_if(do_thru, "AzpWeightDflt::check_thru", msg, "cannot be used with the thru option"); 
  }
}; 

#endif 