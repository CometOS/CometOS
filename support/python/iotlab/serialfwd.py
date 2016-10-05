import sys
import traceback
import subprocess
import os
import time
import traceback
import datetime 

### iotlab imports
import util 


IOTLAB_M3_PORT = 20000

class SerialSniffer():
    ''' Wrap a process starting a script that reads sniffer output and stores it in a file 
    '''
    
    def __init__(self, snifflogDir, serialTunnel, dataRecordLen=80):
        self.processes = []
        
        if not os.path.exists(snifflogDir):
            os.makedirs(snifflogDir)
        ## start subprocesses
        for mapping in serialTunnel.portMappings:
            targetfile = "{0}/{1}_sniff_{2}".format(snifflogDir, datetime.date.today().isoformat(), mapping['uid'])
            args = ['./logSnifferOutput.py', mapping['port'], str(serialTunnel.baudrate), targetfile, "-l", str(dataRecordLen)]
            p = subprocess.Popen(args)
            self.processes.append(p)
            print "Started sniffer log script with pid {0}".format(p)
            
    def __enter__(self):
        return self
    
    def __exit__(self, exception_type, exception_val, exception_tb):
        print "ExceptionType: {0} ExceptionVal: {1} ExceptionTb: {2}".format(exception_type, exception_val, traceback.print_tb(exception_tb))
        for p in self.processes:
            "Print terminating sniffer read scripts with pid {0}".format(p)
            _shutdownProcess(p)
        

class Ressource(object):
    def __init__(self):
        self.ressources = []
        print "Creating {0}...".format(type(self))

    def __enter__(self):
        return self
    
    
    def __exit__(self, exception_type, exception_val, exception_tb):
        print "ExceptionType: {0} ExceptionVal: {1} ExceptionTb: {2}".format(exception_type, exception_val, traceback.print_tb(exception_tb))
        print "Deleting Forwarding processes: {0}".format(self.ressources)
        for p in self.ressources:
            print "Killing {0}...".format(p.pid)
            _shutdownProcess(p)
        return True




class SerialTunnel(Ressource):
    DEVICE_PATH = "{0}/dev".format(os.environ['HOME'])
    
    def __init__(self, nodeFile, baudrate):
        super(SerialTunnel, self).__init__()
        self.baudrate = baudrate
        self.portMappings = []
         
        if not os.path.exists(SerialTunnel.DEVICE_PATH):
            os.makedirs(SerialTunnel.DEVICE_PATH)

        self.nodes = util.parseNodeFile(nodeFile)


    def appendComportToMappings(self, nodeId, nodeUid):
        comport = "{0}/virtualcom{1}".format(SerialTunnel.DEVICE_PATH, nodeId)
        self.portMappings.append({"port" : comport, "uid" : nodeUid})
        return comport
    
    
def createA8toM3Forwarder(nodefile, gwfile):
    ''' To be run on A8 nodes.

        Create socat processes to forward between A8
        node virtual serial comms and m3-node serial comms. Connects
        to port "20{0:03}".format(<m3-id>) at A8 node and forwards
        to m3-<m3-id>:20000. To be used together with the A8toM3 subclass
        of SerialTunnel
    '''
    global IOTLAB_M3_PORT
    nodes = util.parseNodeFile(nodefile)
    gateways = util.parseNodeFile(gwfile)
    for n in nodes:
        for gw in gateways:
            a8port = "20{0:03}".format(n['id'])
            socatParams = ["socat",
                           "tcp:node-{0}:a8port".format(gw['id']),
                           "tcp:m3-{0}:{1}".format(n['id'], IOTLAB_M3_PORT)
                          ]
            socatProc = subprocess.Popen(socatParams)   


class A8toM3_SshSerialFwd(SerialTunnel):
    def __init__(self, nodefile, login, baudrate=500000, sshgwAddr='10.0.15.251', remoteport=20000):
        super(A8toM3_SshSerialFwd,self).__init__(nodefile,baudrate)
        for n in self.nodes:
            comport = self.appendComportToMappings(n['id'], n['uid'])
            sshParams = ['ssh',
                         '{0}@{1}'.format(login, sshgwAddr),
                         '-N',
                         '-L',
                         '20{0:03}:m3-{0}:{1}'.format(n['id'], IOTLAB_M3_PORT)
                        ]
            print "Opening ssh tunnel to iotlab ssh gateway: {0}".format(sshParams)
            pid = subprocess.Popen(sshParams)
            self.ressources.append(pid)
            time.sleep(1.0)
            socatParams = ['socat',
                           'tcp:localhost:20{0:03}'.format(n['id']),
                           'pty,link={0},raw,b{1}'.format(comport, baudrate)
                          ] 
            print "Creating socat forwarder: {0}".format(socatParams)
            pid = subprocess.Popen(socatParams)
            self.ressources.append(pid)


