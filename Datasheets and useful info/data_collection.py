#!/usr/bin/python

from ws4py.client import WebSocketBaseClient
from ws4py.manager import WebSocketManager
from ws4py import format_addresses
import time


WS_TIMEOUT = 3  # Seconds before we give up connecting
ws_manager = WebSocketManager()


class DataReceiver(WebSocketBaseClient):
	def handshake_ok(self):
		print("Connected to '{}'!".format(self.url))
		ws_manager.add(self)

	def received_message(self, msg):
		print("Received data! {}".format(msg))


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
		start_data_collection([('10.0.0.110', 81)])

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
