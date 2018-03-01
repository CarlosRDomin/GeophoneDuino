#!/usr/bin/python

import os
import socket
import time
from datetime import datetime, timedelta
from ws4py.client import WebSocketBaseClient
from threading import Thread, Event


OUTPUT_FOLDER = os.path.abspath("Experiment data")
event_stop = Event()
ws_threads = []


class DataReceiver(WebSocketBaseClient):
	def __init__(self, url, delta_new_file=60, *args, **kwargs):
		super(DataReceiver, self).__init__(url, *args, **kwargs)
		self.sock.settimeout(5)  # Set the socket timeout so if a host is unreachable it doesn't take 60s (default) to figure out
		self.output_filename = ''
		self.output_file_handle = None
		self.deadline_new_file = datetime.now()
		self.DELTA_NEW_FILE = timedelta(seconds=delta_new_file)

	def generate_new_filename(self):
		self.output_filename = os.path.join(OUTPUT_FOLDER, "data_{}_{}.csv".format(self.bind_addr[0], datetime.now().strftime('%Y-%h-%d_%H-%M-%S')))
		self.deadline_new_file += self.DELTA_NEW_FILE  # Update the timestamp at which to start a new file

	def run_thread(self):
		# Run forever until event_stop tells us to stop
		while not event_stop.is_set():
			print("Connecting to '{}'...".format(self.url))
			try:
				self.connect()  # Attempt to connect to the Arduino
			except Exception as e:
				print("Unable to connect to '{}' (probably timed out). Reason: {}".format(self.url, e))
				if isinstance(e, socket.error):  # If host is unreachable, slow down the re-connect rate so it doesn't use up all the network
					time.sleep(5)

			else:
				self.run()  # If we were able to connect, then run the websocket (received_message will get called appropriately)

		print("Thread in charge of '{}' exited :)".format(self.url))

	def opened(self):
		print("Successfully connected to '{}'!".format(self.url))

	def received_message(self, msg):
		# Parse the message
		s = str(msg.data)
		if s.startswith('['): s = s[1:-1]  # Remove brackets if necessary

		# Check if we need to start a new file
		if datetime.now() > self.deadline_new_file:
			# Close existing file if necessary
			if self.output_file_handle:
				self.output_file_handle.close()
				print("\tClosed file: '{}' (it's been {}s)".format(self.output_filename, self.DELTA_NEW_FILE.total_seconds()))

			# And create a new one
			self.generate_new_filename()
			self.output_file_handle = open(self.output_filename, 'w')

		# Write the parsed message to the file
		try:  # In case the file has been closed (user stopped data collection), surround by try-except
			self.output_file_handle.write(s + ',')
		except Exception as e:
			print("Couldn't write to '{}'. Error: {}".format(self.output_filename, e))
		print("Received data from '{}'!".format(self.url))

	def close(self, code=1000, reason=''):
		try:
			super(DataReceiver, self).close(code, reason)
		except socket.error as e:
			print("Error closing the socket '{}' (probably the host was unreachable). Reason: {}".format(self.url, e))

	def closed(self, code, reason=None):
		if self.output_file_handle:
			self.output_file_handle.close()
			self.output_file_handle = None
			print("Data was saved at '{}' after closing the socket".format(self.output_filename))

	def unhandled_error(self, error):
		print("unhandled_error")


def start_data_collection(conn_info):
	if not os.path.exists(OUTPUT_FOLDER):
		os.makedirs(OUTPUT_FOLDER)

	for (ip, port) in conn_info:
		ws_url = "ws://{}:{}/geophone".format(ip, port)
		ws = DataReceiver(ws_url, delta_new_file=10)
		ws_thread = Thread(target=ws.run_thread)
		ws_threads.append((ws_thread, ws))
		ws_thread.start()


if __name__ == '__main__':
	try:
		# Start the data collection process
		start_data_collection([('192.168.0.112', 81), ('192.168.0.116', 81), ('192.168.0.108', 81), ('192.168.0.200', 81)])

		# And wait for a keyboard interruption while threads collect data
		while True:
			time.sleep(1)
	except KeyboardInterrupt:
		print("Stopping data collection!")

		event_stop.set()  # Let the threads know they need to exit

		# Close all the websockets
		for (_, ws) in ws_threads:
			Thread(target=ws.close).start()  # ws.close is blocking so just call it from a new thread (as long as we're not collecting data from too many nodes, we shouldn't hit the max thread limit)
		# And wait for all threads to finish
		for (ws_thread, _) in ws_threads:
			ws_thread.join()
