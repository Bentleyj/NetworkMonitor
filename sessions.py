import random
import socket
import sys
import time


HOST = 'localhost'
SESSION_PORT = 7106
HEARTBEAT_PORT = 7104

sessionSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
heartbeatSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def writeToSessionSocket(msg):
	print('[S] ' + msg)
	heartbeatSocket.sendto(msg, (HOST, SESSION_PORT))				
	
def writeToHeartbeatSocket(msg):
	print('[H] ' + msg)
	heartbeatSocket.sendto(msg, (HOST, HEARTBEAT_PORT))				

	

class Machine:
	def __init__(self, sessionName, machineID):
		self.sessionName = sessionName
		self.machineID = machineID
		self.enterState('STOPPED')
		
	def update(self):
		self.timer = self.timer - 1
		if self.timer == 0:
			if self.state == 'STOPPED':
				self.enterState('STARTING')
			elif self.state == 'STARTING':
				self.enterState('ACTIVE')
			elif self.state == 'ACTIVE':
				if random.random() < 0.95:
					writeToHeartbeatSocket('MACHINESTATUS|'+self.machineID+'|11.2|60')
					self.enterState('ACTIVE')
				else:
					self.enterState('CRASHED')
			elif self.state == 'CRASHED':
				self.enterState('ACTIVE')
			

	def enterState(self, newState):
		if newState == 'STOPPED':
			self.timer = random.randrange(10, 100)
		elif newState == 'STARTING':
			writeToHeartbeatSocket('HELLO|'+self.machineID)			
			self.timer = random.randrange(5, 10)
		elif newState == 'ACTIVE':
			if self.state != 'ACTIVE':
				self.onBecomeActive()
			self.timer = random.randrange(1, 5)
		elif newState == 'CRASHED':
			self.timer = random.randrange(1, 50)
		self.state = newState;
		
class Master(Machine):
	def __init__(self, sessionName, machineID, machineIDs):
		self.machineIDs = machineIDs
		Machine.__init__(self, sessionName, machineID)

	def onBecomeActive(self):
		msg = 'SESSION2|'+self.sessionName+'|'+self.machineID
		for machine in self.machineIDs:
			msg = msg + '|' + machine
		writeToSessionSocket(msg)
		
class Slave(Machine):
	def onBecomeActive(self):
		writeToSessionSocket('MACHINE|'+self.machineID+'|'+self.sessionName)

		
	
machines = [Master('my session', 'paulspc', ['peterspc', 'rachelspc', 'bobspc']),
			Slave('my session', 'peterspc'), 
			Slave('my session', 'rachelspc'), 
			Slave('my session', 'bobspc'), 
			Slave('test session', 'aliceslaptop')]

while (True):
	map(lambda machine:machine.update(), machines)
	time.sleep(0.1)
	
