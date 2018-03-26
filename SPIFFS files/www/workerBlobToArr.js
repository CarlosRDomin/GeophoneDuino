function createNewWorker() {
	if (window.location.protocol === 'file:') {	// It's unsafe to load a WebWorker from the filesystem so browsers block loading Workers when protocol is 'file:' -> Workaround
		return new Worker(URL.createObjectURL(new Blob(["("+createNewWorkerWorkaround.toString()+")()"], {type: 'text/javascript'})));
	} else {
		return new Worker('workerBlobToArr.js');
	}
}

function createNewWorkerWorkaround() {	// It's unsafe to load a WebWorker from the filesystem so browsers block loading Workers when protocol is 'file:' -> Workaround
	self.onmessage = function(e) {
		importScripts(e.data.scriptPath);	// Import this script so we can call its functions
		workerOnMessage(e);					// Handle the message
	};
}

function workerOnMessage(e) {	// Determines how to handle an incoming message
	if (e.data.cmd === 'plot') {	// "Hack" so that the WebWorker can be reused to update the Plotly graph in a different thread
		self.postMessage(e.data);	// Simply forward the message so the worker.onmessage() function in the main html file handles the graph update
	} else {	// Otherwise cmd='parse' (convert Blob to binary array)
		convertBlobToBinArray(e.data.blob, e.data.toType);
	}
}
self.onmessage = workerOnMessage;	// If a WebWorker is instantiated with this script [new Worker('workerBlobToArr.js')], this line tells it how to handle incoming messages

function convertBlobToBinArray(blob, toType) {
	var blobToArrBuffConverter = new FileReader();	// Use helper class FileReader to convert Blob data (default class for binary ws data) to UInt16Array
	blobToArrBuffConverter.onload = function() {
		var binDataArr;
		switch (toType) {	// Once converted, cast the ArrayBuffer (this.result) to the requested binary type (uint16_t, double, etc.)
			case 'double':
				binDataArr = new Float64Array(this.result);
				break;
			default:
			case 'uint16_t':
				binDataArr = new Uint16Array(this.result);
		}
		self.postMessage({cmd: 'parse', binData: binDataArr});
	};
	blobToArrBuffConverter.readAsArrayBuffer(blob);	// Convert blob to the requested binary type (eg: uit16_t, double...) and pass the result back
}

function getWorkerScriptPath() {
	if (typeof getWorkerScriptPath.path == 'undefined') {	// Only need to compute the path once, so store in a "static" variable [the first time this function is run, getWorkerScriptPath.path is undefined so we can set its value appropriately]
		//getWorkerScriptPath.path = window.location.origin + window.location.pathname.replace(/\/[^\/]*$/g, '/workerBlobToArr.js');	// Get current path, rstrip from the last '/' on, then append this script's file name
		getWorkerScriptPath.path = window.location.origin + window.location.pathname.replace(/\/[^\/]*$/g, '/' + 'workerBlobToArr.js');
	}
	return getWorkerScriptPath.path;
}