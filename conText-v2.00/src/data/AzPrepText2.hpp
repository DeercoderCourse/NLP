/* * * * *
 *  AzPrepText2.hpp 
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

#ifndef _AZ_PREP_TEXT2_HPP_
#define _AZ_PREP_TEXT2_HPP_

#include "AzUtil.hpp"
#include "AzSmat.hpp"
#include "AzDmat.hpp"
#include "AzDic.hpp"

/*------------------------------------------------*/
class AzPrepText2 {
protected: 
  AzOut out; 
public:
  AzPrepText2(const AzOut &_out) : out(_out) {}
 
  void gen_regions_unsup(int argc, const char *argv[]) const; 
  void gen_regions_parsup(int argc, const char *argv[]) const;   

protected:
  static void check_milestone(int &milestone, int index, int inc) {
    if (milestone > 0 && index >= milestone) {
      fprintf(stdout, "."); fflush(stdout); 
      milestone += inc; 
    }
  }
  static void finish_milestone(int &milestone) {
    if (milestone > 0) fprintf(stdout, "\n"); 
    milestone = -1; 
  }

  void write_XY(AzDataArr<AzSmat> &am_xy, /* destroyed to save memory */
                int data_num, const AzBytArr &s_batch_id, 
                const char *outnm, const char *xy_ext, 
                const AzDic *xy_dic=NULL, const char *xy_dic_ext=".xtext") const; 
  template <class Vmat> /* Vmat: AzSmatVar | AzSmatcVar */
  int _write_XY(AzDataArr<AzSmat> &am_xy, int data_num, const AzBytArr &s_batch_id, const char *outnm, const char *xy_ext) const; 
  void _write_XY_dic(const AzDic *dic, int x_row_num, const char *nm, const char *ext) const;   
  void gen_X_bow(const AzIntArr &ia_tokno, int dic_sz, 
                 int pch_sz, int pch_step, int padding,           
                 bool do_skip_stopunk, 
                 AzSmat *m_feat, /* output */
                 AzIntArr *ia_pos=NULL) const; /* patch position */
  void gen_X_seq(const AzIntArr &ia_tokno, int dic_sz, 
                 int pch_sz, int pch_step, int padding,  
                 bool do_allow_zero, bool do_skip_stopunk, 
                 AzSmat *m_feat, AzIntArr *ia_pos) const; /* patch position: may be NULL */
  void gen_X_ngram_bow(const AzIntArr &ia_nn, 
                       const AzDataArr<AzIntArr> &aia_tokno, int dic_sz, 
                       int pch_sz, int pch_step, int padding, bool do_allow_zero, 
                       /*---  output  ---*/
                       AzSmat *m_feat, 
                       AzIntArr *ia_pos) const; /* patch position: may be NULL */             
             
  void gen_Y(const AzIntArr &ia_tokno, int dic_sz, const AzIntArr &ia_pos, 
             int xpch_sz, /* patch size used to generate X */
             int min_dist, int max_dist, 
             AzSmat *m_y) const; 
  void gen_Y_nolr(const AzIntArr &ia_tokno, int dic_sz, const AzIntArr &ia_pos, 
                  int xpch_sz, /* patch size used to generate X */
                  int min_dist, int max_dist, 
                  AzSmat *m_y) const; 
  void gen_Y_ngram_bow(const AzIntArr &ia_nn, 
                       const AzDataArr<AzIntArr> &aia_tokno, int dic_sz, 
                       const AzIntArr &ia_pos, 
                       int xpch_sz, /* patch size used to generate X */
                       int min_dist, int max_dist, 
                       bool do_nolr, 
                       AzSmat *m_y) const; 
                             
  void reduce_xy(int min_x, int min_y, AzSmat *m_x, AzSmat *m_y, bool do_keep_one=false) const;  

  void gen_seq_dic(const AzDic *inp_dic, int pch_sz, AzDic *out_dic) const; 
                             
  void check_batch_id(const AzBytArr &s_batch_id) const; 

  /*---  for partially-supervised target  ---*/
  class feat_info { /* just to collect statistics ... */
  public:
    double min_val, max_val, sum, count; 
    feat_info() : min_val(99999999), max_val(-99999999), sum(0), count(0) {}
    void update(const AzIFarr &ifa) {
      if (ifa.size() <= 0) return; 
      min_val = MIN(min_val, ifa.findMin()); 
      max_val = MAX(max_val, ifa.findMax()); 
      count += ifa.size(); 
      sum += ifa.sum(); 
    }
    void show(AzBytArr &s) const {
      if (count <= 0) s << "No info"; 
      s << "min," << min_val << ",max," << max_val << ",avg," << sum/count << ",count," << count; 
    }    
  };  
  void gen_Y_ifeat(int top_num_each, int top_num_total, const AzSmat *m_feat, 
                   const AzIntArr &ia_tokno, const AzIntArr &ia_pos, 
                   int xpch_sz, int min_dist, int max_dist, 
                   bool do_nolr, 
                   int f_pch_sz, int f_pch_step, int f_padding, 
                   AzSmat *m_y, 
                   feat_info fi[2]) const;   
  void set_ifeat(const AzSmat *m_feat, int top_num, 
                 int col, int offs, AzIFarr *ifa_ctx, feat_info fi[2]) const; 
}; 
#endif 
