#!/usr/bin/env python 

# TODO
#figure out how to use pdv_initcam; reference initcam.c and edtapi

import __future__
import sys, os, time, math, threading
import datetime
import subprocess
#from wclogger import #WCLogger
import traceback
import struct
from ctypes import *
from PIL import Image, ImageDraw

#_EdtLib = CDLL("libpdv.so")
_EdtLib = CDLL("/opt/EDTpdv/libpdv.so")

TIMEOUT_OFFSET = 15000

class EdtIF:
    numbuffs = 1
    MINEXPTIME = .000001    #CRED2: Arbitrary value used to replace JoeCam val

    def __init__(self, devName=b'pdv'):
        """
        """
        #CRED2: Confirmed and tested
        self._initEdtIF()
        self.lockComm = threading.Lock()
        self.imageBuffer = [], 0 
        self.open(0, 0, devName)
        self.camName = devName
        self.blackColumns = 0
        self.darkColumns = 0
        self.initialize()
        self.getTimeouts()        
        self.getInfo()

    def _initEdtIF(self):
        #CRED2: Confirmed and tested
        _EdtLib.pdv_close.argtypes = [c_void_p]
        _EdtLib.pdv_enable_external_trigger.argtypes = [c_void_p, c_int]
        _EdtLib.pdv_flush_fifo.argtypes = [c_void_p]
        _EdtLib.pdv_get_height.argtypes = [c_void_p]
        _EdtLib.pdv_get_height.restype = c_int
        _EdtLib.pdv_get_timeout.argtypes = [c_void_p, c_int]
        _EdtLib.pdv_get_timeout.restype = c_int
        _EdtLib.pdv_get_width.argtypes = [c_void_p]
        _EdtLib.pdv_get_width.restype = c_int
        _EdtLib.pdv_last_image_timed.argtypes = [c_void_p, c_void_p]
        _EdtLib.pdv_last_image_timed.restype = POINTER(c_short)
        _EdtLib.pdv_multibuf.argtypes = [c_void_p, c_int]
        _EdtLib.pdv_open_channel.argtypes = [c_char_p, c_int, c_int]
        _EdtLib.pdv_open_channel.restype = c_void_p
        _EdtLib.pdv_serial_command.argtypes = [c_void_p, c_char_p]
        _EdtLib.pdv_serial_command.restype = c_int
        _EdtLib.pdv_serial_read.argtypes = [c_void_p, c_char_p, c_int]
        _EdtLib.pdv_serial_read.restype = c_int
        _EdtLib.pdv_serial_wait.argtypes = [c_void_p, c_int, c_int]
        _EdtLib.pdv_serial_wait.restype = c_int
        _EdtLib.pdv_set_exposure.argtypes = [c_void_p, c_int]
        _EdtLib.pdv_set_timeout.argtypes = [c_void_p, c_int]
        _EdtLib.pdv_start_images.argtypes = [c_void_p, c_int]
        _EdtLib.pdv_timeout_restart.argtypes = [c_void_p, c_int]
        _EdtLib.pdv_timeouts.argtypes = [c_void_p]
        _EdtLib.pdv_timeouts.restype = c_int
        _EdtLib.pdv_wait_image_timed.argtypes = [c_void_p, c_void_p]
        _EdtLib.pdv_wait_image_timed.restype = POINTER(c_short)
        _EdtLib.pdv_wait_last_image.argtypes = [c_void_p, c_void_p]
        _EdtLib.pdv_wait_last_image.restype = POINTER(c_short)

    def open(self, unit, channel, devName):
        #CRED2: Confirmed and tested
        self.unit, self.channel, self.devName = unit, channel, devName
        self.pdvObj = _EdtLib.pdv_open_channel(devName, unit, channel)

    def initialize (self):
        #CRED2: Confirmed and tested
        self.stopContinuousMode()
        _EdtLib.pdv_multibuf(self.pdvObj, self.numbuffs)
        self.width, self.height = self.getImageSize()
        self.expTime = 0
        self.timeouts = 0
        self.timeout = 0
        self.extMode = 0
        self.hdBinning = 1
        self.setExtMode(0)      #CRED2 should work w/ internal trigger
                                    #As such, use 0, NOT 2
        ##WCLogger.info("Little Joe version " + self.getVersion())
        #self.setProgNum(0)     #CRED2: Not needed, see function
        _EdtLib.pdv_flush_fifo(self.pdvObj)
        self.setExposureTime(.05) 
        #self.startInfoReaderThread() #CRED2: Not needed, see function
        self.startContinuousMode()
        self.startReaderThread()
        self.getLastImage(wait=True) 
        self.enableTriggers(0)
        #self._startFreeRun()
        return    

    def reset (self):
        self.initialize()

    def _serialCommand(self, cmd):
        #CRED2: Confirmed and tested
        return _EdtLib.pdv_serial_command(self.pdvObj, bytes(cmd, "UTF-8"))

    def _serialWait (self, timeout, maxchars):
        #CRED2: Confirmed and tested
        return _EdtLib.pdv_serial_wait(self.pdvObj, timeout, maxchars)

    def _serialRead(self, timeout=100, nchars=32):
        #CRED2: Confirmed and tested
        revbuf = create_string_buffer(b'\000' * 40)
        n = self._serialWait(timeout, nchars) 
        out = []
        while n > 0:
            res = _EdtLib.pdv_serial_read(self.pdvObj, revbuf, nchars)
            #print ("res %s " % res)
            if res > 0:
                data = revbuf.value.decode('UTF-8')
                out.append (data)
                n = self._serialWait(timeout, nchars)
            else:
                break
        res = "".join(out).strip()
        #print ("serialRead %s" % (res), " len %d" % len(res))
        
        if len(res) == 0:
            return ""
        if ord(res[0]) == 6:
            return res[1:]
        if len(res) > 3:
            return res
        raise Exception ("Error in response")

    def _sendCommand(self, cmd):
        #CRED2: Confirmed and tested
        with self.lockComm:
            return self._sendCommandLocked(cmd)
            
    def _sendCommandLocked(self, cmd):
        #CRED2: Confirmed and tested        
        try:
            res = self._serialRead() 
        except Exception as e:
            pass
        for i in range(3):
            try :
                sentRes = self._serialCommand(cmd)
                recRes = self._serialRead()
                ###WCLogger.info ("Sent res=%s, rec res=%s" % (sentRes, recRes))
                return recRes
            except Exception as e:
                ##WCLogger.error ("warning while sending " + cmd + " " + str(e))
                continue
        raise Exception ("Error in sendCommand")

    def _parseResponse (self, resp):
        """
        Parses response in the form "@CMD #N:NN; #N:NN"
        Returns a list of NN,NN,..
        """
        #CRED2: This function is not needed; 
            #our commands don't follow this format
        #try:
        #    parts = resp.split()
        #    #print ("parsing", parts)
        #    out = []
        #    for p in parts[1:]:
        #        parts1 = p.split(':')
        #        if len(parts1) > 1:
        #            out.append(parts1[1].replace(';', ''))
        #    return out
        #except:
        #    return 0,''
        pass

    def setHdBinning (self, hdbin=1):
        #self.hdBinning = max(1, min(4, hdbin))
        pass
        
    def getHdBinning (self):
        return self.hdBinning
        
    def getInfo (self):
        #print (self._parseResponse(self._sendCommand("@RCL?\n")))
        #print (self._parseResponse(self._sendCommand("@TXC?\n")))
        #print (self._parseResponse(self._sendCommand("@PRG?\n")))
        #WCLogger.info ("PROGNR=%s, ExtMode=%s" % (self.getProgNum(), self.getExtMode()))
        #print ("ExtMode=", self.getExtMode())
        #print ("Temps", self.getTemperatures())
        return


    def getVersion (self):
        '''
        CRED2: returns single, printable string with all version values
        ie: use print(getVersion()) to print the result
        '''
        #CRED2: Confirmed and tested
        res = self._sendCommand("version")
        return "".join(res.split('\r'))

    def setExtMode(self, mode=0):
        """
        returns early if invalid mode is provided
        mode = 0, internal
        mode = 2, external
        """
        #CRED2: Confirmed and tested
        if mode == 0:
            mdstr = "off"
        elif mode == 2:
            mdstr = "on"
        else:
            return mode
        res = self._sendCommand("set extsynchro " + mdstr)
        #WCLogger.info("Set external mode %d" % mode)
        self.extMode = mode
        return mode

    def getExtMode (self):
        #CRED2: Confirmed and tested
        res = self._sendCommand("extsynchro")
        return " ".join (res.split()[1:])
        
    def setRepMode(self, rep=0):
        """
        Sets the number of additional repetition of the accumulation sequence.
        rep=0 means 1 time
        """
        #CRED2: Not needed for CRED2
        #CRED2 has no command to repeat captures like this, maybe IMRO
        #res = self._sendCommand("@REP %d" %rep) 
        pass

    def getRepMode(self):
        #CRED2: Not needed; no equivalent command
            #Though maybe IMRO is this, not sure
        #res = self._sendCommand("@REP?")
        #return int(self._parseResponse(res)[0])
        pass

    def setSeq(self, seq=1):
        """
        seq = 0 stop sequence, else start sequence
        """
        #CRED2: Not needed for CRED2
        #CRED2 has no command equivalent to SEQ; it is always in free-run mode
        #res = self._sendCommand("@SEQ %d" %seq)
        pass

    def setProgNum(self, num=0):
        """
        Default prg=0.
        """
        #CRED2: Not needed for CRED2; no prog equivalent
        #res = self._sendCommand("@PRG %d" %num)
        #prgNr = self._getProgNum()
        #self._progNr = prgNr
        #WCLogger.info("Set prog number %d res=%s" % (num, prgNr))
        #self.expIF.setProgNum(num)
        #WCLogger.info("Set exp control prog number %d" % num)
        pass
        
    def getProgNum (self):
        #CRED2: Not needed for CRED2; no prog equivalent
        #return self._progNr
        pass

    def _getProgNum(self):
        #CRED2: Not needed for CRED2; no prog equivalent
        #res = self._sendCommand("@PRG?")
        #try:
        #    parts = res.split()
        #    self._progNr = int(parts[1])
        #    return self._progNr
        #except:
        #    return self._progNr
        pass

    def getTemperatures (self, full=None):
        """
        Default is to return just sensor temperature
        Returns
            full = None: (float) just sensor temperature
            full != None: (str) 
                single, printable string with all temp values
                ie: use print(getVersion()) to print the result
        """
        #CRED2: Confirmed and tested
        if full is None:
            res = self._sendCommand("temperatures snake raw")
            return float(res.split()[0])
        else:
            res = self._sendCommand("temperatures")
            return "".join(res.split('\r'))
        
    def getTempSetpoint (self):
        '''
        Returns setpoint for the camera's sensor (float)
        '''
        #CRED2: Confirmed and tested
        res = self._sendCommand("temperatures snake setpoint raw")
        return float(res.split()[0])
        
    def setTemperature (self, degC):
        '''
        Set the temperature for the camera (sensor)
        degC: temperature to set in Celsius
        '''
        #CRED2: Confirmed and tested
        #WCLogger.info("Setting temp to %d degC" % degC)
        self._sendCommand("set temperatures snake " + str(degC))
        
    def getTimeout (self, timeout=1000):
        return _EdtLib.pdv_get_timeout (self.pdvObj)

    def setTimeout (self, timeout=1000):
        if self.timeout == timeout:
            return
        #WCLogger.info ("Setting timeout %.0f" % timeout)
        self.timeout = timeout
        _EdtLib.pdv_set_timeout (self.pdvObj, timeout)

    def setExposureTime(self, expTime=1):
        '''
        Set the exposure time (in s)
        '''
        #CRED2: Confirmed and tested
        lastExpTime = self.expTime
        self.expTime = expTime

        if expTime < self.MINEXPTIME:
            return
               
        #WCLogger.info("Set exposure time %.2f ms" % expTime)
        res = self._sendCommand("set tint " + str(expTime))
        self.setTimeout (int(max(TIMEOUT_OFFSET, TIMEOUT_OFFSET+(max(lastExpTime, self.expTime)*1000)))) 
        self._waitForExposure() 
        time.sleep(0.5)
        #WCLogger.info("Set exposure done")

    def _startFreeRun(self):
        _EdtLib.pdv_start_images(self.pdvObj, 0)

    def _startImage(self, nimgs=1):
        _EdtLib.pdv_start_images(self.pdvObj, nimgs)

    def _getImage (self, wait=False):
        #CRED2: Confirmed and tested
        def ts2str (timeStamp, ndigits=11):
            dtime = datetime.datetime.fromtimestamp(timeStamp)
            return dtime.strftime('%H:%M:%S.%f')[:ndigits]
        """
        Reads image from EDT image buffer.
        Checks that there is no timeouts.
        Adds the newly read image into the image queue.
        """
        
        tstamp = (c_uint * 2)(0, 0)
        size = self.width * self.height

        if wait:
            skipped = c_int(0)
            data = _EdtLib.pdv_wait_last_image(self.pdvObj, byref(skipped))
            dmaTime = datetime.datetime.now().timestamp()
        else:
            data = _EdtLib.pdv_last_image_timed(self.pdvObj, byref(tstamp))
            dmaTime = tstamp[0] + 1E-9 * tstamp[1]

        touts = self.getTimeouts() 
        if touts == 0:            
            data1 = [max(0,data[i]) for i in range(size)]
            ##WCLogger.info ("lowest %d " % min(data1))
            ##WCLogger.info ("Read image " + ts2str(dmaTime))
                     
            return data1, dmaTime
        else:
            #WCLogger.warn("Timeouts= %d" % (touts))            
            self.restartTimeout () 
            
        #WCLogger.warn("Out _getImage with error")
        
        return [], 0
 
    def _waitForExposure(self):
        #CRED2: Confirmed and tested
        start = datetime.datetime.now().timestamp()       
        
        data, ts = self.imageBuffer
        while start > ts:
            if self.expTime >= 1:
                time.sleep(0.5)
            else:
                time.sleep(self.expTime)
            data, ts = self.imageBuffer
            if ts == 0:
                break
            
        now = datetime.datetime.now().timestamp()    
        return now - start

    def waitForExposure (self):
        #CRED2: Confirmed and tested
        wtime = self._waitForExposure()
        #WCLogger.info("Waited for %.2f s" % wtime)
        
                            
    def getLastImage (self, wait=False):
        #CRED2: Confirmed and tested
        try:
            if wait:
                #self.imageBuffer = self._getImage(wait=True)
                self.waitForExposure()
            return self.imageBuffer
        except Exception as e:
            traceback.print_exc()
            #WCLogger.info("GetLastImage failed " + str(e))
            return [], 0 

    def convert (self, data, size):
        fmt = "%dH" % size
        data = struct.pack (fmt, *data)
        return data

    def restartTimeout(self):
        _EdtLib.pdv_timeout_restart(self.pdvObj, 0)
        if self.continuousMode:
            self._startFreeRun()
        else:
            self._startImage(1)
        #WCLogger.warning ("Restarting after timeout " + str(self.timeout))
        return 

    def enableTriggers(self, flags=0):
        """
        This is to allow the external control.
        Not sure if really needed.
        flag 0: off, 1: photo trigger, 
            2: field ID trigger (not for PCI C-Link)
        """
        #CRED2: We don't use triggers but I kept this in just in case the 
            #FG boots into trigger mode or something. ie. probs not needed
        _EdtLib.pdv_enable_external_trigger (self.pdvObj, flags)
        
    def getTimeouts(self):
        """
        Gets the number of timeouts after a readout.
        """
        timeouts = _EdtLib.pdv_timeouts(self.pdvObj)
        diff = timeouts - self.timeouts
        self.timeouts = timeouts
        return diff

    def getImageSize(self):
        """
        Gets the size of the image from the EDT configuration.
        """
        width =_EdtLib.pdv_get_width(self.pdvObj)
        height =_EdtLib.pdv_get_height(self.pdvObj)
        return width, height

    def close(self):
        """
        Puts the camera in internal mode before quiting.
        """
        try:
            self.setSeq(1)
            #self.setExtMode(0)
            _EdtLib.pdv_start_images(self.pdvObj, 1)
            _EdtLib.pdv_close(self.pdvObj)
        except:
            pass
        
    def startContinuousMode (self):
        self.continuousMode = True
        self._startFreeRun() 

    def stopContinuousMode (self):
        self.continuousMode = False
        self._startImage(1)

    def startReaderThread (self):
        #CRED2: Confirmed and tested
        def runner ():
            while True :
                try:
                    ##WCLogger.info ("Wait for new image")
                    self.imageBuffer = self._getImage(True)  
                    data, ts = self.imageBuffer  
                    #dtime = datetime.datetime.fromtimestamp(ts) 
                    ##WCLogger.info ("Got image time stamp: " + dtime.strftime('%H:%M:%S.%f')[:11])
                except Exception as e:
                    #WCLogger.error("In reader " + e.getMessage())
                    pass
                    
        thr = threading.Thread(target=runner)
        thr.daemon = True
        thr.start()

    def startInfoReaderThread (self):
        #CRED2: Not needed; I see the use in a thread to constantly probe 
            #these vals but I don't think it's necessary for us
        #def runner():
        #    while True:
        #        time.sleep(1)        
        #        #self.getTemperatures()
        #thr = threading.Thread(target=runner)
        #thr.daemon = True
        #thr.start()
        pass

