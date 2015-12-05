/* * * * *
 *  AzpData_.hpp
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

#ifndef _AZP_DATA__HPP_
#define _AZP_DATA__HPP_

#include "AzUtil.hpp"
#include "AzPmat.hpp"
#include "AzPmatApp.hpp"
#include "AzParam.hpp"
#include "AzHelp.hpp"
#include "AzxD.hpp"
#include "AzTextMat.hpp"
#include "AzDic.hpp"

#define AzpData_Ext_xpmat ".xpmat"
#define AzpData_Ext_xsmatvar ".xsmatvar"
#define AzpData_Ext_xsmat ".xsmat"
#define AzpData_Ext_x ".x"
#define AzpData_Ext_y ".y"
#define AzpData_Ext_ypmat ".ypmat"
#define AzpData_Ext_ysmat ".ysmat"
#define AzpData_Ext_ysmatvar ".ysmatvar"

#define AzpData_Ext_xtext ".xtext"

class AzpData_X {
protected: 
  /*---  Only one of these will be set by AzpData_::gen_data  ---*/
  AzPmat md; 
  AzPmatSpa ms; 
  AzPmatVar mv; 
  AzPmatSpaVar msv; 
  void reset_all_but_dense() { ms.reset(); mv.reset(); msv.reset(); }
  void reset_all_but_sparse() { md.reset(); mv.reset(); msv.reset(); }
  void reset_all_but_var() { md.reset(); ms.reset(); msv.reset(); }
  void reset_all_but_spavar() { md.reset(); ms.reset(); mv.reset(); }
public: 
  friend class AzpData_; 
  const AzPmat *dense() const { return &md; }
  const AzPmatSpa *sparse() const { return &ms; }
  const AzPmatVar *var() const { return &mv; }
  const AzPmatSpaVar *spavar() const { return &msv; }
  bool is_dense() const { return (dense()->colNum() > 0); }
  bool is_sparse() const { return (sparse()->colNum() > 0); }
  bool is_var() const { return (var()->dataNum() > 0); }
  bool is_spavar() const { return (spavar()->dataNum() > 0); }
  void separate_columns() { if (msv.dataNum() > 0) msv.separate_columns(); }  
}; 

/*---  For the multi-dataset situation  ---*/
class AzpData_batch_info {
protected: 
  AzIntArr ia_b_pop; /* [i] #data of batch-i */
  AzDataArr<AzIntArr> aia_data_index; /* [i] data index of batch-i */  
public:
  void reset(int batch_num) { ia_b_pop.reset(batch_num, 0); aia_data_index.reset(batch_num); }
  void reset() { ia_b_pop.reset(); aia_data_index.reset(); }
  void reset(const AzpData_batch_info *bi) {
    ia_b_pop.reset(&bi->ia_b_pop); aia_data_index.reset(bi->aia_data_index.size()); 
    for (int ix = 0; ix < aia_data_index.size(); ++ix) aia_data_index(ix)->reset(bi->aia_data_index[ix]);  
  }
  void update(int bx, int data_num, const AzIntArr *ia) {
    ia_b_pop(bx, data_num); aia_data_index(bx)->reset(); 
    if (ia != NULL) aia_data_index(bx)->reset(ia); 
  }
  void check_pop(const AzpData_batch_info *bi, int dsno0, int dsno1) const { 
    if (ia_b_pop.compare(&bi->ia_b_pop) != 0) {
      AzBytArr s0("dataset#"); s0 << dsno0; 
      ia_b_pop.print(log_out, s0.c_str()); 
      AzBytArr s1("dataset#"); s1 << dsno1; 
      bi->ia_b_pop.print(log_out, s1.c_str()); 
      AzX::throw_if(true, AzInputError, "AzpData_batch_info::check_pop", "Conflict in the number of data points."); 
    }
  }
  void check_shape(const AzpData_batch_info *bi, int dsno0, int dsno1) const {
    bool ok = true; 
    if (aia_data_index.size() != bi->aia_data_index.size()) ok = false; 
    else for (int ix = 0; ix < aia_data_index.size(); ++ix) if (aia_data_index[ix]->compare(bi->aia_data_index[ix]) != 0) ok = false; 
    if (ok) return; 
    AzBytArr s("Shape mismatch between dataset#"); s << dsno0 << " and dataset#" << dsno1; 
    AzX::throw_if(true, AzInputError, "AzpData_batch_info::check_shape", s.c_str()); 
  } 
}; 

