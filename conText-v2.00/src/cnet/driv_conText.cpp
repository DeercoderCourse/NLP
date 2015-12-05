#define _AZ_MAIN_
#include "AzUtil.hpp"
#include "AzpMain_conText.hpp"
#include "AzpEv.hpp"

extern AzPdevice dev; 

/*-----------------------------------------------------------------*/
void help()
{
#ifdef __AZ_GPU__  
  cout << "Arguments:  gpu#[:memsz]  action  parameters" <<endl; 
  cout << "   gpu#: Specify the GPU to be used.  \"-1\": use the default GPU." << endl; 
  cout << "   memsz (optional): If specified and positive, the GPU device memory handler" << endl; 
  cout << "         is enabled with pre-allocated memory of this amount.  Unit: GB." << endl; 
#else
  cout << "Arguments:  action  parameters" <<endl; 
#endif 
  cout << "   action: cnn | cnn_predict | write_features | adapt_word_vectors"<<endl; 
  cout << endl; 
  cout << "Enter, for example, \"conText -1 cnn\" to print help for a specific action." << endl; 
}
 
/*******************************************************************/
/*     main                                                        */
/*******************************************************************/
int main(int argc, const char *argv[]) 
{
  AzException *stat = NULL; 

#ifdef __AZ_GPU__    
  if (argc < 3) {
    help(); 
    return -1; 
  }
  const char *gpu_param = argv[1]; 
  for (int ix = 2; ix < argc; ++ix) {
    argv[ix-1] = argv[ix]; 
  }
  --argc; 
#else
  if (argc < 2) {
    help(); 
    return -1; 
  }
  const char *gpu_param = ""; /* anything */
#endif  

  int gpu_dev = dev.setDevice(gpu_param); 
  if (gpu_dev < 0) {
    AzPrint::writeln(log_out, "Using CPU ... "); 
  }
  else {
    AzPrint::writeln(log_out, "Using GPU#", gpu_dev); 
  }  
  
  int ret = 0; 
  const char *action = argv[1]; 
  AzBytArr s_action(action); 
  try {
    Az_check_system2_(); 

    AzpEv ev; 
    AzpMain_conText driver; 
    if      (s_action.equals("cnn"))         driver.cnn(argc-2, argv+2); 
    else if (s_action.equals("cnn_predict")) driver.cnn_predict(argc-2, argv+2);    
    else if (s_action.equals("write_word_mapping")) driver.write_word_mapping(argc-2, argv+2);   
    else if (s_action.equals("write_embedded")) driver.write_embedded(argc-2, argv+2);     
    else if (s_action.equals("adapt_word_vectors")) driver.adapt_word_vectors(argc-2, argv+2);  
    else if (s_action.equals("write_features")) driver.write_features(argc-2, argv+2);     
    /*---  for thresholding for multi-label categorization  ---*/
    else if (s_action.equals("optimize_thresholds_all")) ev.opt_th_all(argc-2, argv+2); 
    else if (s_action.equals("optimize_thresholds_cv2")) ev.opt_th_cv2(argc-2, argv+2); 
    else {
      help(); 
      ret = -1; 
      goto labend; 
    }
  }
  catch (AzException *e) {
    stat = e; 
  }
  if (stat != NULL) {
    cout << stat->getMessage() << endl; 
    ret = -1; 
    goto labend;  
  }

labend:
  dev.closeDevice();   
  return ret; 
}