class EdtSim:
    def __init__(self, config=None):
        self.offsets = [0, 0]
        self.initialize(config)

    def initialize (self, config):
        self.stopContinuousMode()
        self.width, self.height = self.getImageSize()
        self.expTime = 0
        self.timeouts = 0
        self.timeout = 0
        self.extMode = 0
        self.hdBinning = 1
        self.setRepMode(0)
        self.setSeq(1)
        self.setExtMode(2)
        self.setGain(0)
        self.setOffset(0, 300)
        self.setOffset(1, 262)
        #WCLogger.info("Simulation mode " + self.getVersion())
        self.setExposureTime(1000)
        self.startContinuousMode()
        self.startReaderThread()
        self.stopExposure()
        self.getLastImage(wait=True)
        return

    def setHdBinning (self, hdbin=1):
        pass
        
    def getHdBinning (self):
        return self.hdBinning
        
    def getVersion (self):
        return "Sim version"

    def setExtMode(self, mode=0):
        self.extMode = mode
        return mode

    def setRepMode(self, rep=0):
        pass

    def getRepMode(self):
        return 0

    def setSeq(self, seq=1):
        pass

    def getSeq(self):
        return 0

    def setProgNum(self, num=0):
        pass

    def getProgNum(self):
        return 0

    def getTemperatures (self):
        return [20, 12, 33, 5]
    
    def setTemperatureRaw (self, rawTemp):
        pass

    def setTemperature (self, chan, degC):
        pass
    
    def setOffset (self, chan, value):
        self.offsets[chan] = value

    def getOffsets(self):
        return self.offsets

    def setGain (self, gain=0):
        self.gain = gain
        
    def getGain (self):
        return self.gain

    def getTimeout (self, timeout=1000):
        return self.timeout

    def setTimeout (self, timeout=1000):
        self.timeout = timeout

    def setExposureTime(self, expTime=1000):
        self.expTime = expTime

    def _startFreeRun(self):
        pass

    def _startImage(self, nimgs=1):
        pass

    def _getImage (self, wait=False):
        def ts2str (timeStamp, ndigits=11):
            dtime = datetime.datetime.fromtimestamp(timeStamp)
            return dtime.strftime('%H:%M:%S.%f')[:ndigits]
        return [], 0

    def stopExposure (self):
        pass
 
    def waitForExposure (self):
        pass

    def getLastImage (self, wait=False):
        ts = datetime.datetime.now()
        img = Image.new ('L', self.getImageSize(), 0)
        dimg = ImageDraw.Draw (img)
        mid = 512
        l1, l2, l3 = 400, 390, 30
        dimg.ellipse ((mid-l1,mid-l1,mid+l1,mid+l1), fill=80)
        dimg.ellipse ((mid-l2,mid-l2,mid+l2,mid+l2), fill=60)
        sec = ts.second
        mins = ts.minute
        hh = (ts.hour % 12)
        #print ("hh", hh, mins, sec)
        hh = hh * 5
        for i, x in enumerate((hh, mins, sec)):
            angle =  1.5 * math.pi + x * math.pi / 30
            l4 = 200 + i * 80
            w = 30 - i * 10
            x1 = mid + l4 * math.cos(angle)
            y1 = mid + l4 * math.sin(angle)
            dimg.line ((mid,mid,x1,y1), fill=200, width = w)
        dimg.ellipse ((mid-l3,mid-l3,mid+l3,mid+l3), fill=85)
        #dimg.text ((50, 100), "%.0f"%sec, fill=(255))        
        buf = img.getdata()        
        return buf, ts

    def restartTimeout(self):
        pass

    def enableTriggers(self, flags=0):
        pass

    def getTimeouts(self):
        return 0

    def getImageSize(self):
        return 1024, 1024

    def close(self):
       pass

    def startContinuousMode (self):
        pass

    def stopContinuousMode (self):
        pass

    def startReaderThread (self):
        pass


def EdtIFFactory (simMode, config=None, devName=b'pdv'):
    if simMode:
        return EdtSim (config=None)
    else:
        return EdtIF(config, devName)

# Test cases are in edtTest.py

