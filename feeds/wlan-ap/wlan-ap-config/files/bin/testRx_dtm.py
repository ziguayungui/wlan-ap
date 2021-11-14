# -*- coding: utf-8-*-
#by tianlin@ci-g.com

import dtm
import time
import sys

def main():
  mode=sys.argv[1]	
  channel=sys.argv[2]
  status=sys.argv[3]
  COM_1 = "/dev/ttyMSM1"
  print(COM_1)
  setup = dict ()
  setup['TestSerialPortName'] = COM_1
  setup['GoldenSerialPortName'] = ""	
  setup['Frequency'] = 1				
  setup['Bitpattern'] = 0		
  setup['Length'] = 1					
  setup['Runtime'] = 100					
  setup['PERLit'] = 30					
  setup['debug'] = True	
  setup['Dbm'] = 0					
  setup['PhyMode'] = 1	
  dtmObject = dtm.DTM(setup)

  dtmObject.frequency = int(channel)
  dtmObject.bytepattern = int(mode)
      
  #Run tests on a single channel
  print('Running tx tests on mode %s channel %s' % (dtmObject.bytepattern, dtmObject.frequency))

  
  if int(status) == 1:
    print "start test"
    try:
      dtmObject.startRx_test()
    except dtm.DTMError as error:
      print error.errormessage()   
  elif int(status) == 0:
    print "end test"
    try:
      print "received pkt :"+str(dtmObject.stopRx_test())
    except dtm.DTMError as error:
      print error.errormessage()     
  else:
    print "error test"
 
	
if __name__ == '__main__':
    main()
