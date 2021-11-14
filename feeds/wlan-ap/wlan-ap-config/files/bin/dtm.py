# Copyright (c) 2013 Nordic Semiconductor. All Rights Reserved.
#
# The information contained herein is property of Nordic Semiconductor ASA.
# Terms and conditions of usage are described in detail in NORDIC
# SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
#
# Licensees are granted free, non-transferable use of the information. NO
# WARRANTY of ANY KIND is provided. This heading must NOT be removed from
# the file.

import sys, os, serial, time

PRBS9				= 0
FOUR_ONE_FOUR_ZERO	= 1
ONE_ZERO			= 2
CONSTANT_CARRIER	= 3

LE_RESET			= 0
LE_RECEIVER_TEST	= 1
LE_TRANSMITTER_TEST = 2
LE_TEST_END			= 3

LE_TEST_STATUS_EVENT	  = 0
LE_PACKET_REPORTING_EVENT = 1

STATUS_SUCCESS	= 0
STATUS_ERROR	= 1

class DTMError(Exception):
	def __init__(self, testname, operation, message):
		self.testname = testname
		self.operation = operation
		self.message = message
	def errormessage(self):
		return "%s %s %s" % (self.testname, self.operation, self.message)
class ConnectionError(DTMError):
	pass
class MessageError(DTMError):
	pass
class PERError(DTMError):
	pass

