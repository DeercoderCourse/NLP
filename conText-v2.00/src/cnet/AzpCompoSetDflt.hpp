/* * * * *
 *  AzpCompoSetDflt.hpp
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

#ifndef _AZP_COMPO_SET_DFLT_HPP_
#define _AZP_COMPO_SET_DFLT_HPP_

#include "AzpCompoSet_.hpp"
#include "AzpLossDflt.hpp"
#include "AzpPatchDflt.hpp"
#include "AzpPatchDflt_var.hpp"
#include "AzpWeightDflt.hpp"
#include "AzpActivDflt.hpp"
#include "AzpDropoutDflt.hpp"
#include "AzpPoolingDflt.hpp"
#include "AzpPoolingDflt_var.hpp"
#include "AzpResNormDflt.hpp"

#include "AzpWeight_adad.hpp"
#include "AzpLmAdaD.hpp" /* AdaDelta */

class AzpCompoSetDflt : public virtual AzpCompoSet_ {
protected: 
  AzpLossDflt loss_dflt; 
  AzpPatchDflt patch_dflt; 
  AzpPatchDflt_var patch_var_dflt; 
  AzpDropoutDflt dropout_dflt; 
  AzpPoolingDflt pool_dflt; 
  AzpPoolingDflt_var pool_var_dflt; 
  AzpResNormDflt resnorm_dflt; 
  AzpActivDflt activ_dflt; 
  
  AzpWeightDflt weight_dflt; 
  AzpWeight_adad weight_adad; 

  AzpWeight_ *p_weight; 
  
public:   
  AzpCompoSetDflt() : p_weight(&weight_dflt) {}
  
  #define kw_do_adadelta "AdaDelta"
  void reset(AzParam &azp) {
    /*---  weight optimization  ---*/
    bool do_adadelta = false, do_adadelta_avg = false; 
    azp.swOn(&do_adadelta, kw_do_adadelta);    
    p_weight = (do_adadelta) ? &weight_adad : &weight_dflt; 
  }
  
  virtual const AzpLoss_ *loss() const { return &loss_dflt; }
  virtual const AzpPatch_ *patch() const { return &patch_dflt; }
  virtual const AzpPatch_var_ *patch_var() const { return &patch_var_dflt; }
  virtual const AzpDropout_ *dropout() const { return &dropout_dflt; }
  virtual const AzpPooling_ *pool() const { return &pool_dflt; }
  virtual const AzpPooling_var_ *pool_var() const { return &pool_var_dflt; }
  virtual const AzpResNorm_ *resnorm() const { return &resnorm_dflt; }
  virtual const AzpActiv_ *activ() const { return &activ_dflt; }
  
  virtual const AzpWeight_ *weight() const { return p_weight; }
};   
#endif 