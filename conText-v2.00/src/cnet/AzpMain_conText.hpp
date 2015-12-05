/* * * * *
 *  AzpMain_conText.hpp
 *  Copyright (C) 2015 Rie Johnson
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

#ifndef _AZP_MAIN_CONTEXT_HPP_
#define _AZP_MAIN_CONTEXT_HPP_

#include <time.h>
#include "AzUtil.hpp"
#include "AzPrint.hpp"
#include "AzTextMat.hpp"
#include "AzpDataSetDflt.hpp"
#include "AzParam.hpp"
#include "AzpCompoSetDflt.hpp"

#include "AzpCNet3.hpp"
#include "AzpCNet3_multi.hpp"
#include "AzpCNet3_multi_side.hpp"

/* "ct" for ConText */
class AzpMain_conText {
protected:
  AzpCompoSetDflt cs; 
  AzpCompoSet_ *compo_set(AzParam &azp) {
    cs.reset(azp); 
    return &cs; 
  }
  
public:
  AzpMain_conText() {} 

  void cnn(int argc, const char *argv[]); 
  void cnn_predict(int argc, const char *argv[]); 
  void write_embedded(int argc, const char *argv[]); 
  void write_word_mapping(int argc, const char *argv[]); 
  void adapt_word_vectors(int argc, const char *argv[]); 
  void write_features(int argc, const char *argv[]); 
  
protected:
  AzpCNet3 *choose_cnet(const AzBytArr &s_cnet_ext, AzpCNet3 *dflt, AzpCNet3_multi *multi, 
                        AzpCNet3_multi_side *multi_side, bool dont_throw=false) {
    if      (s_cnet_ext.length() <= 0)   return dflt; 
    else if (s_cnet_ext.equals("multi")) return multi;    
    else if (s_cnet_ext.equals("multi_side")) return multi_side; 
    else {
      if (dont_throw) return dflt; 
      throw new AzException("AzpMain_conText::choose_cnet", "Unknown extension: ", s_cnet_ext.c_str());       
    }
  }  
  
  void show_elapsed(const AzOut &out, clock_t clocks) const; 
  AzOut *reset_eval(const AzBytArr &s_eval_fn, AzOfs &ofs, AzOut &eval_out);                                  
  
  bool printHelp_cnn_if_needed(const AzOut &out, AzParam &azp); 
  bool printHelp_cnn_predict_if_needed(const AzOut &out, AzParam &azp); 
  bool printHelp_write_features_if_needed(const AzOut &out, AzParam &azp); 
  bool printHelp_write_embedded_if_needed(const AzOut &out, AzParam &azp);   
  void zerovec_to_randvec(double wvec_rand_param, AzDmat &m_wvec) const; 
  void read_word2vec_bin(const char *fn, AzDic &dic, AzDmat &m_wvec, bool do_ignore_dup); 
}; 
#endif