class A8toSshGw_SshTunnel(Ressource):
    def __init__(self, login, localport, remoteport, forwardTo="localhost", sshgwAddr='10.0.15.251'):
        global IOTLAB_M3_PORT 
        super(A8toSshGw_SshTunnel,self).__init__()
        sshParams = ['ssh',
                     '{0}@{1}'.format(login, sshgwAddr),
                     '-N',
                     '-L',
                     '{0}:{1}:{2}'.format(localport, forwardTo, remoteport)
                    ]
        print "Opening ssh tunnel to iotlab ssh gateway: {0}".format(sshParams)
        pid = subprocess.Popen(sshParams)
        self.ressources.append(pid)



class A8toM3_SocksProxy(Ressource):
    def __init__(self):
        super(A8toM3_SocksProxy, self).__init__()
        sshParams = ['ssh',
                     'weigel@10.0.15.251',
                     '-D',
                     '{0}'.format(A8toM3_Socks.SOCKS_PORT),
                     '-N'] 
        print "Start socks proxy on remote host"
        sshProc = subprocess.Popen(sshParams)
        self.ressources.append(sshProc)


class A8toM3_Socks(SerialTunnel): 
    SOCKS_PORT = 1234
    def __init__(self, nodeFile, baudrate=500000):
        global IOTLAB_M3_PORT
        super(A8toM3_Socks, self).__init__(nodeFile, baudrate)
        
        for n in self.nodes:
            ### we expect a socks proxy running on the ssh gateway node
            comport = self.appendComportToMappings(n['id'], n['uid'])
            socatParams = ["socat", 
                           "socks4:localhost:m3-{0}:{1},socksport={2}".format(n['id'], IOTLAB_M3_PORT, A8toM3_Socks.SOCKS_PORT),
                           "pty,link={0},raw,b{1}".format(comport, self.baudrate)
                          ]
            print "Open connection to M3 node serial port"
            print socatParams
            socatProc = subprocess.Popen(socatParams)
            self.ressources.append(socatProc)

        

class SshGwToM3(SerialTunnel):
    def __init__(self, nodeFile, baudrate=500000):
        global IOTLAB_M3_PORT
        super(SshGwToM3, self).__init__(nodeFile, baudrate)
        
        for n in self.nodes:
            comport = self.appendComportToMappings(n["id"], n["uid"])
            socatParams = ["socat",
                           "pty,link={0},raw,b{1}".format(comport, self.baudrate),
                           "tcp:m3-{0}:{1}".format(n['id'],IOTLAB_M3_PORT)]
            print "Create comport at {0}, forward to m3-{0}".format(comport, n['id'])
            socatProc = subprocess.Popen(socatParams)
            
            self.ressources.append(socatProc)


class SshToIotlabM3(SerialTunnel):
    IOTLAB_DOMAIN = "iot-lab.info"
    
    def __init__(self, nodeFile, iotlabLogin, baudrate=500000, site='grenoble'):
        global IOTLAB_M3_PORT
        super(SshToIotlabM3, self).__init__(nodeFile, baudrate)
        self.login = iotlabLogin
        self.site = site
          
        self.fakeAddress = None
        
        for n in self.nodes:
            ## comport
            comport = self.addComportToMappings(n["id"], n["uid"])
             
            sshProc = None
            if iotlabLogin != None:
                localPort = "20{0:03}".format(n["id"])
                ## open ssh tunnel to node and virtual comport 
                sshParams = ["ssh",
                             "-N",
                             "-L",
                             "{0}:{1}:{2}".format(localPort, n["serialHost"], IOTLAB_M3_PORT),
                             "{0}@{1}.{2}".format(self.login, self.site, SerialTunnel.IOTLAB_DOMAIN)
                             ]
                print "opening ssh tunnel: {0}".format(" ".join(sshParams))
                sshProc = subprocess.Popen(sshParams)
                time.sleep(1)
                tcpTargetPort = "tcp:{0}:{1}".format("localhost", localPort)
                self.ressources.append(sshProc)
            else:
                tcpTargetPort = "tcp:{0}:{1}".format(n["serialHost"], IOTLAB_M3_PORT)
            
            socatParams =  [
                            "socat", 
                            "pty,link={0},raw,b{1}".format(comport, self.baudrate),
                            tcpTargetPort
                            ]
            
            print "creating virtual serial com tunnel: {0} --> {1}".format(" ".join(socatParams), tcpTargetPort)
            socatProc = subprocess.Popen(socatParams)
            
            self.ressources.append(socatProc)
            
            sshPid = sshProc.pid if sshProc != None else 0
            
            print "Processes created: ssh={0} socat={1}".format(sshProc.pid, socatProc.pid)
            
        if len(nodes) == 1:
            self.fakeAddress = n["uid"]

class DummyContextMngr():
    def __enter__(self):
        print "Creating dummy context manager"
        return None
    def __exit__(self, exception_type, exception_val, exception_tb):
        return False

   
           
            
def _shutdownProcess(process):
        process.terminate()
        for i in range(5):
            if process.poll() != None:
                print "Process {0} terminated".format(process.pid)
                return
            else:
                time.sleep(0.1)
                print "Waiting for process {0} to terminate".format(process.pid)
        print "Killing process {0}".format(process.pid)
        process.kill()
