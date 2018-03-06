#!/usr/bin/python

import os
import socket
import coloredlogs, verboselogs
import time
from datetime import datetime, timedelta
from ws4py.client import WebSocketBaseClient
from threading import Thread, Event


# Create a colored logger so it's easy to skim console messages
logger = verboselogs.VerboseLogger(__name__)
coloredlogs.install(level="DEBUG", fmt="%(asctime)s.%(msecs)03d %(levelname)-8s %(message)s", field_styles={"asctime": {"color": "cyan", "bold": True}, "levelname": {"bold": True}})


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

		logger.success("Thread in charge of '{}' exited :)".format(self.url))

	def opened(self):
		logger.success("Successfully connected to '{}'!".format(self.url))

	def received_message(self, msg):
		# Parse the message
		s = str(msg.data)
		if s.startswith('['): s = s[1:-1]  # Remove brackets if necessary
		else: return  # Ignore Geophone ID message (eg: Geophone_AABBBCC)

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
			self.output_file_handle.write(s + ',')
		except Exception as e:
			logger.error("Couldn't write to '{}'. Error: {}".format(self.output_filename, e))
		logger.debug("Received data from '{}'!".format(self.url))

	def close(self, code=1000, reason=''):
		try:
			super(DataReceiver, self).close(code, reason)
		except socket.error as e:
			logger.error("Error closing the socket '{}' (probably the host was unreachable). Reason: {}".format(self.url, e))

	def closed(self, code, reason=None):
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

	def start(self, conn_info):
		for (ip, port) in conn_info:
			ws_url = "ws://{}:{}/geophone".format(ip, port)
			# Create a websocket thread responsible for collecting data from ws_url
			ws = DataReceiver(self, ws_url, delta_new_file=10, heartbeat_freq=1)  # Change delta_new_file to how often (in seconds) a new file should be created!
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
	experiment = DataCollection()
	try:
		# Start the data collection process
		experiment.start([('10.0.0.100', 81)])

		# And wait for a keyboard interruption while threads collect data
		while True:
			time.sleep(1)
	except KeyboardInterrupt:
		experiment.stop()