class AzpData_ {
protected:
  AzOut out; 
  AzBytArr s_nm, s_dir, s_x_ext, s_y_ext; 
  bool is_multicat, is_regression; 
  int batch_num; 
  AzDicc dummy_dic; 

  void copy_paramonly_from(const AzpData_ *inp) {
    s_dir.reset(&inp->s_dir); 
    s_x_ext.reset(&inp->s_x_ext); 
    s_y_ext.reset(&inp->s_y_ext); 
    is_multicat = inp->is_multicat; 
    is_regression = inp->is_regression; 
  }
  virtual AzpData_ *clone_nocopy() const = 0; /* don't copy anything */ 
  
public: 
  AzpData_() : is_multicat(false), is_regression(false), batch_num(1) {}
  virtual ~AzpData_() {}
  friend class AzpDataSet_; 
  
  virtual void resetOut(const AzOut &_out) { out = _out; }
  AzOut quiet() { AzOut myout = out; out.deactivate(); return myout; }
  
  /*------------------------------------------------*/
  #define kw_dir "data_dir="
  #define kw_x_ext "x_ext="
  #define kw_y_ext "y_ext="  
  #define kw_batch_num "num_batches="
  #define kw_is_regression "Regression"
  #define kw_is_multicat "MultiLabel"
  /*------------------------------------------------*/
  virtual void resetParam(AzParam &azp, bool is_training, 
                          bool is_there_y) { /* false if making prediction on new data */
    const char *eyec = "AzpData_::resetParam"; 
    azp.vStr(kw_dir, &s_dir); 
    if (is_training) {
      azp.vInt(kw_batch_num, &batch_num);  
    }
    azp.vStr(kw_x_ext, &s_x_ext); 
    if (is_there_y) azp.vStr(kw_y_ext, &s_y_ext);     
    azp.swOn(&is_regression, kw_is_regression); 
    azp.swOn(&is_multicat, kw_is_multicat); 
    batch_num = MAX(1,batch_num);
    
    AzXi::throw_if_empty(s_x_ext, eyec, kw_x_ext); 
    if (is_there_y) AzXi::throw_if_empty(s_y_ext, eyec, kw_y_ext); 
    AzX::throw_if((is_regression && is_multicat), AzInputError, eyec, "Regression and multicat-categorization are mutually exclusive"); 
    
    resetParam_data(azp, is_training); 
  }
  virtual void resetParam_data(AzParam &azp, bool is_training) = 0; 
  virtual void printParam(const AzOut &out) const {
    if (out.isNull()) return; 
    AzPrint o(out); 
    o.printV(kw_dir, s_dir); 
    o.printV(kw_x_ext, s_x_ext); 
    o.printV(kw_y_ext, s_y_ext);     
    o.printV(kw_batch_num, batch_num); 
    o.printSw(kw_is_regression, is_regression); 
    o.printSw(kw_is_multicat, is_multicat); 
    o.printEnd(); 
    printParam_data(out); 
  }
  virtual void printHelp(AzHelp &h, bool is_training, bool is_there_y) const {
    printHelp_data(h); 

    h.item(kw_dir, "Directory where data files are."); 
    h.item_required(kw_x_ext, "Feature filename extension."); 
    if (is_there_y) h.item_required(kw_y_ext, "Target filename extension."); 
    /* kw_batch_num */
  }
  virtual void printParam_data(const AzOut &out) const = 0;   /* data-type-specific parameters */
  virtual void printHelp_data(AzHelp &h) const = 0;  /* help for data-type-specific parameters */
  virtual void reset_data(const AzOut &_out, const char *nm, int _dummy_ydim=-1, 
                          AzpData_batch_info *bi=NULL) = 0; 

  bool isRegression() const { return is_regression; }
  bool isMulticat() const { return is_multicat; }
  
  virtual void first_batch() {}
  virtual void next_batch() {}
  virtual int batchNum() const { return batch_num; }
  virtual int dataNum_total() const = 0; 
  virtual bool is_vg_x() const = 0;
  virtual bool is_vg_y() const { return false; }
  virtual bool is_sparse_x() const = 0; 
  virtual bool is_sparse_y() const = 0; 
  
  virtual void reset() = 0; 
  virtual void destroy() { batch_num = 0; }
  virtual int classNum() const = 0; 
  virtual int dataNum() const = 0; 
  virtual int outNum() const { return dataNum(); }
  virtual const AzIntArr *dataIndex() const { return NULL; }
  virtual int featNum() const = 0; 
  virtual int channelNum(int data_no=0) const = 0; 
  virtual int datasetNum() const { return 1; }
  virtual int dimensionality() const = 0; 
  virtual int size(int index) const = 0;  
  
