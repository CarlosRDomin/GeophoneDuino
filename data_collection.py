#!/usr/bin/python

import argparse
import os
import socket
import time
from datetime import datetime, timedelta
from ws4py.client import WebSocketBaseClient
from threading import Thread, Event
from log_helper import logger
from struct import unpack


class DataReceiver(WebSocketBaseClient, Thread):
	TIMEOUT = 5  # Specify a small socket timeout (in sec) such that if any operation takes too long, the call doesn't block forever

	def __init__(self, data_collection, url, delta_new_file=60, *args, **kwargs):
		Thread.__init__(self)
		self.data_collection = data_collection  # Keep track of the global data_collection object (to access OUTPUT_FOLDER and event_stop)
		self.url = url
		self.init_args = args  # Store the initialization args and kwargs so we can re-init the websocket later
		self.init_kwargs = kwargs
		self.output_filename = ''
		self.output_file_handle = None
		self.deadline_new_file = datetime.now()
		self.DELTA_NEW_FILE = timedelta(seconds=delta_new_file)

	def generate_new_filename(self):
		self.output_filename = os.path.join(self.data_collection.OUTPUT_FOLDER, "data_{}_{}.csv".format(self.bind_addr[0], datetime.now().strftime('%Y-%h-%d_%H-%M-%S')))
		self.deadline_new_file = datetime.now() + self.DELTA_NEW_FILE  # Update the timestamp at which to start a new file

	def run(self):
		# Run forever until event_stop tells us to stop
		while not self.data_collection.event_stop.is_set():
			# Initialize the websocket
			WebSocketBaseClient.__init__(self, self.url, *self.init_args, **self.init_kwargs)
			self.sock.settimeout(self.TIMEOUT)  # Set the socket timeout so if a host is unreachable it doesn't take 60s (default) to figure out
			logger.notice("Connecting to '{}'...".format(self.url))
			try:
				self.connect()  # Attempt to connect to the Arduino
			except Exception as e:
				logger.error("Unable to connect to '{}' (probably timed out). Reason: {}".format(self.url, e))
			else:  # If we were able to connect, then run the websocket (received_message will get called appropriately)
				while self.once():
					pass  # self.once() will return False on error/close -> Only stop when the connection is lost or self.close() is called
			self.terminate()
			time.sleep(2)  # Wait for a couple of seconds for Arduino to reboot/connect or just to avoid network overload

		logger.success("Thread in charge of '{}' exited :)".format(self.url))

	def opened(self):
		logger.success("Successfully connected to '{}'!".format(self.url))

	def received_message(self, msg):
		if msg.is_text: return  # Ignore Geophone ID message (eg: Geophone_AABBBCC)

		# Parse the message: '<' for Little-Endian, 'H' for uint16_t
		msg_format = '<' + 'H'*(len(msg.data)/2)
		msg_vals = unpack(msg_format, msg.data)
		cvs_vals = ','.join(map(str, msg_vals))  # Convert each item to str then join with ','

		# Check if we need to start a new file
		if datetime.now() > self.deadline_new_file:
			# Close existing file if necessary
			if self.output_file_handle:
				self.output_file_handle.close()
				logger.verbose("Closed file: '{}' (it's been {}s)".format(self.output_filename, self.DELTA_NEW_FILE.total_seconds()))

			# And create a new one
			self.generate_new_filename()
			self.output_file_handle = open(self.output_filename, 'w')

		# Write the parsed message to the file
		try:  # In case the file has been closed (user stopped data collection), surround by try-except
			self.output_file_handle.write(cvs_vals + ',')
		except Exception as e:
			logger.error("Couldn't write to '{}'. Error: {}".format(self.output_filename, e))
		logger.debug("Received data from '{}'!".format(self.url))

	def close(self, code=1000, reason=''):
		try:
			super(DataReceiver, self).close(code, reason)
		except socket.error as e:
			logger.error("Error closing the socket '{}' (probably the host was unreachable). Reason: {}".format(self.url, e))

	def closed(self, code, reason=None):
		self.deadline_new_file = datetime.now()  # Even if a reconnect happens before the current deadline, force the creation of a new file, instead of the 'reconnected' data being appended to the current file
		if self.output_file_handle:
			self.output_file_handle.close()
			self.output_file_handle = None
			logger.verbose("Data was saved at '{}' after closing the socket".format(self.output_filename))

	def unhandled_error(self, error):
		logger.error("Unhandled websocket error: {}".format(error))