class DTM(object):
	def __init__(self, setup):
		self.txportname = setup['TestSerialPortName']
		self.rxportname = setup['GoldenSerialPortName']
		self.frequency = setup['Frequency']
		self.bytepattern = setup['Bitpattern']
		self.length = setup['Length']
		self.runtimeinms = setup['Runtime']
		self.dbm = setup['Dbm']
		self.phymode = setup['PhyMode']
		self.debug = setup.get('debug', False)
		self.perlimit = setup.get('PERLimit', 30)
		self.teststartdelay = setup.get('TestPause', 0)
		
		self.baudrate = 19200
		self.parity = serial.PARITY_NONE
		self.stopbits = serial.STOPBITS_ONE
		self.bytesize = serial.EIGHTBITS
		
		self.txper = "N/A"
		self.rxper = "N/A"
		
		self.txserialport = 0
		self.rxserialport = 0
		
	def __del__(self):
		self._disconnect(self.txserialport)
		self._disconnect(self.rxserialport)
		
	def _debug(self, printmessage):
		if self.debug:
			print printmessage
		
	def _connect(self, portname):
		"""Sets up and connects a serialport"""
		self._debug ("Connect to port: " + portname)
		try:
			port = serial.serial_for_url(portname, baudrate = self.baudrate, parity=self.parity, bytesize=self.bytesize, stopbits=self.stopbits, timeout=1)
		except serial.SerialException as error:
			raise ConnectionError (self._testname, "Connect", "Serialport error " + error.message)
		return (port)
		
	def _disconnect(self, serialport):
		"""Disconnects a serialport"""
		if isinstance (serialport, serial.Serial):
			serialport.close()
		
	def _startTX(self):
		"""Starts the Transmitter part of the test"""
		self._debug ("Start TX")
		self._debug ("Com port: " + self.txportname)
		self.txserialport = self._connect(self.txportname)
		self._reset(self.txserialport, "TX")
		self._sendCommand(self.txserialport, LE_TRANSMITTER_TEST, self.frequency, self.length, self.bytepattern, "TX start")
		self._receiveEvent(self.txserialport, "TX start")	
	
	def _stopTX(self):
		"""Stops the Transmitter part of the test"""
		self._sendCommand(self.txserialport, LE_TEST_END, 0, 0, 0, "TX end")
		self._receiveEvent(self.txserialport, "TX end")
		self._disconnect(self.txserialport)
		self.txserialport = 0
			
	def _runRX(self):
		"""Runs the RX part of the test and returns the number of received packages"""
		self._debug("Start RX")
		self._debug("Com port: " + self.rxportname)
		self.rxserialport = self._connect(self.rxportname)
		self._reset(self.rxserialport, "RX")
		self._startTimer()
		self._sendCommand(self.rxserialport, LE_RECEIVER_TEST, self.frequency, self.length, self.bytepattern, "RX start")
		self._receiveEvent(self.rxserialport, "RX start")
		
		while not self._timedOut(self.endtime):
			pass
			
		self._sendCommand(self.rxserialport, LE_TEST_END, 0, 0, 0, "RX end")
		data = self._receiveEvent(self.rxserialport, "RX end")	
		self._disconnect(self.rxserialport)
		self.rxserialport = 0
		return (data)
		
	def _calculateTimeForPackage (self):
		"""Calculate how long time from start of one package to the start of next"""
		return 0.625
		
	def _calculatePER(self, receivedPackages):
		"""Calculates the PER"""
		maxpackages = int (round(self.runtimeinms / self._calculateTimeForPackage ()))
		lostpackages = maxpackages - receivedPackages
		lostpackages = max(0, lostpackages)
		self._debug("Received: %s Max: %s Lost: %s" % (receivedPackages, maxpackages, lostpackages))
		per = int (round((float(lostpackages) / float(maxpackages)) * 100))
		return per
		
	def _reset(self, serialport, printmessage):
		"""Resets a connection"""
		self._sendCommand(serialport, LE_RESET, 0, 0, PRBS9, "Reset " + printmessage)
		self._receiveEvent(serialport, "Reset " + printmessage)

	def _sendCommand(self, serialport, commandtype, data1, data2, packettype, printmessage=""):
		"""Sends a package.
		Parameters:
			* serialport - the port to send on
			* commandtype - contents of upper 2 bits of MSByte - one of LE_RESET, LE_RECEIVER_TEST, LE_TRANSMITTER_TEST, LE_TEST_END
			* data1 - contents of lower 6 bits of MSByte
			* data2 - contents of upper 6 bits of LSByte
			* packettype - contents of lower 2 bits of LSByte - one of PRBS9, FOUR_ONE_FOUR_ZERO, ONE_ZERO, CONSTANT_CARRIER"""
		serialport.flushInput()
		self._debug("Sending command " + printmessage)
		data1 = data1 + (commandtype << 6)
		data2 = (data2 << 2) + packettype
		self._debug("1: %s %s %s 2: %s %s %s" % (hex(data1), bin(data1), chr(data1), hex(data2), bin(data2), chr(data2)))
		command = chr(data1) + chr(data2)
		self._debug("Command: %s Result: %s" % (command, serialport.write(command)))
		
	def _receiveEvent(self, serialport, printmessage):
		"""Receives an event
		Parameters:
			* serialport - the port tp receive on
		Returns:
			For Receive test: the number of packages received
			For other: 0
		Will throw an exception if result is error"""
		
		self._debug("Receive event: " + printmessage)
		
		data = serialport.read(2)
		returndata = 0
		if len(data) < 2:
			raise ConnectionError(self._testname, printmessage, "Received less data than expected")
		self._debug("Bytes received: %s %s %s " % ((len (data)), ord(data[0]), ord(data[1])))
		event = ord(data[0])
		event = event & 0x80
		event = event >> 7

		if event == LE_TEST_STATUS_EVENT:
			self._debug("Status event")
			statusbyte = ord(data[1])
			returndata = statusbyte & 0x01
			if (not returndata == 0):
				self._debug("Event Error")
				"""raise MessageError(self._testname, printmessage, "Event Error")"""
		else:
			self._debug("other event")
			packetcountMSByte = ord(data[0])
			packetcountMSByte = packetcountMSByte & 0x7F
			packetcountMSByte = packetcountMSByte << 8
			packetcountLSByte = ord(data[1])
			returndata = packetcountMSByte + packetcountLSByte
		self._debug(returndata)
		return returndata
	
	def _getms(self):
		"""Get current number of ms since epoch"""
		millis = int(round(time.time() * 1000))
		return millis
	
	def _startTimer(self):
		"""Gets current time and sets the appropriate endtime"""
		self.timestarted = self._getms()
		self.endtime = self.timestarted + self.runtimeinms
	
	def _timedOut(self, endtime):
		"""Checks if timer has run out"""
		currentMs = self._getms()
		isTimedOut = endtime < currentMs
		return (isTimedOut)
		
	def _delay(self):
		"""Inserts a short delay"""
		delaystarted = self._getms()
		delayend = delaystarted + self.teststartdelay
		
		while not self._timedOut(delayend):
			pass
		
	def runTest(self):
		"""Runs the test"""
		self._startTX()
		receivedPackages = self._runRX()
		self._stopTX()
		per = self._calculatePER(receivedPackages)
		
		if per > self.perlimit:
			raise PERError(self._testname, "", "Packeterrorrate to high at %s%%" % (per))
		return per
	
	def runTransmitterTest(self):
		"""Runs transmitter tests"""
		self._testname = "Transmitter test"
		self._delay ()
		self.txper = self.runTest()
		return self.txper
	
	def runReceiverTest(self):
		"""Runs receiver tests"""
		originalTXPortname = self.txportname
		originalRXPortname = self.rxportname
		self.txportname = originalRXPortname
		self.rxportname = originalTXPortname
		self._testname = "Receiver test"
		self._delay ()
		self.rxper = self.runTest()
		self.txportname = originalTXPortname
		self.rxportname = originalRXPortname
		return self.rxper
			
	def runBothRXandTXTests(self):
		"""Runs tests first in normal defined order (DUT as TX, Golden as RX). Then runs in reversed order (DUT as RX, Golden as TX)"""
		self.runTransmitterTest()
		self.runReceiverTest()
	def _setDBM(self, serialport, data1, data2, printmessage=""):
		"""Set dBm.
		Parameters:
			* serialport - the port to send on
			* data1 - power 0dbm(0) 4dbm(4)	"""
		serialport.flushInput()
		self._debug("Sending command " + printmessage)
		data1 = data1 + (1 << 7)
		self._debug("1: %s %s %s 2: %s %s %s" % (hex(data1), bin(data1), chr(data1), hex(data2), bin(data2), chr(data2)))
		command = chr(data1) + chr(data2)
		self._debug("Command: %s Result: %s" % (command, serialport.write(command)))
		
	def _setPhyMode(self, serialport, data1, data2, printmessage=""):
		"""Set phymode.
		Parameters:
			* serialport - the port to send on
			* data1 - data rate 1(1Mbps) 2(2Mbps)	"""
		serialport.flushInput()
		self._debug("Sending command " + printmessage)
		data2 = (4 << (data2 - 1 ))
		self._debug("1: %s %s %s 2: %s %s %s" % (hex(data1), bin(data1), chr(data1), hex(data2), bin(data2), chr(data2)))
		command = chr(data1) + chr(data2)
		self._debug("Command: %s Result: %s" % (command, serialport.write(command)))
	def startTx_test(self):
		"""Starts the Transmitter part of the test"""
		self._debug ("Start TX Com port: " + self.txportname)
		self._testname = "tx test start"
		self.txserialport = self._connect(self.txportname)
		self._reset(self.txserialport, "TX")
		self._setDBM(self.txserialport, self.dbm, 11, "SET DBM")
		self._receiveEvent(self.txserialport, "SET DBM")
		self._setPhyMode(self.txserialport, 2, self.phymode, "SET PHYMODE")
		self._receiveEvent(self.txserialport, "SET PHYMODE")
		self._sendCommand(self.txserialport, LE_TRANSMITTER_TEST, self.frequency, self.length, self.bytepattern, "TX start")
		self._receiveEvent(self.txserialport, "TX start")
		self._disconnect(self.txserialport)

	def stopTx_test(self):
		"""Stop the Transmitter part of the test"""
		self._debug ("Stop TX Com port: " + self.txportname)
		self._testname = "tx test stop"
		self.txserialport = self._connect(self.txportname)
		self._sendCommand(self.txserialport, LE_TEST_END, 0, 0, 0, "TX end")
		self._receiveEvent(self.txserialport, "TX end")
		self._disconnect(self.txserialport)
		
	def startRx_test(self):
		"""Runs the RX part of the test and returns the number of received packages"""
		self._debug("Start RX Com port: " + self.txportname)
		self._testname = "RX test start"
		self.txserialport = self._connect(self.txportname)
		self._reset(self.txserialport, "RX")
		self._sendCommand(self.txserialport, LE_RECEIVER_TEST, self.frequency, self.length, self.bytepattern, "RX start")
		self._receiveEvent(self.txserialport, "RX start")
		self._disconnect(self.txserialport)
	
	def stopRx_test(self):
		"""Stop the RX part of the test and returns the number of received packages"""
		self._debug("Stop RX Com port: " + self.txportname)
		self._testname = "RX test stop"
		self.txserialport = self._connect(self.txportname)
		self._sendCommand(self.txserialport, LE_TEST_END, 0, 0, 0, "RX end")
		data = self._receiveEvent(self.txserialport, "RX end")	
		self._disconnect(self.txserialport)
		self.txserialport = 0
		return (data)
		
		