  virtual const AzDicc &get_dic(int dataset_no=0) const { return dummy_dic; } /* never return NULL */
  virtual void gen_data(const int *dxs, int d_num, AzpData_X *data_x, 
                        int dataset_no=0) const {
    if (is_vg_x()) {
      if (is_sparse_x()) { /* sparse variable-sized */
        data_x->reset_all_but_spavar(); 
        _gen_data(dxs, d_num, &data_x->msv, dataset_no); 
      }
      else {
        data_x->reset_all_but_var(); /* dense variable-sized */
        _gen_data(dxs, d_num, &data_x->mv, dataset_no);    
      }
    }
    else {
      if (is_sparse_x()) { /* sparse fixed-sized */
        data_x->reset_all_but_sparse(); 
        _gen_data(dxs, d_num, &data_x->ms, dataset_no); 
      }
      else {
        data_x->reset_all_but_dense(); /* dense fixed-size */
        _gen_data(dxs, d_num, &data_x->md, dataset_no); 
      }
    }
  }

  virtual void gen_data(int dx_begin, int d_num, AzpData_X *data_x, 
                        int dataset_no=0) const {
    AzIntArr ia_dxs; ia_dxs.range(dx_begin, dx_begin+d_num); 
    gen_data(ia_dxs.point(), ia_dxs.size(), data_x, dataset_no);     
  }
  
  virtual void gen_targets(const int *dxs, int d_num, AzPmat *m_y) const = 0;   
  virtual void gen_targets(int dx_begin, int d_num, AzPmat *m_y) const {
    AzIntArr ia_dxs; ia_dxs.range(dx_begin, dx_begin+d_num); 
    gen_targets(ia_dxs.point(), ia_dxs.size(), m_y); 
  }
  virtual double min_target() const = 0; 
  virtual double max_target() const = 0; 

  virtual void gen_membership(AzDataArr<AzIntArr> &aia_cl_dxs) const = 0;     

  virtual void gen_dataweights(const int *dxs, int d_num, AzPmat *m_dw) const { no_support("gen_dataweights"); }
  virtual void gen_dataweights(int dx_begin, int d_num, AzPmat *m_dw) const { 
    AzIntArr ia_dxs; ia_dxs.range(dx_begin, dx_begin+d_num); 
    gen_dataweights(ia_dxs.point(), ia_dxs.size(), m_dw); 
  }
  virtual void gen_targets(const int *dxs, int d_num, AzSmat *m_y) const { no_support("gen_targets(smat)"); }
  virtual void gen_targets(int dx_begin, int d_num, AzSmat *m_y) const {
    AzIntArr ia_dxs; ia_dxs.range(dx_begin, dx_begin+d_num); 
    gen_targets(ia_dxs.point(), ia_dxs.size(), m_y);  
  }
    
  virtual void gen_targets(const int *dxs, int d_num, AzPmatSpa *m_y) const { no_support("gen_targets(sparse)"); }
  virtual void gen_targets(int dx_begin, int d_num, AzPmatSpa *m_y) const {
    AzIntArr ia_dxs; ia_dxs.range(dx_begin, dx_begin+d_num); 
    gen_targets(ia_dxs.point(), ia_dxs.size(), m_y); 
  }    
  virtual AzDvect *base() const { return NULL; }
  
  /*-------------------------------------------------------------------*/
  virtual void get_info(AzxD *data_dim) const {
    AzIntArr ia_sz; 
    int dim = dimensionality(); 
    int ix;
    for (ix = 0; ix < dim; ++ix) {
      ia_sz.put(size(ix)); 
    }
    data_dim->reset(&ia_sz); 
  }
 
  /*----------------------------------------------------------------------*/
  virtual void signature(AzBytArr *s) const {
    s->reset(); 
    s->c("dim:"); s->cn(dimensionality()); s->c(";"); 
    s->c("channel:"); s->cn(channelNum()); s->c(";"); 
    int ix;
    for (ix = 0; ix < dimensionality(); ++ix) {
      s->c("size"); s->cn(ix); s->c(":"); s->cn(size(ix)); s->c(";"); 
    }
  }

