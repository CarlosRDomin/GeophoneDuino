#!/usr/bin/python

import os
import time
from datetime import datetime, timedelta
from ws4py.client import WebSocketBaseClient
# from ws4py.manager import WebSocketManager
from threading import Thread, Event


OUTPUT_FOLDER = os.path.abspath("Experiment data")
event_stop = Event()
ws_threads = []
# ws_manager = WebSocketManager()


class DataReceiver(WebSocketBaseClient):
	def __init__(self, url, delta_new_file=60, *args, **kwargs):
		super(DataReceiver, self).__init__(url, *args, **kwargs)
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
			else:
				self.run()  # If we were able to connect, then run the websocket (received_message will get called appropriately)

		print("Thread in charge of '{}' exited :)".format(self.url))

	def handshake_ok(self):
		print("Successfully connected to '{}'!".format(self.url))
		# ws_manager.add(self)

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
		self.output_file_handle.write(s + ',')
		print("Received data from '{}'!".format(self.url))

	def close(self, code=1000, reason=''):
		self.output_file_handle.close()
		print("Closed socket! Data was saved at '{}'".format(self.output_filename))


def start_data_collection(conn_info):
	if not os.path.exists(OUTPUT_FOLDER):
		os.makedirs(OUTPUT_FOLDER)

	for (ip, port) in conn_info:
		ws_url = "ws://{}:{}/geophone".format(ip, port)
		ws = DataReceiver(ws_url, delta_new_file=10)
		ws_thread = Thread(target=ws.run_thread)
		ws_threads.append(ws_thread)
		ws_thread.start()


if __name__ == '__main__':
	try:
		# ws_manager.start()
		start_data_collection([('192.168.0.1', 81), ('192.168.0.101', 81)])
		while True:
			time.sleep(1)

		# while True:
		# 	for ws in ws_manager.websockets.itervalues():
		# 		if not ws.terminated:
		# 			break
		# 	else:
		# 		break
		# 	time.sleep(3)
	except KeyboardInterrupt:
		# ws_manager.close_all()
		# ws_manager.stop()
		# ws_manager.join()
		print("Stopping data collection! (If a thread is attempting a connection to an unreachable IP it may take up to 60s to exit)")
		event_stop.set()
		for ws_thread in ws_threads:
			ws_thread.join()
