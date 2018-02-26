#!/usr/bin/python

from ws4py.client import WebSocketBaseClient
from ws4py.manager import WebSocketManager
import time


WS_TIMEOUT = 3  # Seconds before we give up connecting
ws_manager = WebSocketManager()


class DataReceiver(WebSocketBaseClient):
	def handshake_ok(self):
		print("Connected to '{}'!".format(self.url))
		ws_manager.add(self)
		self.FILE_NAME = "data_{}.txt".format(self.bind_addr[0])
		self.file_handle = open(self.FILE_NAME, 'w')

	def received_message(self, msg):
		s = str(msg.data)
		if s.startswith('['): s = s[1:-1]  # Remove brackets if necessary
		self.file_handle.write(s + ',')
		print("Received data! {}".format(msg if False else ''))

	def close(self, code=1000, reason=''):
		self.file_handle.close()
		print("Closed socket! Data was saved at '{}'".format(self.FILE_NAME))


def start_data_collection(conn_info):
	for (ip, port) in conn_info:
		ws_url = "ws://{}:{}/geophone".format(ip, port)
		print("Connecting to '{}'...".format(ws_url))

		try:
			ws = DataReceiver(ws_url)
			ws.connect()
		except Exception as e:
			print("Unable to connect to '{}' (it probably timed out after {}s). Reason: {}".format(ws_url, WS_TIMEOUT, e))


if __name__ == '__main__':
	try:
		ws_manager.start()
		# start_data_collection([('10.0.0.110', 81)])
		start_data_collection([('192.168.0.99', 81)])

		while True:
			for ws in ws_manager.websockets.itervalues():
				if not ws.terminated:
					break
			else:
				break
			time.sleep(3)
	except KeyboardInterrupt:
		ws_manager.close_all()
		ws_manager.stop()
		ws_manager.join()