  /*----------------------------------------------------------------------*/
  virtual bool isSignatureCompatible(const AzBytArr *s_nn, const AzBytArr *s_data) const {
    if (s_nn->compare(s_data) == 0) return true; 
    AzStrPool sp1(10,10), sp2(10,10); 
    AzTools::getStrings(s_nn->c_str(), ';', &sp1); 
    AzTools::getStrings(s_data->c_str(), ';', &sp2); 
    if (sp1.size() != sp2.size()) {
      return false; 
    }
    int ix; 
    for (ix = 0; ix < sp1.size(); ++ix) {
      if (strcmp(sp1.c_str(ix), sp2.c_str(ix)) != 0) {
        AzBytArr s1; sp1.get(ix, &s1); 
        AzBytArr s2; sp2.get(ix, &s2); 
        if (s1.beginsWith("size") && s2.beginsWith("size") && s1.endsWith("-1")) {
          /*---  nn is trained for variable-size input, and data is fixed-size ---*/
          continue; 
        }
        else return false; 
      }
    }
    return true; 
  }

protected:  
  virtual void gen_kw(int extno, const char *kw, AzBytArr &s_kw) const {
    if (AzBytArr::endsWith(kw, "=")) s_kw.reset((AzByte *)kw, Az64::cstrlen(kw)-1); 
    else                             s_kw.reset(kw); 
    s_kw << extno << "="; 
  } 
  /*----------------------------------------------------------------------*/  
  template <class M> /* M: AzDmatc | AzSmatc */
  static void readY_text(const char *fn, int dummy_ydim, int data_num, M *m_y) {  
    if (dummy_ydim > 0) {
      m_y->reform(dummy_ydim, data_num); 
      return; 
    } 
    AzTextMat::readMatrix(fn, m_y); 
    AzX::throw_if((m_y->colNum() != data_num), AzInputError, "AzpData_::readY_text", 
                            "#data mismatch btw target and feature files"); 
  }
  /*----------------------------------------------------------------------*/  
  template <class M>  /* M: AzPmat | AzPmatSpa | AzSmat */
  void readY_bin(const char *fn, int dummy_ydim, int data_num, 
                         M *m_y) {  
    if (dummy_ydim > 0) {
      AzTimeLog::print("No target ... ", out); 
      m_y->reform(dummy_ydim, data_num); 
      return; 
    }
    m_y->read(fn); 
    AzX::throw_if((m_y->colNum() != data_num), AzInputError, "AzpData_::readY_bin", 
                            "#data mismatch btw target and feature files"); 
  }  
  template <class M> /* M: AzSmatc | AzDmatc */
  static void _gen_membership(const M &m, AzDataArr<AzIntArr> &aia_cl_dxs) {
    aia_cl_dxs.reset(m.rowNum());  /* # of classes */
    for (int dx = 0; dx < m.colNum(); ++dx) {
      int cl; m.first_positive(dx, &cl); 
      AzX::throw_if((cl < 0), "AzpData_::_gen_membership", 
         "Failed to determine membership.  This data may not be for classification"); 
      aia_cl_dxs(cl)->put(dx);       
    }
  }

  /*------------------------------------------*/  
  /* directory/name.(xsmat|ypmatspa)[.batch#] */
  virtual const char *gen_batch_fn(int batch_no, const char *ext, AzBytArr *s) {
    s->reset(&s_dir); 
    if (s->length() > 0) (*s) << "/"; 
    (*s) << s_nm.c_str() << ext; 
    if (batch_num ==1) return s->c_str(); 
    (*s) << "." << batch_no+1 << "of" << batch_num; 
    return s->c_str(); 
  }  
  virtual const char *gen_xtext_fn(AzBytArr *s) {
    s->reset(&s_dir);  
    if (s->length() > 0) (*s) << "/"; 
    (*s) << s_nm.c_str() << AzpData_Ext_xtext; 
    return s->c_str();     
  }
  void no_support(const char *who, const char *nm) const { AzX::no_support(true, who, nm); }
  void no_support(const char *nm) const { AzX::no_support(true, "AzpData_", nm); }
  
  /*---  a derived class should override these.  ---*/
  virtual void _gen_data(const int *dxs, int d_num, AzPmat *m_data, int dsno) const = 0; 
  virtual void _gen_data(const int *dxs, int d_num, AzPmatSpa *m_data, int dsno) const = 0; 
  virtual void _gen_data(const int *dxs, int d_num, AzPmatSpaVar *m_data, int dsno) const = 0; 
  virtual void _gen_data(const int *dxs, int d_num, AzPmatVar *m_data, int dsno) const = 0; 
};    