class DataCollection:
	def __init__(self, output_folder=os.path.abspath("Experiment data")):
		self.OUTPUT_FOLDER = output_folder
		if not os.path.exists(self.OUTPUT_FOLDER):  # Create output folder if needed
			os.makedirs(self.OUTPUT_FOLDER)

		# Thread-related global variables
		self.event_stop = Event()
		self.ws_threads = []

	def start(self, conn_info, delta_new_file=60):
		for (ip, port) in conn_info:
			ws_url = "ws://{}:{}/geophone".format(ip, port)
			# Create a websocket thread responsible for collecting data from ws_url
			ws = DataReceiver(self, ws_url, delta_new_file)
			ws.start()  # Execute our custom run() method in the new thread
			self.ws_threads.append(ws)  # Store a list of all threads so we can close all sockets when the experiment needs to end

	def stop(self):
		logger.notice("Stopping data collection!")
		self.event_stop.set()  # Let the threads know they need to exit

		# First, close the websockets
		for ws in self.ws_threads:
			Thread(target=ws.close).start()  # ws.close is blocking so just call it from a new thread (as long as we're not collecting data from too many nodes, we shouldn't hit the max thread limit)
		# And wait for all threads to finish
		for ws in self.ws_threads:
			ws.join()


if __name__ == '__main__':
	parser = argparse.ArgumentParser(description='GeophoneDuino sensor network data collection')
	parser.add_argument('-l', '--ip-list', nargs='+', metavar='IP',
		help='Specify the (space-separated) list of IPs (eg: "... -l 192.168.0.101 192.168.0.105" to collect from only those two IPs).')
	parser.add_argument('-r', '--range', nargs=3, metavar=('PREFIX', 'FROM', 'TO'),
		help='Specify the range of IPs to collect data from. Eg: "--range 192.168.0 100 105" would connect to 192.168.0.100, 192.168.0.101, ..., 192.168.0.105 (NOTE THAT PREFIX DOESN''T END WITH A PERIOD) [default: %(default)s].',
		default=['192.168.0', '100', '100'])
	parser.add_argument('-p', '--port',
		help='Specify the port number to which the websocket should connect [default: %(default)s].',
		default=81, type=int)
	parser.add_argument('-n', '--new-file-interval',
		help='Specify how often (in seconds) a new file is created to store the data collected (prevents large and/or corrupted files) [default: %(default)s].',
		default=60, type=int)

	experiment = DataCollection()
	args = parser.parse_args()
	if args.ip_list is not None:
		conn_info = [(x, args.port) for x in args.ip_list]
	else:
		# Parse prefix (make sure it ends in a period)
		range_prefix = args.range[0]
		if not range_prefix.endswith('.'): range_prefix += '.'

		# And parse range start/end (ensure ints, etc.)
		try:
			range_start = int(args.range[1])
			range_end = int(args.range[2])
			if not (1 <= range_start <= range_end <= 254): raise ValueError("IP range value needs to be between 1 and 254")
		except ValueError:
			logger.critical("Couldn't parse IP range FROM/TO value (need to be ints between 1 and 254, with FROM <= TO)")
			exit(-1)
		conn_info = [(range_prefix + str(x), args.port) for x in range(range_start, range_end+1)]

	try:
		# Start the data collection process
		experiment.start(conn_info, args.new_file_interval)

		# And wait for a keyboard interruption while threads collect data
		while True:
			time.sleep(1)
	except KeyboardInterrupt:
		experiment.stop()