/*---  data sets for training and testing  ---*/
class AzpDataSet_ {
protected: 
  AzBytArr s_typ; 
  AzBytArr s_trnnm; 
  AzStrPool sp_tstnm; 
  AzpData_ *trn, *tst, *tst2; /* pointers obtained by clone; they need delete. */
public: 
  AzpDataSet_() : trn(NULL), tst(NULL), tst2(NULL) {}
  ~AzpDataSet_() { 
    reset(); 
  }
  AzpData_ *trn_data() { return trn; } /* not const to go over batches */
  const AzpData_ *tst_data() { return tst; }
  const AzpData_ *tst_data2() { return tst2; }
  void reset() {
    delete trn; trn = NULL; 
    delete tst; tst = NULL; 
    delete tst2; tst2 = NULL;   
    s_trnnm.reset(); sp_tstnm.reset(); 
  }
  
  #define kw_datatype "datatype="
  #define kw_trnnm "trnname="
  #define kw_tstnm "tstname="
  #define kw_tstnm2 "tstname2="  
  
  #define help_trnnm "Filename of the training files without extension."
  #define help_tstnm "Filename of the test files without extension."
  #define help_tstnm2 "Filename of the second set of test files without extension."
  void resetParam(AzParam &azp,bool do_train, bool do_test, bool is_there_y) {
    reset(); 
    const char *eyec = "AzpDataSet_::resetParam"; 
    azp.vStr(kw_datatype, &s_typ); 
    AzXi::throw_if_empty(s_typ, eyec, kw_datatype); 
    if (do_train) {
      azp.vStr(kw_trnnm, &s_trnnm); 
      AzXi::throw_if_empty(s_trnnm, eyec, kw_trnnm);
      trn = ptr(s_typ)->clone_nocopy(); 
      bool is_trn_data = true; 
      trn->resetParam(azp, is_trn_data, is_there_y);       
    }
    if (do_test)  {
      AzBytArr s_tstnm; 
      azp.vStr(kw_tstnm, &s_tstnm);
      AzXi::throw_if_empty(s_tstnm, eyec, kw_tstnm); 
      AzByte dlm = '+'; 
      AzTools::getStrings(s_tstnm.point(), s_tstnm.length(), dlm, &sp_tstnm); 
      AzX::throw_if((sp_tstnm.size() > 2), AzInputError, eyec, kw_tstnm, "can only have two dataset names delimited by +."); 
      bool is_trn_data = false; 
      tst = ptr(s_typ)->clone_nocopy(); 
      tst->resetParam(azp, is_trn_data, is_there_y); 
      if (sp_tstnm.size() > 1) {
        tst2 = ptr(s_typ)->clone_nocopy(); 
        tst2->resetParam(azp, is_trn_data, is_there_y);     
      }      
    }    
  }
  void printParam(const AzOut &out) const {
    AzPrint o(out); 
    o.printV(kw_datatype, s_typ); 
    o.printV_if_not_empty(kw_trnnm, s_trnnm); 
    for (int ix = 0; ix < sp_tstnm.size(); ++ix) {
      if (ix == 0) o.printV(kw_tstnm, sp_tstnm.c_str(ix)); 
      else         o.printV(kw_tstnm2, sp_tstnm.c_str(ix)); 
    }
    if (trn != NULL) {
      trn->printParam(out); 
      return; 
    }    
    if (tst != NULL) {
      tst->printParam(out); 
    }
  }
  void printHelp(AzHelp &h, bool do_train, bool do_test, bool is_there_y) const {
    if (do_train) {
      h.item_required(kw_trnnm, "Training data name."); 
      h.item_required(kw_tstnm, "Test data name.  Required for testing unless \"NoTest\" is specified."); 
    }
    else {
      h.item_required(kw_tstnm, "Test data name.");       
    }
  }

  /*---  call this after resetParam  ---*/  
  void reset_data(const AzOut &out, int dummy_ydim=-1) {
    if (trn != NULL) trn->reset_data(out, s_trnnm.c_str(), dummy_ydim); 
    if (tst != NULL) tst->reset_data(out, sp_tstnm.c_str(0), dummy_ydim); 
    if (tst2 != NULL) tst2->reset_data(out, sp_tstnm.c_str(1), dummy_ydim);       
  }

protected: 
  virtual const AzpData_ *ptr(const AzBytArr &s_typ) const = 0;   
 
}; 
#endif 